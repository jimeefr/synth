#include "gui.h"
#include "synth.h"
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

instrument instr1 = {
 5, 16 , 127, 32, /* a,d,s,r */
 { OSC_SAW, OSC_SAW, OSC_SAW }, /* osc_type */
 { 64, 64, 64 }, /* transpose */
 { 64, 76, 54 }, /* finetune */
 { 96, 96, 96 }, /* amplitude */
 82, 100,         /* cutoff, res */
 96, 64,         /* reverb_level, reverb_time */
};

instrument instr2 = {
 19, 59 , 127, 99, /* a,d,s,r */
 { OSC_SAW, OSC_SAW, OSC_SAW }, /* osc_type */
 { 64, 64, 64 }, /* transpose */
 { 64, 76, 54 }, /* finetune */
 { 64, 64, 64 }, /* amplitude */
 95, 100,         /* cutoff, res */
 30, 100,         /* reverb_level, reverb_time */
};

instrument *instr = &instr1;

__attribute__((fastcall)) static char *my_strchr(char *p,char c){
  while(*p){
    if(*p == c) return p;
    else p++;
  }
  return 0;
}

typedef struct _ruler {
  short x1,y1,x2,y2;
  char min, max;
  char *value;
} ruler;

__attribute__((fastcall)) static void rect(int x1,int y1,int x2,int y2){
  glVertex2i(x1,y1);
  glVertex2i(x1,y2);
  glVertex2i(x2,y2);
  glVertex2i(x2,y1);
}

void draw_ruler(ruler *rul){
  int x1,y1,x2,y2;
  x1 = rul->x1; x2 = rul->x2+1;
  y1 = rul->y1; y2 = rul->y2+1;
  glBegin(GL_QUADS);
  glColor3ub(255,255,255);
  rect(x1++,y1++,x2--,y2--);
  glColor3ub(0,0,0);
  rect(x1++,y1++,x2,y2--);
  glColor3ub(255,255,255);
  if(rul->value) rect(x1,y1,(*(rul->value)-rul->min+1) * (x2-x1-1) / (rul->max-rul->min+1) + x1,y2);
  glEnd();
}

__attribute__((fastcall)) static void move_ruler(ruler *rul, int x, int y){
  if(!rul->value) return;
  if((x > rul->x1+1) &&
     (x < rul->x2-1) &&
     (y > rul->y1+1) &&
     (y < rul->y2-1)){
    *(rul->value) = (x - rul->x1 - 2) * (rul->max - rul->min + 1) / (rul->x2 - rul->x1 - 3) - rul->min;
    update_instr(instr);
  }
}

#define RULERS 20

void move_rulers(ruler *rul, int x, int y){
  int i=RULERS;
  do {
    move_ruler(rul++,x,y);
  } while(--i);
}

static void draw_rulers(ruler *rul){
  int i=RULERS;
  do {
    draw_ruler(rul++);
  } while(--i);
}

/*
static void create_ruler(ruler *rul,int x1, int y1, int x2, int y2, char min, char max, char *val){
  rul->x1=x1;
  rul->x2=x2;
  rul->y1=y1;
  rul->y2=y2;
  rul->min=min;
  rul->max=max;
  rul->value=val;
}
*/

ruler R[RULERS] = {
  {14,10,145,18,0,127,NULL},
  {14,20,145,28,0,127,NULL},
  {14,40,145,48,0,3,NULL},
  {14,50,145,58,0,127,NULL},
  {14,60,145,68,0,127,NULL},
  {14,70,145,78,0,127,NULL},
  {14,90,145,98,0,3,NULL},
  {14,100,145,108,0,127,NULL},
  {14,110,145,118,0,127,NULL},
  {14,120,145,128,0,127,NULL},
  {14,140,145,148,0,127,NULL},
  {14,150,145,158,0,127,NULL},
  {174,40,305,48,0,3,NULL},
  {174,50,305,58,0,127,NULL},
  {174,60,305,68,0,127,NULL},
  {174,70,305,78,0,127,NULL},
  {174,10,305,18,0,127,NULL},
  {174,20,305,28,0,127,NULL},
  {174,140,305,148,0,127,NULL},
  {174,150,305,158,0,127,NULL}
};

