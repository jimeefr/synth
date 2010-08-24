#include "gui.h"
#include "synth.h"
#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

char keymap[] = "wsxdcvgbhnj,;";
float freqtable[] = {
  261.6f,277.2f,293.7f,311.1f,329.6f,349.2f,370.f,392.f,415.3f,440.f,466.2f,493.9f,523.3f
};
instrument instr = {
 19, 59 , 127, 99, /* a,d,s,r */
 { OSC_TRIANGLE, OSC_SAW, OSC_SQUARE }, /* 3 osc types */
 { 64, 64, 64 }, /* 3 transpose */
 { 64, 66, 62 }, /* 3 finetune */
 { 127,100,100 },/* 3 amps */
 100, 100         /* cutoff, res */
};

char *my_strchr(char *s,char c){
  char *p = s;
  while(*p){
    if(*p == c) return p;
    else p++;
  }
  return 0;
}

typedef struct _ruler {
  int x1,y1,x2,y2;
  char min, max;
  char *value;
} ruler;

void draw_ruler(ruler *rul){
  int pos = (*(rul->value)-rul->min+1) * (rul->x2-rul->x1-3) / (rul->max-rul->min+1) + rul->x1+2;
  glColor3ub(255,255,255);
  glBegin(GL_QUADS);
  glVertex2i(rul->x1,rul->y1);
  glVertex2i(rul->x1,rul->y2+1);
  glVertex2i(rul->x2+1,rul->y2+1);
  glVertex2i(rul->x2+1,rul->y1);
  glColor3ub(0,0,0);
  glVertex2i(rul->x1+1,rul->y1+1);
  glVertex2i(rul->x1+1,rul->y2);
  glVertex2i(rul->x2,rul->y2);
  glVertex2i(rul->x2,rul->y1+1);
  glColor3ub(255,255,255);
  glVertex2i(rul->x1+2,rul->y1+2);
  glVertex2i(rul->x1+2,rul->y2-1);
  glVertex2i(pos,rul->y2-1);
  glVertex2i(pos,rul->y1+2);
  glEnd();
}

void move_ruler(ruler *rul, int x, int y){
  if((x > rul->x1+1) &&
     (x < rul->x2-1) &&
     (y > rul->y1+1) &&
     (y < rul->y2-1))
    *(rul->value) = (x - rul->x1 - 2) * (rul->max - rul->min + 1) / (rul->x2 - rul->x1 - 3) - rul->min;
}

#define RULERS 18

void move_rulers(ruler *rul, int x, int y){
  int i=RULERS-1;
  do {
    if(rul[i].value) move_ruler(rul+i,x,y);
  } while(i--);
}

void draw_rulers(ruler *rul){
  int i=RULERS-1;
  do {
    if(rul[i].value) draw_ruler(rul+i);
  } while(i--);
}

void create_ruler(ruler *rul,int x1, int y1, int x2, int y2, char min, char max, char *val){
  rul->x1=x1;
  rul->x2=x2;
  rul->y1=y1;
  rul->y2=y2;
  rul->min=min;
  rul->max=max;
  rul->value=val;
}

ruler R[RULERS];

void draw_gui(){
  glClear(GL_COLOR_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.,320.,240.,0.);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  draw_rulers(R);
  SDL_GL_SwapBuffers();
}

