#include "synth.h"
#include <SDL/SDL.h>

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

FILE *fichier = 0;

void fetch_audio_buffer(void *userdata, Uint8 *stream, int len){
  render_synth((synth *)userdata,(short *)stream,len>>2);
  //if(fichier)fwrite(stream,1,len,fichier);
}

#ifdef DEBUG
inline void echo(char *s,int l){
  asm("mov $4,%%eax\n"
      "mov $1,%%ebx\n"
      "int $0x80\n"
      : : "c"(s), "d"(l) : "eax","ebx");
}
#endif

//void usleep(int t){
//  struct timespec ts;
//  ts.tv_sec = t / 1000000;
//  ts.tv_nsec = (t % 1000000) * 1000;
//  asm("mov $162,%%eax\n"
//      "xor %%ecx,%%ecx\n"
//      "int $0x80\n"
//      : : "b"(&ts) : "eax","ecx");
//}

char *my_strchr(char *s,char c){
  char *p = s;
  while(*p){
    if(*p == c) return p;
    else p++;
  }
  return 0;
}

void _start(){
  synth syn;
  SDL_AudioSpec desired, obtained;
  SDL_Event event;
  int fini = 0;
  char *p;
  float octave = 1.f;
  int cutoff=instr.cutoff;
  int res=instr.res;
  int osc=1;

  desired.freq=SAMPLERATE;
  desired.format=AUDIO_S16;
  desired.channels=2;
  desired.samples=1024;
  desired.callback=fetch_audio_buffer;
  desired.userdata = &syn;
  //SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO);
  if(!SDL_OpenAudio(&desired, &obtained)){
    SDL_SetVideoMode(320,240,32,0);
  
    init_synth(&syn);

    SDL_PauseAudio(0);
    while(!fini){
      while(SDL_PollEvent(&event)) {
        switch(event.type){
          case SDL_KEYDOWN :
            switch(event.key.keysym.sym){
              case SDLK_ESCAPE : fini=1 ; break;
              case SDLK_KP1 : osc = 0; break;
              case SDLK_KP2 : osc = 1; break;
              case SDLK_KP3 : osc = 2; break;
              case SDLK_KP4 : if(instr.freqt[osc] > 0 ) instr.freqt[osc]-=1; break; 
              case SDLK_KP5 : if(instr.freqf[osc] > 0 ) instr.freqf[osc]-=1; break; 
              case SDLK_KP6 : if(instr.amp[osc] > 0 ) instr.amp[osc]-=1; break; 
              case SDLK_KP7 : if(instr.freqt[osc] < 127 ) instr.freqt[osc]+=1; break; 
              case SDLK_KP8 : if(instr.freqf[osc] < 127 ) instr.freqf[osc]+=1; break; 
              case SDLK_KP9 : if(instr.amp[osc] < 127 ) instr.amp[osc]+=1; break; 
              case SDLK_KP_DIVIDE   : instr.type[osc] = OSC_TRIANGLE; break;
              case SDLK_KP_MULTIPLY : instr.type[osc] = OSC_SAW; break;
              case SDLK_KP_MINUS    : instr.type[osc] = OSC_SQUARE; break;
              case SDLK_KP_PLUS     : instr.type[osc] = OSC_NOISE; break;
              case 'o' : cutoff -= 1; if(cutoff<0) cutoff=0; break;
              case 'p' : cutoff += 1; if(cutoff>127) cutoff=127; break;
              case 'l' : res -= 1; if(res<0) res=0; break;
              case 'm' : res += 1; if(res>127) res=127; break;
              case 'a' : octave /= 2.f; break;
              case 'z'   : octave *= 2.f; break;
              default :
                if((p=my_strchr(keymap,event.key.keysym.sym))){
                  int note = p-keymap;
                  float freq = octave * freqtable[note];
                  float amp = .2f;
                  instr.cutoff=cutoff; instr.res=res;
                  create_note(&syn,freq,amp,&instr);
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
                  release_note(&syn,freq,amp,&instr);
                }
            }
            break;
        }
      }
      //usleep(1000);
    }
    SDL_CloseAudio();
#ifdef DEBUG
  } else {
    char *error = SDL_GetError();
    char *t=error;
    int len=-1;
    while(t[++len]);
    echo("SDL_OpenAudio(): ",17);
    echo(error,len);
    echo("\n",1);
#endif
  }
  //SDL_Quit();
  asm("movl $1,%eax\nxor %ebx,%ebx\nint $128\n");
}
