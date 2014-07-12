#include <GL/gl.h>
#include <SDL/SDL.h>
#include <unistd.h>

#define FLAMES 3
#define RES 13 // 2^13 textures

#define FIRSTLEVEL 5
#define LASTLEVEL 10

#define ITERATIONS 3

#define INITSCALE 2.0

#define TEXSCALE 0.25f

const char *fragmentShader = ""
"varying vec2 destcoords;\n"
"uniform sampler2D tex;\n"
"const float M_PI=3.14159265358979; const float M_1_PI=1.0/M_PI; const float EPS=1.0e-6; uniform float texscale; uniform float texscalei; float TXfmPL(float x) { x*=texscale; return x/sqrt(1.0+x*x)*0.5+0.5; } vec2 TXfmPL(vec2 p) { return vec2(TXfmPL(p.x),TXfmPL(p.y)); } float PLfmTX(float s) { float u=2.0*s-1.0; return texscalei*u/sqrt(1.0-u*u); } vec2 PLfmTX(vec2 s) { return vec2(PLfmTX(s.x),PLfmTX(s.y)); }\n"
"/*------------------- w0 -----------------*/\n"
"uniform vec4 color0;\n"
"vec4 g0(vec2 p) {\n"
"        return exp2(texture2D(tex,TXfmPL(p))*20.0)-1.0;\n"
"}\n"
"uniform vec2 xf0[8]; // Forward xyo[0-2], inverse XYO[3-5], vec2(wvar,1.0/wvar)[6], affinearea[7]\n"
"float jacobian0(vec2 t) {float r2=dot(t,t); return 1.0/(r2*r2);}\n"
"vec4 f0(vec2 inv) { /* called by nonlinear inverters, invert linear part of map */ \n"
"        float areaScale=1.0/(1.0e-2+abs(xf0[7].x*jacobian0(inv)));\n"
"        vec2 p=xf0[3]*inv.x+xf0[4]*inv.y+xf0[5];\n"
"        return areaScale*g0(p);\n"
"}\n"
"vec4 nonlinear_inverse0(vec2 p) {\n"
"        p=p*xf0[6].y;\n"
"        float r2=dot(p,p); return f0(p/r2);\n"
"}\n"
"/*------------------- w1 -----------------*/\n"
"uniform vec4 color1;\n"
"vec4 g1(vec2 p) {\n"
"        return exp2(texture2D(tex,TXfmPL(p))*20.0)-1.0;\n"
"}\n"
"uniform vec2 xf1[8]; // Forward xyo[0-2], inverse XYO[3-5], vec2(wvar,1.0/wvar)[6], affinearea[7]\n"
"float jacobian1(vec2 t) {float r2=dot(t,t); return 1.0/(r2*r2);}\n"
"vec4 f1(vec2 inv) { /* called by nonlinear inverters, invert linear part of map */ \n"
"        float areaScale=1.0/(1.0e-2+abs(xf1[7].x*jacobian1(inv)));\n"
"        vec2 p=xf1[3]*inv.x+xf1[4]*inv.y+xf1[5];\n"
"        return areaScale*g1(p);\n"
"}\n"
"vec4 nonlinear_inverse1(vec2 p) {\n"
"        p=p*xf1[6].y;\n"
"        float r2=dot(p,p); return f1(p/r2);\n"
"}\n"
"/*------------------- w2 -----------------*/\n"
"uniform vec4 color2;\n"
"vec4 g2(vec2 p) {\n"
"        return exp2(texture2D(tex,TXfmPL(p))*20.0)-1.0;\n"
"}\n"
"uniform vec2 xf2[8]; // Forward xyo[0-2], inverse XYO[3-5], vec2(wvar,1.0/wvar)[6], affinearea[7]\n"
"float jacobian2(vec2 t) {float r2=dot(t,t); return 1.0/(r2*r2);}\n"
"vec4 f2(vec2 inv) { /* called by nonlinear inverters, invert linear part of map */ \n"
"        float areaScale=1.0/(1.0e-2+abs(xf2[7].x*jacobian2(inv)));\n"
"        vec2 p=xf2[3]*inv.x+xf2[4]*inv.y+xf2[5];\n"
"        return areaScale*g2(p);\n"
"}\n"
"vec4 nonlinear_inverse2(vec2 p) {\n"
"        p=p*xf2[6].y;\n"
"        float r2=dot(p,p); return f2(p/r2);\n"
"}\n"
"/* Combined inverse-sampling function */\n"
"vec4 sum_inverses(vec2 p) {\n"
"        vec4 sum=vec4(0.0);\n"
"        sum+=color0*nonlinear_inverse0(p);\n"
"        sum+=color1*nonlinear_inverse1(p);\n"
"        sum+=color2*nonlinear_inverse2(p);\n"
"\n"
"        return log2(sum+1.0)*(1.0/20.0);\n"
"}\n"
"void main(void) { gl_FragColor = sum_inverses(PLfmTX(destcoords)); }\n";