void bind_rulers(ruler r[], instrument *i){
  r[0].value = &(i->a);
  r[1].value = &(i->s);
  r[2].value = (char *)&(i->type[0]);
  r[3].value = &(i->freqt[0]);
  r[4].value = &(i->freqf[0]);
  r[5].value = &(i->amp[0]);
  r[6].value = (char *)&(i->type[2]);
  r[7].value = &(i->freqt[2]);
  r[8].value = &(i->freqf[2]);
  r[9].value = &(i->amp[2]);
  r[10].value = &(i->cutoff);
  r[11].value = &(i->res);
  r[12].value = (char *)&(i->type[1]);
  r[13].value = &(i->freqt[1]);
  r[14].value = &(i->freqf[1]);
  r[15].value = &(i->amp[1]);
  r[16].value = &(i->d);
  r[17].value = &(i->r);
  r[18].value = &(i->reverb_level);
  r[19].value = &(i->reverb_time);
}

static void draw_gui(){
  int sc_p = scope_pos, i;
  glClear(GL_COLOR_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.,320.,240.,0.);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  draw_rulers(R);
  glBegin(GL_POINT);
  i=320;
  while(--i >= 0){
    if(--sc_p < 0) sc_p += SAMPLERATE;
    glVertex2i(i,(int)(scope[sc_p] * 100.f) + 200);
  } 
  glEnd();
  SDL_GL_SwapBuffers();
}

void gui_init(){
  SDL_SetVideoMode(320,240,32,SDL_OPENGL);
  glViewport(0,0,320,240);
  bind_rulers(R,instr);
  init_synth();
}

int fini = 0;
char octave = 4;
char keymap[] = 
  "wsxdcvgbhnj,;l:m!"
  "a\xe9z\"er(t-y\xe8ui\xe7o\xe0p^=$";
char notetable[] = {
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,
  12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
};

void select_instr(instrument *i){
	instr = i;
	bind_rulers(R,i);
	update_instr(i);
}

static void gui_check_event(){
  SDL_Event event;
  char *p;

  while(SDL_PollEvent(&event)) {
    if(event.type == SDL_KEYDOWN){
      if(event.key.keysym.sym == SDLK_ESCAPE) fini=1;
      else if(event.key.keysym.sym == SDLK_F1) octave--;
      else if(event.key.keysym.sym == SDLK_F2) octave++;
      else if(event.key.keysym.sym == SDLK_F3) select_instr(&instr1);
      else if(event.key.keysym.sym == SDLK_F4) select_instr(&instr2);
      else if(event.key.keysym.sym == SDLK_LESS) init_synth();
      else if((p=my_strchr(keymap,event.key.keysym.sym))) 
        create_note(octave*12+notetable[p-keymap],20,instr);
    } else if(event.type == SDL_KEYUP){
      if((p=my_strchr(keymap,event.key.keysym.sym))){
        release_note(octave*12 + notetable[p-keymap],20,instr);
      }
    } else if(event.type == SDL_MOUSEMOTION){
      if(event.motion.state & SDL_BUTTON(1)) move_rulers(R,event.motion.x,event.motion.y);
    } else if(event.type == SDL_MOUSEBUTTONDOWN){
      if(event.button.button == 1) move_rulers(R,event.button.x,event.button.y);
      else if(event.button.button == 5) { if(instr->cutoff>0) instr->cutoff--; }
      else if(event.button.button == 4) { if(instr->cutoff<119) instr->cutoff++; }
      else if(event.button.button == 6) { if(instr->res>0) instr->res--; }
      else if(event.button.button == 7) { if(instr->res<127) instr->res++; }
    }
  }
}

void gui_mainloop(){
  while(!fini){
    draw_gui();
    gui_check_event();
  }
}
