#include "GL/gl.h"
#include "SDL/SDL.h"
#include <alsa/asoundlib.h>
#include "fcntl.h"
#include "sys/ioctl.h"
#include "unistd.h"

void _start()
{
    SDL_Event event;

    int audio_fd,i;
    short audio_buffer[4096];

    // Set up audio
    snd_pcm_t *snd_handle;
    snd_pcm_open(&snd_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);

    //Generate audio
    for (i=0;i<4096;i++)
    {
        audio_buffer[i]=i<<8;
    }

    SDL_SetVideoMode(640,480,0,SDL_OPENGL|SDL_FULLSCREEN);
    SDL_ShowCursor(SDL_DISABLE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.33,1.33,-1,1,1.5,100);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

    glLoadIdentity();
    glBegin(GL_TRIANGLES);
    glVertex3i(1,1,-10);
    glVertex3i(1,-1,-10);
    glVertex3i(-1,1,-10);
    glEnd();
    SDL_GL_SwapBuffers();

    do
    {
        ++i % 4096;
        snd_pcm_writei(snd_handle, audio_buffer[i], 1);
        SDL_PollEvent(&event);
    } while (event.type!=SDL_KEYDOWN);

    SDL_Quit();

    asm ( \
         "movl $1,%eax\n" \
         "xor %ebx,%ebx\n" \
         "int $128\n" \
         );

}
