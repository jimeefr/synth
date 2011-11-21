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

typedef struct _osc_def {
  osc_type type;
  char freqt; /* transpose [0;127] -> [-64;63] */
  char freqf; /* finetune [0;128[ -> [-1/4t;+1/4t[ */
  char amp;   /* amplitude [0;127] -> [0.f;1.f](log) */
} osc_def;

typedef struct _instrument {
  char a,d,s,r;
  osc_def o[3];  /* 3 osc definitions */
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
  osc o[3];
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
