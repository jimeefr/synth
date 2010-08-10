#include "synth.h"
#include <SDL/SDL.h>

void fetch_audio_buffer(void *userdata, Uint8 *stream, int len){
  float l,r;
  short *p = (short *)stream;
  int i = len / 4;
  synth *syn = (synth *)userdata;
  do {
    render_synth(syn,&l,&r);
    *(p++) = (short)(l * 32767.f);
    *(p++) = (short)(r * 32767.f);
  } while(--i);
}

int main(){
  synth syn;

  SDL_AudioSpec desired, obtained;
  desired.freq=SAMPLERATE;
  desired.format=AUDIO_S16;
  desired.samples=256;
  desired.callback=fetch_audio_buffer;
  desired.userdata = &syn;
  SDL_OpenAudio(&desired, &obtained);
  
  init_synth(&syn);

  SDL_PauseAudio(0);
  usleep(100000);
  SDL_CloseAudio();
  return 0;
}
