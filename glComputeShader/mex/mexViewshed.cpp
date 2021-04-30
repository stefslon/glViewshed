/* */
#include <stdlib.h>
#include <stdio.h>

#include "gl/glad.h"
#include "context.h"
#include "lodepng.h"
#include "linmath.h"

#include "shader.h"

#include "mex.h"
#include "matrix.h"

#define M_PI       3.14159265358979323846 

// #ifdef _WIN32
// #ifndef PERFTIME
// #include <windows.h>
// #include <winbase.h>
// #define PERFTIME_INIT unsigned __int64 freq;  QueryPerformanceFrequency((LARGE_INTEGER*)&freq); double timerFrequency = (1.0/freq);  unsigned __int64 startTime;  unsigned __int64 endTime;  double timeDifferenceInMilliseconds;
// #define PERFTIME_START QueryPerformanceCounter((LARGE_INTEGER *)&startTime);
// #define PERFTIME_END QueryPerformanceCounter((LARGE_INTEGER *)&endTime); timeDifferenceInMilliseconds = ((endTime-startTime) * timerFrequency);  mprintf("Timing %fms\n",timeDifferenceInMilliseconds);
// #define PERFTIME(funct) {unsigned __int64 freq;  QueryPerformanceFrequency((LARGE_INTEGER*)&freq);  double timerFrequency = (1.0/freq);  unsigned __int64 startTime;  QueryPerformanceCounter((LARGE_INTEGER *)&startTime);  unsigned __int64 endTime;  funct; QueryPerformanceCounter((LARGE_INTEGER *)&endTime);  double timeDifferenceInMilliseconds = ((endTime-startTime) * timerFrequency);  mprintf("Timing %fms\n",timeDifferenceInMilliseconds);}
// #endif
// #else
// //AIX/Linux gettimeofday() implementation here
// #endif


#ifdef _WIN32
#include <windows.h>
uint64_t tic() {
    uint64_t startTime;
    QueryPerformanceCounter((LARGE_INTEGER *)&startTime);
    return startTime;
}
double toc(uint64_t startTime) {
    uint64_t freq, endTime;
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    QueryPerformanceCounter((LARGE_INTEGER *)&endTime);
    double timerFrequency = (1.0/freq);
    double timeDifferenceInMilliseconds = ((endTime-startTime) * timerFrequency);
    return timeDifferenceInMilliseconds;
}

uint64_t globalStartTime;
void ticg() { globalStartTime = tic(); }
double tocg() { return toc(globalStartTime); }
#else
#endif



#ifdef _TIMING
#define TIC() ticg();
#define TOC(helpText) mprintf("%s: %s %f ms\n",__FILE__,helpText,tocg());
#else
#define TIC()
#define TOC(helpText)
#endif


/* Global variable */
bool isInit = false;
contextInfo ci;

/* Inputs */
#define IN_Z        prhs[0]
#define IN_R        prhs[1]
#define IN_LAT1     prhs[2]
#define IN_LON1     prhs[3]
#define IN_OBS      prhs[4]
#define IN_TGT      prhs[5]
#define IN_RE       prhs[6]
#define IN_REEFF    prhs[7]

/* Outputs */
#define OUT_VIS     plhs[0]


#define FMAX(a,b) ((a>b)?(a):(b))
#define FMIN(a,b) ((a<b)?(a):(b))



void mprintf(const char* format, ...)
{
    char buf[2560];
	va_list arglist;

	va_start(arglist, format);
    vsprintf(buf, format, arglist);
	va_end(arglist);

    mexPrintf("%s", buf);
	mexEvalString("drawnow");
}


static void closeContext(void)
{
    mprintf("mexViewshed: stopping context...\n");
    pollEvents();
	stopContext(&ci);
    pollEvents();
    isInit = false;
}

void glErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	mprintf("mexViewshed: GL %s %s\n", (type == GL_DEBUG_TYPE_ERROR ? "**ERROR**" : ""), message);
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    bool firstTime = false;
	if (!isInit) {
        mprintf("mexViewshed: starting context...\n");
        mexAtExit(closeContext);
        if (!startContext(&ci, 4, 4)) {
            mprintf("startContext failure.\n");
            return;
        }
        isInit = true;
        firstTime = true;
    }
    
    if (nrhs<8) {
        mexErrMsgIdAndTxt("mexViewshed:nrhs","Need eight input arguments");
    }
    
    pollEvents();

	gladLoadGL();
    if (firstTime)
        mprintf("mexViewshed: OpenGL %d.%d\n", GLVersion.major, GLVersion.minor);

	/* Enable openGL error logging */
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback((GLDEBUGPROC)glErrorCallback, 0);

	/* Compile and link compute shaders */
	GLuint propProg = glCreateProgram();
	shaderAttachFromFile(propProg, GL_COMPUTE_SHADER, "../shaders/visibility.comp");
    //shaderAttachFromFile(propProg, GL_COMPUTE_SHADER, "../shaders/simple.comp");
    glLinkProgram(propProg);

	/* Input elevation data */
	unsigned int inW = mxGetN(IN_Z);
    unsigned int inH = mxGetM(IN_Z);
    double *inPtr = mxGetPr(IN_Z);
	
	GLfloat* elevData = (GLfloat*)malloc(inW * inH * sizeof(GLfloat));
    for (size_t ix = 0; ix < inW; ix++) {
        for (size_t iy = 0; iy < inH; iy++) {
            elevData[iy*inW+ix] = (GLfloat)inPtr[ix*inH+iy];
        }
    }
    
    unsigned int outW = pow(2,ceil(log2(inW)))/32;
    unsigned int outH = pow(2,ceil(log2(inH)));

	/* Input data */

	/* Elevation texture */
	GLuint inElev;
	glGenTextures(1, &inElev);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, inElev);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, inW, inH, 0, GL_RED, GL_FLOAT, elevData); /* note GL_R32F ensures that internally data values are not normalized */
	glBindImageTexture(1, inElev, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);

	/* Output image */
	GLuint outTex;
	glGenTextures(1, &outTex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, outTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, outW, outH, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	glBindImageTexture(0, outTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
    GLuint clearColor[1] = {0};
    glClearTexImage(outTex, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &clearColor);


	/* Setup uniforms */
	glUseProgram(propProg);

	double observerAltitude = mxGetScalar(IN_OBS);
	double targetAltitude   = mxGetScalar(IN_TGT);
	double lat1             = mxGetScalar(IN_LAT1) * M_PI / 180;
	double lon1             = mxGetScalar(IN_LON1) * M_PI / 180;
	double actualRadius     = mxGetScalar(IN_RE); /* km */
	double effectiveRadius  = mxGetScalar(IN_REEFF);
    
    double *imgBoundsPtr    = mxGetPr(IN_R);
	double imgBounds[4]     = { imgBoundsPtr[0] * M_PI / 180, imgBoundsPtr[1] * M_PI / 180, imgBoundsPtr[2] * M_PI / 180, imgBoundsPtr[3] * M_PI / 180 };
    
    if (lat1>FMAX(imgBounds[0],imgBounds[2]) || lat1<FMIN(imgBounds[0],imgBounds[2]) || 
            lon1>FMAX(imgBounds[1],imgBounds[3]) || lon1<FMIN(imgBounds[1],imgBounds[3])) {
        mexErrMsgIdAndTxt("mexViewshed:nrhs","Observer location is outside defined altitude raster");
    }


	glUniform1f(glGetUniformLocation(propProg, "observerAltitude"), (GLfloat)observerAltitude);
	glUniform1f(glGetUniformLocation(propProg, "targetAltitude"), (GLfloat)targetAltitude);
	glUniform1f(glGetUniformLocation(propProg, "lat1"), (GLfloat)lat1);
	glUniform1f(glGetUniformLocation(propProg, "lon1"), (GLfloat)lon1);
	glUniform1f(glGetUniformLocation(propProg, "actualRadius"), (GLfloat)actualRadius);
	glUniform1f(glGetUniformLocation(propProg, "effectiveRadius"), (GLfloat)effectiveRadius);
	glUniform4f(glGetUniformLocation(propProg, "imgBounds"), (GLfloat)imgBounds[0], (GLfloat)imgBounds[1], (GLfloat)imgBounds[2], (GLfloat)imgBounds[3]);

	glUniform2i(glGetUniformLocation(propProg, "imgSize"), (GLint)inH, (GLint)inW);
	

	/* Determining the Work Group Size */
    // int work_grp_cnt[3];
    // glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
    // glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
    // glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
    // mprintf("Max global (total) work group size x:%i y:%i z:%i\n", work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);
    // 
    // int work_grp_size[3];
    // glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
    // glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
    // glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
    // mprintf("Max local (in one shader) work group sizes x:%i y:%i z:%i\n", work_grp_size[0], work_grp_size[1], work_grp_size[2]);
    // 
    // int work_grp_inv;
    // glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
    // mprintf("Max local work group invocations %i\n", work_grp_inv);

	/* Setup job */
	int numRays = 2 * (inW - 2) + 2 * inH;


	TIC();
 	{ // Launch compute shaders!
 		glUseProgram(propProg);
 		glDispatchCompute(numRays, 1, 1);
 	}
    
	// Make sure writing to image has finished before read
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
    TOC("Compute shader");

    {
        TIC();
        GLuint* gldata  = (GLuint*)malloc(outW * outH * sizeof(GLuint));
        OUT_VIS         = mxCreateNumericMatrix(inH, inW, mxUINT8_CLASS, mxREAL);
        GLubyte* data   = (GLubyte*)mxGetData(OUT_VIS);
        if (data) {
            glGetTextureImage(outTex, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, outW * outH * sizeof(GLuint), gldata);
            
            for (size_t ix = 0; ix < inW; ix++) {
                for (size_t iy = 0; iy < inH; iy++) {
                    data[ix*inH+iy] = (gldata[iy*inW/32+ix/32] >> (ix%32)) & 1;
                }
            }
        }
        free(gldata);
        TOC("Transfer visibility texture");
    }
    
	free(elevData);

	return;
}