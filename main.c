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
  if(fichier)fwrite(stream,1,len,fichier);
}

int main(){
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
  SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO);
  atexit(SDL_Quit);
  if(SDL_OpenAudio(&desired, &obtained) == -1) printf("SDL_OpenAudio(): %s\n",SDL_GetError());
  SDL_SetVideoMode(320,240,32,0);
  
  init_synth(&syn);

  SDL_PauseAudio(0);
  while(!fini){
    while(SDL_PollEvent(&event)) {
      switch(event.type){
        case SDL_KEYDOWN :
          //printf("key %d %s\n",event.key.keysym.sym,event.key.state?"down":"up");
          switch(event.key.keysym.sym){
            case SDLK_ESCAPE : fini=1 ; break;
	    case 'a' : fichier = fopen("out.raw","w"); break;
	    case 'z' : if(fichier) fclose(fichier); fichier = 0; break;
	    case 'o' : instr.cutoff /= 1.05f; break;
	    case 'p' : instr.cutoff *= 1.05f; break;
	    case 'l' : instr.res -= .05f; break;
	    case 'm' : instr.res += .05f; break;
            default :
              if(p=strchr(keymap,event.key.keysym.sym)){
                int note = p-keymap;
		float freq = freqtable[note];
		float amp = 0.2f;
		create_note(&syn,freq,amp,&instr);
	      }
          }
	  break;
        case SDL_KEYUP :
          //printf("key %d %s\n",event.key.keysym.sym,event.key.state?"down":"up");
          switch(event.key.keysym.sym){
            default :
              if(p=strchr(keymap,event.key.keysym.sym)){
                int note = p-keymap;
		float freq = freqtable[note];
		float amp = 0.2f;
		release_note(&syn,freq,amp,&instr);
	      }
          }
          break;
      }
    }
    usleep(1000);
  }
  SDL_CloseAudio();
  if(fichier)fclose(fichier);
  return 0;
}