const char *vertexShader = ""
"varying vec2 destcoords;\n"
"void main(void) {\n"
"   destcoords = vec2(gl_Vertex);\n"
"   gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;\n"
"}\n";


inline void checkShaderOp(GLhandleARB obj, GLenum errType)
{
    GLint compiled;
    glGetObjectParameterivARB(obj, errType, &compiled);
    if (!compiled)
    {
        enum { LOGSIZE = 10000 };
        GLcharARB errorLog[LOGSIZE];
        // Our buffer (errorLog) must be fixed-length, because we are using
        //  OpenGL's "C" API. However, OpenGL knows how big the buffer is,
        //  and there is no possibility of overflow.
        GLsizei len = 0;
        glGetInfoLogARB(obj, LOGSIZE, &len, errorLog);
        printf("shader compile error: %s\n", errorLog);
        exit(1);
    }
}


// Create a vertex or fragment shader from this code.
GLhandleARB makeShaderObject(int target, const char *code)
{
    GLhandleARB h = glCreateShaderObjectARB(target);
    glShaderSourceARB(h,1,&code,NULL);
    glCompileShaderARB(h);
    checkShaderOp(h,GL_OBJECT_COMPILE_STATUS_ARB);
    return h;
}

static inline void setAffineArea(float vector[])
{//    affine             wvar        wvar          x.x         y.y         x.y         y.x
    vector[14] = fabs(vector[6] * vector[6] * (vector[0] * vector[3] - vector[1] * vector[2]));
}

static inline void setInvMat(float vector[])
{//                             x.x         y.y         x.y         y.x
    float invdet = 1.0 / (vector[0] * vector[3] - vector[1] * vector[2]);

    //    X.x                   y.y
    vector[6] = invdet *  vector[3];
    //    Y.x                   y.x
    vector[8] = invdet * -vector[2];
    //    X.y                   x.y
    vector[7] = invdet * -vector[1];
    //    Y.y                   x.x
    vector[9] = invdet *  vector[0];

    //     O.x          X.x         o.x         Y.x         o.y
    vector[10] = -vector[6] * vector[4] + vector[8] * vector[5];
    //     O.y          X.y         o.x         Y.y         o.y
    vector[11] = -vector[7] * vector[4] + vector[9] * vector[5];
}

