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
  float (*osc)(float);
  float ph;         /* phase [0;1[ */
  float iph;        /* incr_pĥase = freq / SAMPLERATE */
  float amp;        /* amplitude [0;1[ */
} osc;

typedef struct _enveloppe {
  float a,s;
  float ad,da,dd,dr;
  float v,t;
  int on;
} enveloppe;

typedef struct _instrument {
  char a,d,s,r;
  osc_type type[3];
  char freqt[3]; /* transpose [0;127] -> [-64;63] */
  char freqf[3]; /* finetune [0;128[ -> [-1/4t;+1/4t[ */
  char amp[3];   /* amplitude [0;127] -> [0.f;1.f](log) */
  char unison[3];
  char disper[3];
  char cutoff;   /* [0;127] -> [20;SAMPLERATE/4](log) */
  char res;      /* [0;128[ -> [0.f;1.f[ */
  char reverb_level;
  char reverb_time;
} instrument;

typedef struct _note_instr {
  int used;
  instrument *instr;
  enveloppe env;
  int freq;
  float amp;
  int nos;
  osc o[48];
  float cutoff,res;
  float f,low,band; /* filter */
} note_instr;

extern float scope[];
extern int scope_pos;

void init_synth();
void update_instr(instrument *instr);
void create_note(int freq, int amp, instrument *instr);
void release_note(int freq, int amp, instrument *instr);
void render_synth(short *audio_buffer, int len);

#endif
