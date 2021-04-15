#ifndef SHADER_H
#define SHADER_H

#include "gl/glad.h"

#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif

static char* loadTextFile(const char* filePath);
static GLuint shaderCompileFromFile(GLenum type, const char* filePath);
void shaderAttachFromFile(GLuint program, GLenum type, const char* filePath);

#endif