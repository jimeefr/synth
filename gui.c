#include "gui.h"
#include "synth.h"
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

instrument instr = {
 19, 59 , 127, 99, /* a,d,s,r */
 { { OSC_SAW,64,64,64 },
   { OSC_SAW,64,76,64 },
   { OSC_SAW,64,54,64 } },
 95, 100,         /* cutoff, res */
 30, 100,         /* reverb_level, reverb_time */
};

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
  rect(x1,y1,(*(rul->value)-rul->min+1) * (x2-x1-1) / (rul->max-rul->min+1) + x1,y2);
  glEnd();
}

__attribute__((fastcall)) static void move_ruler(ruler *rul, int x, int y){
  if((x > rul->x1+1) &&
     (x < rul->x2-1) &&
     (y > rul->y1+1) &&
     (y < rul->y2-1)){
    *(rul->value) = (x - rul->x1 - 2) * (rul->max - rul->min + 1) / (rul->x2 - rul->x1 - 3) - rul->min;
    update_instr(&instr);
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
  {10,10,141,18,0,127,&(instr.a)},
  {10,20,141,28,0,127,&(instr.s)},
  {10,40,141,48,0,3,(char *)&(instr.o[0].type)},
  {10,50,141,58,0,127,&(instr.o[0].freqt)},
  {10,60,141,68,0,127,&(instr.o[0].freqf)},
  {10,70,141,78,0,127,&(instr.o[0].amp)},
  {10,90,141,98,0,3,(char *)&(instr.o[2].type)},
  {10,100,141,108,0,127,&(instr.o[2].freqt)},
  {10,110,141,118,0,127,&(instr.o[2].freqf)},
  {10,120,141,128,0,127,&(instr.o[2].amp)},
  {10,140,141,148,0,127,&(instr.cutoff)},
  {10,150,141,158,0,127,&(instr.res)},
  {160,40,291,48,0,3,(char *)&(instr.o[1].type)},
  {160,50,291,58,0,127,&(instr.o[1].freqt)},
  {160,60,291,68,0,127,&(instr.o[1].freqf)},
  {160,70,291,78,0,127,&(instr.o[1].amp)},
  {160,10,291,18,0,127,&(instr.d)},
  {160,20,291,28,0,127,&(instr.r)},
  {160,140,291,148,0,127,&(instr.reverb_level)},
  {160,150,291,158,0,127,&(instr.reverb_time)}
};

static void draw_gui(){
  glClear(GL_COLOR_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.,320.,240.,0.);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  draw_rulers(R);
  SDL_GL_SwapBuffers();
}

void gui_init(){
  SDL_SetVideoMode(320,240,32,SDL_OPENGL);
  glViewport(0,0,320,240);
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

static void gui_check_event(){
  SDL_Event event;
  char *p;

  while(SDL_PollEvent(&event)) {
    if(event.type == SDL_KEYDOWN){
      if(event.key.keysym.sym == SDLK_ESCAPE) fini=1;
      else if(event.key.keysym.sym == SDLK_F1) octave--;
      else if(event.key.keysym.sym == SDLK_F2) octave++;
      else if((p=my_strchr(keymap,event.key.keysym.sym))) 
        create_note(octave*12+notetable[p-keymap],20,&instr);
    } else if(event.type == SDL_KEYUP){
      if((p=my_strchr(keymap,event.key.keysym.sym))){
        release_note(octave*12 + notetable[p-keymap],20,&instr);
      }
    } else if(event.type == SDL_MOUSEMOTION){
      if(event.motion.state & SDL_BUTTON(1)) move_rulers(R,event.motion.x,event.motion.y);
    } else if(event.type == SDL_MOUSEBUTTONDOWN){
      if(event.button.button == 1) move_rulers(R,event.button.x,event.button.y);
      else if(event.button.button == 5) { if(instr.cutoff>0) instr.cutoff--; }
      else if(event.button.button == 4) { if(instr.cutoff<119) instr.cutoff++; }
      else if(event.button.button == 6) { if(instr.res>0) instr.res--; }
      else if(event.button.button == 7) { if(instr.res<127) instr.res++; }
    }
  }
}

void gui_mainloop(){
  while(!fini){
    draw_gui();
    gui_check_event();
  }
}