void _start()
{
    SDL_Event event;

    SDL_SetVideoMode(640,480,0,SDL_OPENGL/*|SDL_FULLSCREEN*/);
    SDL_ShowCursor(SDL_DISABLE);


//    SDL_GL_SwapBuffers();

    // map_exposure=2.0; colorful=0.5;
    // xf.init(0.3333,&singleton_spherical,0.4,"0.5 0 0 0.5 -0.566  0.400" ); xfm.push_back(xf);
    // xf.init(0.3333,&singleton_spherical,0.4,"0.5 0 0 0.5  0.566  0.400"); xfm.push_back(xf);
    // xf.init(0.3333,&singleton_spherical,0.4,"0.5 0 0 0.5  0.000 -0.551"); xfm.push_back(xf);


    float opts[3][16] = {
    //       0     1      2     3         4       5     6     7     8     9    10    11    12      13       14    15
    //     x.x   x.y    y.x   y.y       o.x     o.y   X.x   X.y   Y.x   Y.y   O.x   O.y  wvar   iwvar   affine   pad
        { 0.5f, 0.0f,  0.0f, 0.5f,  -0.566f,  0.400f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 1/0.4f,    0.0f, 0.0f },
        { 0.5f, 0.0f,  0.0f, 0.5f,   0.566f,  0.400f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 1/0.4f,    0.0f, 0.0f },
        { 0.5f, 0.0f,  0.0f, 0.5f,     0.0f, -0.551f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.4f, 1/0.4f,    0.0f, 0.0f },
    };
    setAffineArea(opts[0]);
    setAffineArea(opts[1]);
    setAffineArea(opts[2]);
    setInvMat(opts[0]);
    setInvMat(opts[1]);
    setInvMat(opts[2]);

    do
    {
        /* Rotate
        for (unsigned int m=0;m<xfm.size();m++) { // rotation rate (rads/msec) is slightly different for different maps
            double rate=0.001*0.2*(1+0.1234*m);
            double angle=rate*(last_msec-animate_start);
            //printf("Angle=%.2f rads\n",angle);
            float c=cos(angle), s=sin(angle);
            vec2  tx=xfm[m].x*c - xfm[m].y*s;
            xfm[m].y=xfm[m].x*s + xfm[m].y*c;
            xfm[m].x=tx;
        }*/


        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glFrustum(-1.33,1.33,-1,1,1.5,100);
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
        glLoadIdentity();

        static GLuint grid[RES+1][2] = {{0}};
        GLuint frameSrc=0, frameDst;
        if (grid[0][0] == 0) { // First time, set up
            glGenTextures(2 * (RES + 1), &grid[0][0]);
            for (int level=0; level<RES+1; level++) {
                for (int i=0; i<2; i++) {
                    glBindTexture(GL_TEXTURE_2D, grid[level][i]);
                    glTexImage2D(GL_TEXTURE_2D, // target
                            0, // base level of detail
                            GL_RGBA8, // format
                            1 << level, // width
                            1 << level, // height
                            0, // border
                            GL_LUMINANCE, // format
                            GL_FLOAT, // type
                            0  // data
                            );
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

                }
            }
        }

        static GLuint fbo = 0;
        if (fbo == 0) {
            glGenFramebuffers(1, &fbo);
        }

        glBindFramebuffer(GL_FRAMEBUFFER_EXT, fbo);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDisable(GL_CULL_FACE);
        glScalef(1.0, 1.0, 0.01); /* scale down Z axis, to avoid z-clipping */
        glScalef(2.0, 2.0, 1.0); /* center X and Y on 0.5,0.5, like texture coordinates */
        glTranslatef(-0.5, -0.5, 0.0);

        for (int level = FIRSTLEVEL; level <= LASTLEVEL; level++) {
            frameDst = grid[level][0];
            if (level == FIRSTLEVEL) {
                frameSrc = grid[level][1];
            }

            int width = 1 << level;
            int height = width;

            for (int depth = 0; depth < ITERATIONS; depth++) {
                printf("foo\n");
                glFramebufferTexture2D(GL_FRAMEBUFFER,
                                       GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_2D,
                                       frameDst,
                                       0);

#ifdef DEBUG
                GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                if (status != GL_FRAMEBUFFER_COMPLETE) {
                    printf("level: %d fitte: %d\n", level, status);
                    SDL_Quit();
                    exit(1);
                }
#endif
                glDisable(GL_DEPTH_TEST);
                glViewport(0,0, width, height);
                glScissor(0,0, width, height);

                if (depth == 0 && level == FIRSTLEVEL) { // first round, complete reset
                    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT);

                    glDisable(GL_TEXTURE_2D);
                    glPushMatrix();
                    glTranslatef(0.5,0.5,0.0);
                    glColor3f(0.0f, 0.0f, 0.0f);
                    glBegin(GL_TRIANGLES);
                    glVertex3i(1,1,-10);
                    glVertex3i(1,-1,-10);
                    glVertex3i(-1,1,-10);
                    glEnd();
                    glPopMatrix();
                    glColor3f(1,1,1);

                    glFinish();

                    glBindTexture(GL_TEXTURE_2D, frameDst);
                    glEnable(GL_TEXTURE_2D);
                    glGenerateMipmap(GL_TEXTURE_2D);

                    // swap dst and src
                    GLuint tmp = frameDst;
                    frameDst = frameSrc;
                    frameSrc = tmp;

                    continue; // onto drawing plz
                }

                glBindTexture(GL_TEXTURE_2D, frameSrc);
                glEnable(GL_TEXTURE_2D);

                glDisable(GL_BLEND); // avoids erasing

                static GLhandleARB prog = 0;
                if (prog == 0) {
                    prog = glCreateProgram();
                    GLhandleARB vo = makeShaderObject(GL_VERTEX_SHADER_ARB, vertexShader);
                    GLhandleARB fo = makeShaderObject(GL_FRAGMENT_SHADER_ARB, fragmentShader);
                    glAttachObjectARB(prog, vo);
                    glAttachObjectARB(prog, fo);
                    glLinkProgram(prog);

                    checkShaderOp(prog, GL_OBJECT_LINK_STATUS_ARB);

                    glDeleteObjectARB(vo);
                    glDeleteObjectARB(fo);
                }
                glUseProgramObjectARB(prog);
                glUniform1iARB(glGetUniformLocationARB(prog,"tex"),0);
                glUniform1fARB(glGetUniformLocationARB(prog,"texscale"), TEXSCALE);
                glUniform1fARB(glGetUniformLocationARB(prog,"texscalei"), 1/TEXSCALE);

                glUniform2fvARB(glGetUniformLocationARB(prog, "xf0", 8, &opts[0]));
                glUniform2fvARB(glGetUniformLocationARB(prog, "xf1", 8, &opts[16]));
                glUniform2fvARB(glGetUniformLocationARB(prog, "xf2", 8, &opts[32]));

                glEnable(GL_BLEND);
                glBindTexture(GL_TEXTURE_2D, frameDst);
                glGenerateMipmap(GL_TEXTURE_2D);

                GLuint tmp = frameDst;
                frameDst = frameSrc;
                frameSrc = tmp;
                if (depth == 0) {
                    frameDst = grid[level][1];
                }
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        SDL_PollEvent(&event);
    } while (event.type!=SDL_KEYDOWN);

    SDL_Quit();

    asm ( \
         "movl $1,%eax\n" \
         "xor %ebx,%ebx\n" \
         "int $128\n" \
         );

}
