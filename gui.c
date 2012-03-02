#include "gui.h"
#include "synth.h"
#include <SDL/SDL.h>
#include <GL/gl.h>

typedef struct _ruler {
  short x1,y1,x2,y2;
  char min, max;
  char *value;
} ruler;

instrument instr1 = {
 3, 16 , 127, 3, /* a,d,s,r */
 { OSC_SAW, OSC_SQUARE, OSC_SAW }, /* osc_type */
 { 64, 64, 76 }, /* transpose */
 { 64, 64, 64 }, /* finetune */
 { 96, 96, 96 }, /* amplitude */
 { 15, 15, 15 }, /* unison */
 { 32, 48, 32 }, /* dispersion */
 82, 112,        /* cutoff, res */
 96, 16,         /* reverb_level, reverb_time */
};

instrument instr2 = {
 3, 16 , 127, 3, /* a,d,s,r */
 { OSC_SAW, OSC_SQUARE, OSC_SAW }, /* osc_type */
 { 64, 64, 76 }, /* transpose */
 { 64, 64, 64 }, /* finetune */
 { 96, 96, 96 }, /* amplitude */
 { 15, 15, 15 }, /* unison */
 { 32, 48, 32 }, /* dispersion */
 82, 112,        /* cutoff, res */
 96, 16,         /* reverb_level, reverb_time */
};

instrument *instr = &instr1;

int fini = 0;
char octave = 4;
char keymap[] = 
  "wsxdcvgbhnj,;l:m!"
  "a\xe9z\"er(t-y\xe8ui\xe7o\xe0p^=$";
char notetable[] = {
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,
  12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
};

__attribute__((always_inline)) void usleep(int t){
  struct timespec ts;
  ts.tv_sec = t / 1000000;
  ts.tv_nsec = (t % 1000000) * 1000;
  asm("mov $162,%%eax\n"
      "lea %0,%%ebx\n"
      "xor %%ecx,%%ecx\n"
      "int $0x80\n"
      : : "m"(ts) : "eax","ebx","ecx");
}

__attribute__((always_inline)) static char *my_strchr(char *p,char c){
  while(*p){
    if(*p == c) return p;
    else p++;
  }
  return 0;
}

__attribute__((always_inline)) static void move_ruler(ruler *rul, int x, int y){
  if(!rul->value) return;
  if((x >= rul->x1) &&
     (x <= rul->x2) &&
     (y >= rul->y1) &&
     (y <= rul->y2)){
    int dx = rul->x2 - rul->x1;
    *(rul->value) = ((x - rul->x1) * (rul->max - rul->min) + (dx >> 1)) / dx + rul->min;
    update_instr(instr);
  }
}

#define RULERS 26

void move_rulers(ruler *rul, int x, int y){
  int i=RULERS;
  do {
    move_ruler(rul++,x,y);
  } while(--i);
}

static void draw_ruler(ruler *rul){
  glColor3ub(255,255,255);
  glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
  glRecti(rul->x1-1,rul->y2+3,rul->x2+3,rul->y1-1);
  glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  if(rul->value) glRecti(rul->x1,rul->y1,(*(rul->value)-rul->min) * (rul->x2-rul->x1) / (rul->max-rul->min) + rul->x1+1,rul->y2+1);
}

static void draw_rulers(ruler *rul){
  int i=RULERS;
  do {
    draw_ruler(rul++);
  } while(--i);
}

/*
static void create_ruler(ruler *rul,int x1, int y1, int x2, int y2, char min, char max, char *val){
  rul->x1=x1+2;
  rul->x2=x2-2;
  rul->y1=y1+2;
  rul->y2=y2-2;
  rul->min=min;
  rul->max=max;
  rul->value=val;
}
*/

ruler R[RULERS] = {
  {16,16,143,20,0,127,NULL},
  {16,26,143,30,0,127,NULL},
  {16,46,143,50,0,3,NULL},
  {16,56,143,60,0,127,NULL},
  {16,66,143,70,0,127,NULL},
  {16,76,143,80,0,127,NULL},
  {16,140,143,144,0,3,NULL},
  {16,150,143,154,0,127,NULL},
  {16,160,143,164,0,127,NULL},
  {16,170,143,174,0,127,NULL},
  {16,210,143,214,0,127,NULL},
  {16,220,143,224,0,127,NULL},
  {176,46,303,50,0,3,NULL},
  {176,56,303,60,0,127,NULL},
  {176,66,303,70,0,127,NULL},
  {176,76,303,80,0,127,NULL},
  {176,16,303,20,0,127,NULL},
  {176,26,303,30,0,127,NULL},
  {176,210,303,214,0,127,NULL},
  {176,220,303,224,0,127,NULL},
  {16,86,143,90,0,15,NULL},
  {16,180,143,184,0,15,NULL},
  {176,86,303,90,0,15,NULL},
  {16,96,143,100,0,127,NULL},
  {16,190,143,194,0,127,NULL},
  {176,96,303,100,0,127,NULL}
};

static void bind_rulers(ruler r[], instrument *i){
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
  r[20].value = &(i->unison[0]);
  r[21].value = &(i->unison[2]);
  r[22].value = &(i->unison[1]);
  r[23].value = &(i->disper[0]);
  r[24].value = &(i->disper[2]);
  r[25].value = &(i->disper[1]);
  update_instr(instr);
}

static void draw_gui(){
  int sc_p = scope_pos, i;
  glClear(GL_COLOR_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.,320.,240.,0.,-1.,1.);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  draw_rulers(R);
  glPointSize(4.f);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_POINTS);
  glColor4ub(255,0,0,127);
  i=320;
  while(--i >= 0){
    if(--sc_p < 0) sc_p += SAMPLERATE;
    glVertex2i(i,120 - (int)(scope[sc_p] * 120.f));
  } 
  glEnd();
  glDisable(GL_BLEND);
  SDL_GL_SwapBuffers();
}

void gui_init(){
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_SetVideoMode(320,240,32,SDL_OPENGL);
  glViewport(0,0,320,240);
  bind_rulers(R,instr);
  init_synth();
}

static void select_instr(instrument *i){
	instr = i;
	bind_rulers(R,i);
	update_instr(i);
}

__attribute__((always_inline)) static void gui_check_event(){
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
      else if(event.button.button == 4) { if(instr->cutoff<127) instr->cutoff++; }
      else if(event.button.button == 6) { if(instr->res>0) instr->res--; }
      else if(event.button.button == 7) { if(instr->res<127) instr->res++; }
    }
  }
}

void gui_mainloop(){
  while(!fini){
    draw_gui();
    gui_check_event();
    usleep(20000);
  }
}
