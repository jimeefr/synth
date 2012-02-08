#include <stdio.h>
#include "synth.h"

#define HALFTONE 1.059463094f
#define NOTES 32
#define MIN_VOLUME (1.f / 256.f)

static int my_seed = 32627;

static note_instr note[NOTES];

static float reverb[SAMPLERATE];
static int reverb_pos = 0;
static int reverb_max = 0;
static float reverb_level = 0.F;
float scope[SAMPLERATE];
int scope_pos=0;

__attribute__((noinline)) static float my_rand()
{
  union { float f; unsigned int i; } res;
  my_seed *= 16807;
  res.i = (((unsigned int)my_seed) >> 9) | 0x40000000;
  return res.f - 3.f;
}

__attribute((noinline)) static float my_pow(float x, float y){
  float res;
  asm("fyl2x;"
      "fld %%st; frndint;"
      "fsubr %%st,%%st(1);"
      "fxch %%st(1); f2xm1;"
      "fld1; faddp; fscale;"
      //"fstp %%st(1);"
      "fstps %0; fstp %%st;"
      : "=m" (res)
      : "t" (x),
        "u" (y)
      : "st","st(1)");
  return res;
}

/* sin(2 * PI * f) */
__attribute__((noinline)) static float sin4k(float f){
  float res;
  asm ("fldpi; fld1; fld1; faddp; fmulp; fmulp; fsin;" : "=t" (res) : "0" (f));
  return res;
}

#define do_osc_tri sin4k
__attribute__((noinline)) static float do_osc_saw(float ph){ return 2.f * ph - 1.f; }
__attribute__((noinline)) static float do_osc_sqr(float ph){ if(ph < .5f) return -1.f; return 1.f; }
__attribute__((noinline)) static float do_osc_nse(float ph){ return my_rand(); }

static float (*osc_table[])(float) = { do_osc_tri, do_osc_saw, do_osc_sqr, do_osc_nse };

__attribute__((always_inline)) static float do_osc(osc *o){
  if((o->ph += o->iph) >= 1.f) o->ph -= 1.f;
  return o->osc(o->ph) * o->amp;
}

static float do_adsr(enveloppe *e){
  if(e->t < e->a) e->v *= e->da;
  else if(e->t < e->ad) e->v *= e->dd;
  else if(!e->on) e->v *= e->dr;
  e->t += DT;
  return e->v;
}

float do_note_instr(note_instr *n){
  float e;
  float out = 0.f;
  if(n->used){
    if((e = do_adsr(&(n->env))) <= MIN_VOLUME){
      --n->used;
    } else {
      int i=n->nos-1;
      do { out += do_osc(n->o+i); } while(i--);
      out *= e * n->amp;
      n->cutoff = 16.f * my_pow(HALFTONE, (float) n->instr->cutoff);
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

void init_synth(){
  register int i=NOTES - 1;
  do { note[i].used = 0; } while(i--);
}

void update_instr(instrument *instr){
  reverb_max = instr->reverb_time * SAMPLERATE / 127;
  if(reverb_pos >= reverb_max) reverb_pos = 0;
  reverb_level = my_pow(MIN_VOLUME * 8.f, 1.f - (float)instr->reverb_level / 127.f);
}

__attribute__((always_inline)) static void create_adsr(enveloppe *e, float a, float d, float s, float r){
  a += DT; d += DT; r += DT;
  e->v = MIN_VOLUME;
  e->t = 0.f;
  e->a = a;
  e->s = s;
  e->ad = a+d;
  e->da = my_pow(1.f / MIN_VOLUME, DT / a);
  e->dd = my_pow(s, DT / d);
  e->dr = my_pow(MIN_VOLUME / s, DT / r);
  ++e->on;
}

__attribute__((always_inline)) static void create_osc(osc *o, osc_type t, float f, float a){
  o->osc  = osc_table[t];
  o->amp  = a;
  o->ph   = my_rand();
  o->iph  = f * DT;
}

__attribute__((always_inline)) static void create_note_instr(note_instr *n, instrument *i, int f, int a){
  ++n->used;
  n->instr = i;
  create_adsr(&(n->env),
              (float) i->a / 127.f,
              (float) i->d / 127.f,
              my_pow(MIN_VOLUME, 1.f - (float) i->s / 127.f),
              (float) i->r / 16.f);
  n->freq = f;
  n->amp = (float)(a) / 127.f;
  n->nos = 0; 
  int os=2; do {
    int unison = i->unison[os];
    if(i->amp[os]) do {
      create_osc(n->o+n->nos, i->type[os], 
                 0.197012587 * my_pow(1.00045137f, 128.f * (i->freqt[os] + f) + i->disper[os] * my_rand() + i->freqf[os]),
                 my_pow(MIN_VOLUME, 1.f - (float)(i->amp[os]) / 127.f)); 
      n->nos++;
    } while(unison--);
  } while(os--);
  n->low = n->band = 0.f;
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
    if(out<-1.f) out=-1.f;
    if(out> 1.f) out= 1.f;
    sh = *(sp++) = (short)(out * 32767.f);
    *(sp++) = sh;
    scope[scope_pos++] = out; scope_pos %= SAMPLERATE;
  } while(--len);
}
