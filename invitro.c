#include "GL/gl.h"
#include "SDL/SDL.h"
#include "sys/soundcard.h"
#include "fcntl.h"
#include "sys/ioctl.h"
#include "unistd.h"

#define LINES 50
#define POINTS 250
#define HEIGHT 768
#define WIDTH 1024

float my_cos(float n)
{
    float ret;
    __asm volatile("fcos" : "=t" (ret) : "0" (n));
    return ret;
}

float my_sqrt(float n)
{
    float ret;
    __asm volatile("fsqrt" : "=t" (ret) : "0" (n));
    return ret;
}


float my_sin(float n)
{
    float ret;
    __asm volatile("fsin" : "=t" (ret) : "0" (n));
    return ret;
}

void _start()
{
    SDL_Event event;

    int audio_fd,i;
    short audio_buffer[4096];

    // Set up soundcard
    audio_fd = open("/dev/dsp", O_WRONLY, 0);
    i=AFMT_S16_LE;
    ioctl(audio_fd,SNDCTL_DSP_SETFMT,&i);
    i=1;
    ioctl(audio_fd,SNDCTL_DSP_CHANNELS,&i);
    i=11024;
    ioctl(audio_fd,SNDCTL_DSP_SPEED,&i);

    for (i=0;i<4096;i++)
    {
        audio_buffer[i]=i<<8;
    }

    SDL_SetVideoMode(WIDTH, HEIGHT, 0, SDL_OPENGL/*|SDL_FULLSCREEN*/);
    SDL_ShowCursor(SDL_DISABLE);

    glViewport( 0, 0, WIDTH, HEIGHT );

    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT );

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glFrustum(-1.33,1.33,-1,1,1.5,100);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

    glLoadIdentity();

    SDL_GL_SwapBuffers();

    int wireIndex;
    int pointIndex;
    glEnable(GL_BLEND);
    do
    {
        glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

        for (wireIndex = 0; wireIndex < LINES; wireIndex++){
            glPointSize(1.0);
            glBegin( GL_POINTS );
            double ticks = ((double)SDL_GetTicks())/500.0f;

            for (pointIndex = 0; pointIndex < POINTS; pointIndex++){

                float offset = ticks - pointIndex * 0.03 - wireIndex * 3;
                float longitude = my_cos(offset + my_sin(offset * 0.31)) * 2 + my_sin(offset * 0.83) * 3 + offset * 0.02;
                float latitude = my_sin(offset * 0.7) - my_cos(3 + offset * 0.23) * 3;


                float plug = 0.0;
                if (pointIndex == 0){
                    plug = 1.0;
                }
                float pulse = (my_sin(ticks * 12 - pointIndex / 5) - 0.80) * 70;
                if (pulse < 0.0){
                    pulse = 0.0;
                }

                float luminance = (POINTS - pointIndex) * (plug + pulse);
                float r = luminance * (my_sin(plug + wireIndex + ticks * 0.15));
                float g = luminance * (plug + my_sin(wireIndex - 1));
                float b = luminance * (plug + my_sin(wireIndex - 1.3));
                glColor4f(r/255,g/255,b/255,0.7);

                float distance = my_sqrt(pointIndex+0.2);
                float x = my_cos(longitude) * my_cos(latitude) * distance;
                float y = my_sin(longitude) * my_cos(latitude) * distance;
                float z = my_sin(latitude) * distance;
                glVertex3f(x,y,z);
            }
            glEnd();
        }
        ioctl(audio_fd,SNDCTL_DSP_SYNC);
        write(audio_fd,audio_buffer,8192);
        SDL_GL_SwapBuffers();
        SDL_PollEvent(&event);
    } while (event.type!=SDL_KEYDOWN);

    SDL_Quit();

    // exit cleanly
    asm ( \
         "movl $1,%eax\n" \
         "xor %ebx,%ebx\n" \
         "int $128\n" \
         );
}
