#include "GL/gl.h"
#include "SDL/SDL.h"
#include "sys/soundcard.h"
#include "fcntl.h"
#include "sys/ioctl.h"
#include "unistd.h"

void _start()
{
    SDL_Event event;

    int audio_fd,i;
    short audio_buffer[4096];

    audio_fd = open("/dev/dsp", O_WRONLY, 0);
    i=AFMT_S16_LE;ioctl(audio_fd,SNDCTL_DSP_SETFMT,&i);
    i=1;ioctl(audio_fd,SNDCTL_DSP_CHANNELS,&i);
    i=11024;ioctl(audio_fd,SNDCTL_DSP_SPEED,&i);

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
        ioctl(audio_fd,SNDCTL_DSP_SYNC);
        write(audio_fd,audio_buffer,8192);
        SDL_PollEvent(&event);
    } while (event.type!=SDL_KEYDOWN);

    SDL_Quit();

    asm ( \
         "movl $1,%eax\n" \
         "xor %ebx,%ebx\n" \
         "int $128\n" \
         );

}
