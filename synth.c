#include <stdio.h>
#include "synth.h"

#define HALFTONE 1.059463094f
#define NOTES 32
#define MIN_VOLUME (1.f / 128.f)

static unsigned int my_seed = 32627;

__attribute__((fastcall)) static float my_rand()
{
  my_seed *= 16807;
  return ((float)my_seed) / (float)0x80000000;
}

__attribute((fastcall)) static float my_pow(float x, float y){
  float res;
  asm("flds %2; flds %1; fyl2x; fld %%st; frndint; fsubr %%st,%%st(1); fxch %%st(1); f2xm1;"
      "fld1; faddp; fscale; fstps %0; fstp %%st;"
      : "=m" (res) 
      : "m" (x), "m" (y) 
      : "eax");
  return res;
}

__attribute((fastcall)) static float calc_freq(float base, float interval, int nint){
  return base * my_pow(interval, (float) nint);
}

/* sin(2 * PI * f) */
__attribute__((fastcall)) static float sin4k(float f){
  float res;
  asm ("fld %1; fldpi; fld1; fld1; faddp; fmulp; fmulp; fsin; fstp %0;" : "=m" (res) : "m" (f));
  return res;
}

float do_osc_tri(float ph){ return sin4k(ph); }
float do_osc_saw(float ph){ return 2.f * ph - 1.f; }
float do_osc_sqr(float ph){ if(ph < .5f) return -1.f; return 1.f; }
float do_osc_nse(float ph){ return my_rand(); }

float (*osc_table[])(float) = { &do_osc_tri, &do_osc_saw, &do_osc_sqr, &do_osc_nse };

__attribute__((fastcall)) static void create_osc(osc *o, osc_type t, float f, float a){
  o->osc  = osc_table[t];
  o->amp  = a;
  o->ph   = 0.f;
  o->iph  = f * DT;
}

__attribute__((fastcall)) static float do_osc(osc *o){
  if((o->ph += o->iph) >= 1.f) o->ph -= 1.f;
  return o->osc(o->ph) * o->amp;
}

__attribute__((fastcall)) static void create_adsr(enveloppe *e, float a, float d, float s, float r){
  e->v = MIN_VOLUME;
  e->t = 0.f;
  e->a = a;
  e->s = s;
  e->ad = a+d;
  e->da = (a > 0.f) ? my_pow(1.f / MIN_VOLUME, DT / a) : 1.f / MIN_VOLUME;
  e->dd = (d > 0.f) ? my_pow(s, DT / d) : s;
  e->dr = (r > 0.f) ? my_pow(MIN_VOLUME / s, DT / r) : 0.f;
  ++e->on;
}

__attribute__((fastcall)) static float do_adsr(enveloppe *e){
  if(e->t < e->a) e->v *= e->da;
  else if((e->t - e->a) < DT) e->v = 1.f;
  else if(e->t < e->ad) e->v *= e->dd;
  else if(e->on) e->v = e->s;
  else e->v *= e->dr;
  e->t += DT;
  return e->v;
}

__attribute__((fastcall)) void create_note_instr(note_instr *n, instrument *i, int f, int a){
  ++n->used;
  n->instr = i;
  create_adsr(&(n->env),
              (float) i->a / 127.f,
              (float) i->d / 127.f,
              my_pow(MIN_VOLUME, 1.f - (float) i->s / 127.f),
              (float) i->r / 16.f);
  n->freq = f;
  n->amp = (float)(a) / 127.f;
  register int os = 2; do create_osc(n->o+os, i->o[os].type, 
                        calc_freq(0.197012587, 1.00045137f, 128 * (i->o[os].freqt + f) + i->o[os].freqf),
                        my_pow(MIN_VOLUME, 1.f - (float)(i->o[os].amp) / 127.f)); while(os--);
  n->low = n->band = 0.f;
}

__attribute__((fastcall)) float do_note_instr(note_instr *n){
  float e;
  float out = 0.f;
  if(n->used){
    if((e = do_adsr(&(n->env))) <= MIN_VOLUME){
      --n->used;
    } else {
      int i=3;
      while(i--) out += do_osc(n->o+i);
      out *= e * n->amp;
      n->cutoff = calc_freq(16.f, HALFTONE, n->instr->cutoff);
      n->f = 1.5f * sin4k(n->cutoff * DT / 2.0f);
      n->res = (float)(n->instr->res) / 127.f;
      n->low += n->f * n->band;
      float high = n->res * (out - n->band) - n->low;
      n->band += n->f * high;
      out = n->low;
    }
  }
  return out;
}

note_instr note[NOTES];

float reverb[SAMPLERATE];
int reverb_pos = 0;
int reverb_max = 0;
float reverb_level = 0.F;

void init_synth(){
  register int i=NOTES - 1;
  do { note[i].used = 0; } while(i--);
}

void update_instr(instrument *instr){
  reverb_max = instr->reverb_time * SAMPLERATE / 127;
  if(reverb_pos >= reverb_max) reverb_pos = 0;
  reverb_level = instr->reverb_level / 127.F;
}

void create_note(int freq, int amp, instrument *instr){
  int i=NOTES;
  note_instr *n = note;
  while(--i) {
    if(!n->used){
      create_note_instr(n++,instr,freq,amp);
      break;
    }
    n++;
  }
}

void release_note(int freq, int amp, instrument *instr){
  register int i=NOTES;
  register note_instr *n = note;
  while(--i) {
    if(n->used){
      if(/* (n->instr == instr) && */ (n->freq == freq)){
        n->env.on = 0;
      }
    }
    n++;
  }
}
float scope[SAMPLERATE];
int scope_pos=0;
void render_synth(short *audio_buffer, int len){
  int i;
  float out;
  register short *sp = audio_buffer;
  short sh;

  do {
    i=NOTES - 1; out=0.f;
    do {
      out += do_note_instr(note+i);
    } while(i--);
    if(reverb_max){
      out += reverb[reverb_pos] * reverb_level;
      reverb[reverb_pos++] = out;
      reverb_pos %= reverb_max;
    }
    sh = *(sp++) = (short)(out * 32767.f);
    *(sp++) = sh;
    scope[scope_pos++] = out; scope_pos %= SAMPLERATE;
  } while(--len);
}
