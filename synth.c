#include <stdio.h>
#include "synth.h"

static unsigned int my_seed = 32627;

__attribute__((fastcall)) static float my_rand()
{
  //union { float f; unsigned int i; } res;

  my_seed *= 16807;
  //res.i = ( my_seed >> 9 ) | 0x40000000;
  //return (res.f - 3.0f);
  return ((float)my_seed) / (float)0x80000000;
}

__attribute((fastcall)) static float calc_freq(float base, float interval, int nint){
  while(nint--) base *= interval;
  return base;
}

__attribute__((fastcall)) static float sin4k(float f){
  float res;
  asm ("fld %1; fsin; fstp %0;" : "=m" (res) : "m" (f));
  return res;
}

__attribute__((fastcall)) static void create_osc(osc *o, osc_type t, float f, float a){
  o->type = t;
  o->freq = f;
  o->amp  = a;
  o->ph   = 0.f;
  o->iph  = f / SAMPLERATE_F;
}

__attribute__((fastcall)) static float do_osc(osc *o){
  float out;
  if((o->ph += o->iph) >=1.f) o->ph -= 1.f;
  if(o->type==OSC_TRIANGLE)    out = (o->ph < .5f) ? (4.f * o->ph - 1.f) : (-4.f * o->ph + 3.f);
  else if(o->type==OSC_SAW)    out = 2.f * o->ph - 1.f;
  else if(o->type==OSC_SQUARE) out = (o->ph < .5f) ? -1.f : 1.f;
  else                         out = my_rand();
  out *= o->amp;
  return out;
}

__attribute__((fastcall)) static void create_adsr(enveloppe *e, float a, float d, float s, float r){
  e->v = 0.f;
  e->t = 0.f;
  e->a = a;
  e->d = d;
  e->s = s;
  e->r = r;
  e->ad = a+d;
  e->da = (a > 0.f) ? 1.f / (a * SAMPLERATE_F) : 1.f;
  e->dd = (d > 0.f) ? (1.f - s) / (d * SAMPLERATE_F) : 1.f;
  e->dr = (r > 0.f) ? s / (r * SAMPLERATE_F) : 1.f;
  ++e->on;
}

__attribute__((fastcall)) static float do_adsr(enveloppe *e){
  if(e->t < e->a) e->v += e->da;
  else if((e->t - e->a) < DT) e->v = 1.f;
  else if(e->t < e->ad) e->v -= e->dd;
  else if(e->on) e->v = e->s;
  else e->v -= e->dr;
  e->t += DT;
  return e->v;
}

__attribute__((fastcall)) void create_note_instr(note_instr *n, instrument *i, int f, int a){
  ++n->used;
  n->instr = i;
  create_adsr(&(n->env),
              calc_freq(3.258886363e-3f,1.059463094f,i->a),
              calc_freq(3.258886363e-3f,1.059463094f,i->d),
              (float)(i->s) / 127.f,
              calc_freq(3.258886363e-3f,1.059463094f,i->r));
  n->freq = f;
  n->amp = (float)(a) / 127.f;
  register int os = 2; do create_osc(n->o+os, i->o[os].type, 
                        calc_freq(0.405570987f,1.059463094f,i->o[os].freqt+f) * 
                        calc_freq(0.971531941f,1.000451664f,i->o[os].freqf),
                        (float)(i->o[os].amp) / 127.f); while(os--);
  n->low = n->band = 0.f;
}

__attribute__((fastcall)) float do_note_instr(note_instr *n){
  float e;
  float out = 0.f;
  osc *o = n->o;
  if(n->used){
    if((e = do_adsr(&(n->env)))<=0.f){
      --n->used;
    } else {
      register char i=3;
      do { out += do_osc(o++); } while(--i);
      out *= e * n->amp;
      n->cutoff = calc_freq(16.f, 1.059463094f, n->instr->cutoff);
      n->f = 1.5f * sin4k(3.141592f * n->cutoff / SAMPLERATE_F);
      n->res = (float)(n->instr->res) / 127.f;
      n->low += n->f * n->band;
      float high = n->res * (out - n->band) - n->low;
      n->band += n->f * high;
      out = n->low;
    }
  }
  return out;
}

note_instr note[32];

float reverb[SAMPLERATE];
int reverb_pos = 0;
int reverb_max = 0;
float reverb_level = 0.F;

void init_synth(){
  register int i=31;
  do { note[i].used = 0; } while(i--);
}

void update_instr(instrument *instr){
  reverb_max = instr->reverb_time * SAMPLERATE / 127;
  if(reverb_pos >= reverb_max) reverb_pos = 0;
  reverb_level = instr->reverb_level / 127.F;
}

void create_note(int freq, int amp, instrument *instr){
  int i=31;
  note_instr *n = note;
  do {
    if(!n->used){
      create_note_instr(n++,instr,freq,amp);
      i=0;
    }
    n++;
  } while(i--);
}

void release_note(int freq, int amp, instrument *instr){
  register int i=31;
  register note_instr *n = note;
  do {
    if(n->used){
      if((n->instr == instr) && (n->freq == freq)){
        n->env.on = 0;
        //i=0;
      }
    }
    n++;
  } while(i--);
}

void render_synth(short *audio_buffer, int len){
  int i;
  float out;
  register short *sp = audio_buffer;
  short sh;

  do {
    i=31; out=0.f;
    do {
      out += do_note_instr(note+i);
    } while(i--);
    out += reverb[reverb_pos] * reverb_level;
    reverb[reverb_pos] = out;
    if((++reverb_pos)>= reverb_max) reverb_pos=0;
    sh = *(sp++) = (short)(out * 32767.f);
    *(sp++) = sh;
  } while(--len);
}
