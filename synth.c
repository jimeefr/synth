#include "synth.h"

static int my_seed = 32627;

float my_rand()
{
  float res;
  my_seed *= 16807;
  *((unsigned int *) &res) = ( ((unsigned int)my_seed)>>9 ) | 0x40000000;
  return( res-3.0f );
}

void create_osc(osc *o, osc_type t, float f, float a){
  o->type = t;
  o->freq = f;
  o->amp  = a;
  o->ph   = 0.f;
  o->iph  = f / SAMPLERATE_F;
}

float do_osc(osc *o){
  float out;
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
  return out;
}

void create_adsr(enveloppe *e, float a, float d, float s, float r){
  e->v = 0.f;
  e->t = 0.f;
  e->a = a;
  e->d = d;
  e->s = s;
  e->r = r;
  e->ad = a+d;
  e->da = 1.f / (a * SAMPLERATE_F);
  e->dd = (1.f - s) / (d * SAMPLERATE_F);
  e->dr = s / (r * SAMPLERATE_F);
  e->on = 1;
}

float do_adsr(enveloppe *e){
  if(e->on){
    if(e->t < e->a) e->v += e->da;
    else if((e->t - e->a) < DT) e->v = 1.f;
    else if(e->t < e->ad) e->v -= e->dd;
    else e->v = e->s;
  }
  else e->v -= e->dr;
  e->t += DT;
  return e->v;
}

void create_note_instr(note_instr *n, instrument *i, float f, float a){
  int os;
  n->used = 1;
  n->instr = i;
  create_adsr(&(n->env), i->a, i->d, i->s, i->r);
  n->freq = f;
  n->amp = a;
  for(os=0; os<8; os++) create_osc(n->o+os, i->type[os], i->freq[os]*f, i->amp[os]);
}

float do_note_instr(note_instr *n){
  int i;
  float e;
  float out = 0.f;
  if(n->used){
    e = do_adsr(&(n->env));
    if(e<0.f){
      n->used = 0;
    } else {
      for(i=0; i<8; i++) out += do_osc(n->o+i);
      out *= e * n->amp;
    }
  }
  return out;
}

void init_synth(synth *s){
  int i=31;
  for(i=0; i<32; i++) s->note[i].used = 0;
}

void render_synth(synth *s, float *l, float *r){
  int i;
  float out;

  *l = 0.f;
  *r = 0.f;
  for(i=0; i<32; i++){
    out = do_note_instr(s->note+i);
    *l += out;
    *r += out;
  }
}