static char toto;
void gui_init(){
  int i=0;
  SDL_SetVideoMode(320,240,32,SDL_OPENGL);
  glViewport(0,0,320,240);
  init_synth();
  create_ruler(R+(i++),10,10,141,16,0,127,&(instr.a));
  create_ruler(R+(i++),160,10,291,16,0,127,&(instr.d));
  create_ruler(R+(i++),10,20,141,26,0,127,&(instr.s));
  create_ruler(R+(i++),160,20,291,26,0,127,&(instr.r));
  create_ruler(R+(i++),10,40,141,46,0,3,(char *)&(instr.type[0]));
  create_ruler(R+(i++),10,50,141,56,0,127,&(instr.freqt[0]));
  create_ruler(R+(i++),10,60,141,66,0,127,&(instr.freqf[0]));
  create_ruler(R+(i++),10,70,141,76,0,127,&(instr.amp[0]));
  create_ruler(R+(i++),160,40,291,46,0,3,(char *)&(instr.type[1]));
  create_ruler(R+(i++),160,50,291,56,0,127,&(instr.freqt[1]));
  create_ruler(R+(i++),160,60,291,66,0,127,&(instr.freqf[1]));
  create_ruler(R+(i++),160,70,291,76,0,127,&(instr.amp[1]));
  create_ruler(R+(i++),10,90,141,96,0,3,(char *)&(instr.type[2]));
  create_ruler(R+(i++),10,100,141,106,0,127,&(instr.freqt[2]));
  create_ruler(R+(i++),10,110,141,116,0,127,&(instr.freqf[2]));
  create_ruler(R+(i++),10,120,141,126,0,127,&(instr.amp[2]));
  create_ruler(R+(i++),10,140,141,146,0,127,&(instr.cutoff));
  create_ruler(R+(i++),10,150,141,156,0,127,&(instr.res));
}

static int fini = 0;
static float octave = 1.f;
static int current_osc=0;

void gui_check_event(){
  SDL_Event event;
  char *p;

  while(SDL_PollEvent(&event)) {
    switch(event.type){
      case SDL_KEYDOWN :
        switch(event.key.keysym.sym){
          case SDLK_ESCAPE : fini=1 ; break;
          case SDLK_KP1 : current_osc = 0; break;
          case SDLK_KP2 : current_osc = 1; break;
          case SDLK_KP3 : current_osc = 2; break;
          case SDLK_KP4 : if(instr.freqt[current_osc] > 0 ) instr.freqt[current_osc]-=1; break; 
          case SDLK_KP5 : if(instr.freqf[current_osc] > 0 ) instr.freqf[current_osc]-=1; break; 
          case SDLK_KP6 : if(instr.amp[current_osc] > 0 ) instr.amp[current_osc]-=1; break; 
          case SDLK_KP7 : if(instr.freqt[current_osc] < 127 ) instr.freqt[current_osc]+=1; break; 
          case SDLK_KP8 : if(instr.freqf[current_osc] < 127 ) instr.freqf[current_osc]+=1; break; 
          case SDLK_KP9 : if(instr.amp[current_osc] < 127 ) instr.amp[current_osc]+=1; break; 
          case SDLK_KP_DIVIDE   : instr.type[current_osc] = OSC_TRIANGLE; break;
          case SDLK_KP_MULTIPLY : instr.type[current_osc] = OSC_SAW; break;
          case SDLK_KP_MINUS    : instr.type[current_osc] = OSC_SQUARE; break;
          case SDLK_KP_PLUS     : instr.type[current_osc] = OSC_NOISE; break;
          case 'o' : if(instr.cutoff>0) instr.cutoff--; break;
          case 'p' : if(instr.cutoff<127) instr.cutoff++; break;
          case 'l' : if(instr.res>0) instr.res--; break;
          case 'm' : if(instr.res<127) instr.res++; break;
          case 'a' : octave /= 2.f; break;
          case 'z'   : octave *= 2.f; break;
          default :
            if((p=my_strchr(keymap,event.key.keysym.sym))){
              int note = p-keymap;
              float freq = octave * freqtable[note];
              float amp = .2f;
              create_note(freq,amp,&instr);
            }
        }
        break;
      case SDL_KEYUP :
        switch(event.key.keysym.sym){
          default :
            if((p=my_strchr(keymap,event.key.keysym.sym))){
              int note = p-keymap;
              float freq = octave * freqtable[note];
              float amp = 0.2f;
              release_note(freq,amp,&instr);
            }
        }
        break;
      case SDL_MOUSEMOTION :
        if(event.motion.state & SDL_BUTTON(1)) move_rulers(R,event.motion.x,event.motion.y);
        break;
      case SDL_MOUSEBUTTONDOWN :
        if(event.button.button == 1) move_rulers(R,event.button.x,event.button.y);
        break;
    }
  }
}

void gui_mainloop(){
  while(!fini){
    draw_gui();
    gui_check_event();
    //usleep(1000);
  }
}
