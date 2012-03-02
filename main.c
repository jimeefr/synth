#include "synth.h"
#include "gui.h"
#include <SDL/SDL.h>

static void fetch_audio_buffer(void *userdata, Uint8 *stream, int len){
  render_synth((short *)stream,len>>2);
}

#ifdef DEBUG
__attribute__((always_inline)) static void echo(char *s,int l){
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

void _start(){
  SDL_AudioSpec desired, obtained;

  desired.freq=SAMPLERATE;
  desired.format=AUDIO_S16;
  desired.channels=2;
  desired.samples=512;
  desired.callback=fetch_audio_buffer;
  SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO);
  if(!SDL_OpenAudio(&desired, &obtained)){
    gui_init();
    SDL_PauseAudio(0);

    gui_mainloop();

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
  SDL_Quit();
  asm("movl $1,%eax\nxor %ebx,%ebx\nint $128\n");
}
