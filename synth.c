#include <stdio.h>
#include "synth.h"

static unsigned int my_seed = 32627;

__attribute__((fastcall)) static float my_rand()
{
  union { float f; unsigned int i;} res;

  my_seed *= 16807;
  res.i = ( my_seed >> 9 ) | 0x40000000;
  return (res.f - 3.0f);
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
  float out=0.f;
  if(o->freq != 0.f){
    if((o->ph += o->iph) >=1.f) o->ph -= 1.f;
    switch(o->type){
      case OSC_TRIANGLE :
        out = (o->ph < .5f) ? (4.f * o->ph - 1.f) : (-4.f * o->ph + 3.f);
        break;
      case OSC_SAW :
        out = 2.f * o->ph - 1.f;
        break;
      case OSC_SQUARE :
        out = (o->ph < .5f) ? -1.f : 1.f;
        break;
      case OSC_NOISE :
        out = my_rand();
        break;
    }
    out *= o->amp;
  }
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
  e->on = 1;
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

__attribute__((fastcall)) void create_note_instr(note_instr *n, instrument *i, float f, float a){
  int os;
  n->used = 1;
  n->instr = i;
  create_adsr(&(n->env), i->a, i->d, i->s, i->r);
  n->freq = f;
  n->amp = a;
  os = 2; do create_osc(n->o+os, i->type[os], 
                        calc_freq(0.024803141f,1.059463094f,i->freqt[os]) * 
                        calc_freq(0.971531941f,1.000451664f,i->freqf[os]) * f,
                        (float)(i->amp[os]) / 127.f); while(os--);
  n->cutoff = calc_freq(16.f, 1.059463094f, i->cutoff);
  n->res = (float)(i->res) / 127.f;
  n->f = 1.5f * sin4k(3.141592f * n->cutoff / SAMPLERATE_F);
  n->low = n->band = 0.f;
}

__attribute__((fastcall)) float do_note_instr(note_instr *n){
  float e;
  float out = 0.f;
  osc *o = n->o;
  if(n->used){
    e = do_adsr(&(n->env));
    if(e<=0.f){
      n->used = 0;
    } else {
      out += do_osc(&o[0]);
      out += do_osc(&o[1]);
      out += do_osc(&o[2]);
      out *= e * n->amp;
      n->low += n->f * n->band;
      float high = n->res * (out - n->band) - n->low;
      n->band += n->f * high;
      out = n->low;
    }
  }
  return out;
}

void init_synth(synth *s){
  int i=31;
  do { s->note[i].used = 0; } while(i--);
}

void create_note(synth *syn, float freq, float amp, instrument *instr){
  int i=31;
  do {
    if(!syn->note[i].used){
      create_note_instr(syn->note+i,instr,freq,amp);
      i=0;
    }
  } while(i--);
}

void release_note(synth *syn, float freq, float amp, instrument *instr){
  int i=31;
  do {
    if(syn->note[i].used){
      if((syn->note[i].instr == instr) && (syn->note[i].freq == freq) && syn->note[i].env.on){
        syn->note[i].env.on = 0;
        //i=0;
      }
    }
  } while(i--);
}

void render_synth(synth *s, short *audio_buffer, int len){
  int i;
  float out;
  note_instr *p;
  register short *sp = audio_buffer;
  short sh;

  do {
    i=32; out=0.f; p=s->note;
    do {
      out += do_note_instr(p++);
    } while(--i);
    sh = *(sp++) = (short)(out * 32767.f);
    *(sp++) = sh;
  } while(--len);
}
