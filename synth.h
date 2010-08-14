#ifndef SYNTH_H
#define SYNTH_H

#define SAMPLERATE 44100
#define SAMPLERATE_F 44100.f
#define DT (1.f / SAMPLERATE_F)

typedef enum {
  OSC_TRIANGLE,
  OSC_SAW,
  OSC_SQUARE,
  OSC_NOISE
} osc_type;

typedef struct _osc {
  osc_type type;
  float ph;         /* phase [0;1[ */
  float freq;
  float iph;        /* incr_pĥase = freq / SAMPLERATE */
  float amp;        /* amplitude [0;1[ */
} osc;

typedef struct _enveloppe {
  float a,d,s,r;
  float ad,da,dd,dr;
  float v,t;
  int on;
} enveloppe;

typedef struct _instrument {
  float a,d,s,r;
  /* 3 osc definitions */
  osc_type type[3];
  char freqt[3]; /* transpose [0;127] -> [-64;63] */
  char freqf[3]; /* finetune [0;128[ -> [-1/4t;+1/4t[ */
  char amp[3];   /* amplitude [0;127] -> [0.f;1.f](log) */
  char cutoff;   /* [0;127] -> [20;SAMPLERATE/4](log) */
  char res;      /* [0;128[ -> [0.f;1.f[ */
} instrument;

typedef struct _note_instr {
  int used;
  instrument *instr;
  enveloppe env;
  float freq;
  float amp;
  osc o[3];
  float cutoff,res;
  float f,low,band; /* filter */
} note_instr;

typedef struct _synth {
  note_instr note[32];
} synth;

void init_synth(synth *s);
void create_note(synth *syn, float freq, float amp, instrument *instr);
void release_note(synth *syn, float freq, float amp, instrument *instr);
void render_synth(synth *s, short *audio_buffer, int len);

#endif
