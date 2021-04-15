#ifndef _CONTEXT_H
#define _CONTEXT_H

#include <stdio.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <GL\GL.h>
#include "gl\wglext.h"

#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif

// Basic code was taken from:
// https://gist.github.com/Eleobert/d4bbf044db7cb5a587666cff5a6f1174

struct contextInfo {
	HDC DC;
	HGLRC RC;
	HWND hWnd;
    WNDCLASSEX wcex;
};

#define LOAD_GL_FUNC(x, p) p x; x =  reinterpret_cast<p>(wglGetProcAddress(#x)); \
					if (x == nullptr) { printf("Error Loading OpenGL function %s", #x); return 0;}

int startContext(contextInfo* ci, int major, int minor);
void stopContext(contextInfo* ci);
void pollEvents();


#endif // !_CONTEXT_H
