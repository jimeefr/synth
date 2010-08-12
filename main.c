#include "synth.h"
#include <SDL/SDL.h>

char keymap[] = "wsxdcvgbhnj,;";
float freqtable[] = {
  261.6f,277.2f,293.7f,311.1f,329.6f,349.2f,370.f,392.f,415.3f,440.f,466.2f,493.9f,523.3f
};
instrument instr = {
 0.0f, 0.1f , .0f, 0.f, /* a,d,s,r */
 { OSC_TRIANGLE, OSC_TRIANGLE, OSC_TRIANGLE, OSC_SQUARE,
   OSC_SQUARE, OSC_NOISE, OSC_TRIANGLE, OSC_TRIANGLE }, /* 8 osc types */
 { 1.0f, 0.f, 0.f, 0.1f, 0.11f, 0.f, 0.f, 0.f }, /* 8 freqs */
 { 1.0f, 0.f, 0.f, 2.0f, 2.f, 0.f, 0.f, 0.f },  /* 8 amps */
 800.f, 0.8f
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
              case 'o' : instr.cutoff /= 1.05f; break;
              case 'p' : instr.cutoff *= 1.05f; break;
              case 'l' : instr.res -= .05f; break;
              case 'm' : instr.res += .05f; break;
              default :
                if(p=my_strchr(keymap,event.key.keysym.sym)){
                  int note = p-keymap;
                  float freq = freqtable[note];
                  float amp = 0.2f;
                  create_note(&syn,freq,amp,&instr);
                }
            }
            break;
          case SDL_KEYUP :
            switch(event.key.keysym.sym){
              default :
                if(p=my_strchr(keymap,event.key.keysym.sym)){
                  int note = p-keymap;
                  float freq = freqtable[note];
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
