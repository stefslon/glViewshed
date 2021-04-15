#include <stdlib.h>
#include <stdio.h>

#include "gl/glad.h"
#include "context.h"
#include "lodepng.h"
#include "linmath.h"

#include "shader.h"

#define M_PI       3.14159265358979323846 

#ifdef _WIN32
#ifndef PERFTIME
#include <windows.h>
#include <winbase.h>
#define PERFTIME_INIT unsigned __int64 freq;  QueryPerformanceFrequency((LARGE_INTEGER*)&freq); double timerFrequency = (1.0/freq);  unsigned __int64 startTime;  unsigned __int64 endTime;  double timeDifferenceInMilliseconds;
#define PERFTIME_START QueryPerformanceCounter((LARGE_INTEGER *)&startTime);
#define PERFTIME_END QueryPerformanceCounter((LARGE_INTEGER *)&endTime); timeDifferenceInMilliseconds = ((endTime-startTime) * timerFrequency);  printf("Timing %fms\n",timeDifferenceInMilliseconds);
#define PERFTIME(funct) {unsigned __int64 freq;  QueryPerformanceFrequency((LARGE_INTEGER*)&freq);  double timerFrequency = (1.0/freq);  unsigned __int64 startTime;  QueryPerformanceCounter((LARGE_INTEGER *)&startTime);  unsigned __int64 endTime;  funct; QueryPerformanceCounter((LARGE_INTEGER *)&endTime);  double timeDifferenceInMilliseconds = ((endTime-startTime) * timerFrequency);  printf("Timing %fms\n",timeDifferenceInMilliseconds);}
#endif
#else
//AIX/Linux gettimeofday() implementation here
#endif

void glErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	printf("GL error: %s type = 0x%x, severity = 0x%x\n             message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}

int main()
{
	contextInfo ci;
	if (!startContext(&ci, 4, 3))
		exit(EXIT_FAILURE);

	gladLoadGL();
	printf("OpenGL %d.%d\n", GLVersion.major, GLVersion.minor);


	/* Enable openGL error logging */
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback((GLDEBUGPROC)glErrorCallback, 0);

	/* Compile and link compute shaders */
	GLuint propProg = glCreateProgram();
	//shaderAttachFromFile(propProg, GL_COMPUTE_SHADER, "./shaders/propagation.comp");
	//shaderAttachFromFile(propProg, GL_COMPUTE_SHADER, "./shaders/visibility.comp");
	shaderAttachFromFile(propProg, GL_COMPUTE_SHADER, "./shaders/fft.comp");
	glLinkProgram(propProg);

	//GLuint coordProg = glCreateProgram();
	//shaderAttachFromFile(coordProg, GL_COMPUTE_SHADER, "./shaders/coordinates.comp");
	//glLinkProgram(coordProg);


	/* Input elevation data: PNG -> Elevation */
	unsigned int inW, inH;
	GLubyte* inData;
	lodepng_decode32_file(&inData, &inW, &inH, "./elevation/z13_2048.png");
	
	GLfloat* elevData = (GLfloat*)malloc((size_t)inW * (size_t)inH * sizeof(GLfloat));
	for (size_t ip = 0; ip < (size_t)inW * (size_t)inH; ip++) {
		//(red * 256 + green + blue / 256) - 32768
		//elevData[ip] = ((GLfloat)inData[ip] * 256.0 + (GLfloat)inData[ip + (size_t)inW * (size_t)inH] + (GLfloat)inData[ip + (size_t)inW * (size_t)inH * 2] / 256.0) - 32768.0;
		elevData[ip] = ((GLfloat)inData[ip * 4] * 256.0 + (GLfloat)inData[ip * 4 + 1] + (GLfloat)inData[ip * 4 + 2] / 256.0) - 32768.0;
	}

	/* Input data */

	/* Elevation texture */
	GLuint inElev;
	glGenTextures(1, &inElev);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, inElev);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, inW, inH, 0, GL_RED, GL_FLOAT, elevData); /* note GL_R32F ensures that internally data values are not normalized */
	glBindImageTexture(1, inElev, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

	/* Output image */
	int tex_w = inW, tex_h = inH;
	GLuint outTex;
	glGenTextures(1, &outTex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, outTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, tex_w, tex_h, 0, GL_RED, GL_FLOAT, NULL);
	glBindImageTexture(0, outTex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

	/* Setup uniforms */
	glUseProgram(propProg);

	double observerAltitude = 2.0;
	double targetAltitude = 10.0;
	double lat1 = 32.56 * M_PI / 180;
	double lon1 = -117.25 * M_PI / 180;
	double actualRadius = 6371.009; /* km */
	double effectiveRadius = 4.0 / 3.0 * actualRadius;
	double imgBounds[4] = { 32.73212635384415 * M_PI / 180, -117.29277687517559 * M_PI / 180, 32.44374242183821 * M_PI / 180, -116.91606950256245 * M_PI / 180 };


	glUniform1f(glGetUniformLocation(propProg, "observerAltitude"), (GLfloat)observerAltitude);
	glUniform1f(glGetUniformLocation(propProg, "targetAltitude"), (GLfloat)targetAltitude);
	glUniform1f(glGetUniformLocation(propProg, "lat1"), (GLfloat)lat1);
	glUniform1f(glGetUniformLocation(propProg, "lon1"), (GLfloat)lon1);
	glUniform1f(glGetUniformLocation(propProg, "actualRadius"), (GLfloat)actualRadius);
	glUniform1f(glGetUniformLocation(propProg, "effectiveRadius"), (GLfloat)effectiveRadius);
	glUniform4f(glGetUniformLocation(propProg, "imgBounds"), (GLfloat)imgBounds[0], (GLfloat)imgBounds[1], (GLfloat)imgBounds[2], (GLfloat)imgBounds[3]);

	glUniform2i(glGetUniformLocation(propProg, "imgSize"), (GLint)inH, (GLint)inW);
	

	/* Determining the Work Group Size */
	//int work_grp_cnt[3];
	//glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
	//glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
	//glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
	//printf("Max global (total) work group size x:%i y:%i z:%i\n", work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);

	//int work_grp_size[3];
	//glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
	//glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
	//glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
	//printf("Max local (in one shader) work group sizes x:%i y:%i z:%i\n", work_grp_size[0], work_grp_size[1], work_grp_size[2]);

	//int work_grp_inv;
	//glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
	//printf("Max local work group invocations %i\n", work_grp_inv);

	/* Setup job */
	int numRays = 2 * (inW - 2) + 2 * inH;


	PERFTIME_INIT
	PERFTIME_START
	{ // launch compute shaders!
		glUseProgram(propProg);
		glDispatchCompute(numRays, 1, 1);
	}
	PERFTIME_END

	// make sure writing to image has finished before read
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	//glUniform1i(glGetUniformLocation(propProg, "procType"), 1);

	//PERFTIME_START
	//{ // launch compute shaders!
	//	glUseProgram(propProg);
	//	glDispatchCompute(360, 100, 1);
	//}
	//PERFTIME_END

	//glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);



	//GLubyte* data = (GLubyte*)malloc((size_t)tex_w * (size_t)tex_h * sizeof(GLfloat));
	GLubyte* data = (GLubyte*)malloc((size_t)tex_w * (size_t)tex_h);
	if (data) {
		//glReadPixels(0, 0, tex_w, tex_h, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, data);
		//glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		//unsigned error = lodepng_encode32_file("temp.png", data, tex_w, tex_h);
		unsigned error = lodepng_encode_file("temp.png", data, tex_w, tex_h, LCT_GREY, 8);
		free(data);
	}

	free(inData);
	free(elevData);

	//glfwDestroyWindow(window);
	//glfwTerminate();
	stopContext(&ci);
	exit(EXIT_SUCCESS);
}