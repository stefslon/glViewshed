#include "context.h"

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
HWND createWindow(HINSTANCE hInstance, WNDCLASSEX& wcex, HWND& WND, HDC& DC, HGLRC& RC, int major, int minor);

void printWindowsError()
{
    char strBuffer[1024];
    
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        strBuffer,
        01024, NULL );
    
    printf("%s\n",strBuffer);
}


/// <summary>
/// Create OpenGL context
/// </summary>
/// <param name="ci"></param>
/// <param name="major"></param>
/// <param name="minor"></param>
/// <returns></returns>
int startContext(contextInfo* ci, int major, int minor)
{
	ci->DC = NULL;
	ci->RC = NULL;
	ci->hWnd = NULL;
	createWindow(0, ci->wcex, ci->hWnd, ci->DC, ci->RC, major, minor);

	pollEvents();

	if (ci->hWnd == nullptr)
		return 0;

	SetWindowText(ci->hWnd, (LPCSTR)glGetString(GL_VERSION));
	//ShowWindow(ci->hWnd, 1);

	return 1;
}

/// <summary>
/// Destroy OpenGL context
/// </summary>
/// <param name="ci"></param>
void stopContext(contextInfo* ci)
{
	pollEvents();

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(ci->RC);
	ReleaseDC(ci->hWnd, ci->DC);
	DestroyWindow(ci->hWnd);
    UnregisterClassA(ci->wcex.lpszClassName,ci->wcex.hInstance);
}


/// <summary>
/// Internal function to create a window
/// </summary>
/// <param name="hInstance"></param>
/// <param name="WND"></param>
/// <param name="DC"></param>
/// <param name="RC"></param>
/// <returns></returns>
HWND createWindow(HINSTANCE hInstance, WNDCLASSEX& wcex, HWND& WND, HDC& DC, HGLRC& RC, int major, int minor)
{
	/* Register window class */
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize         = sizeof(wcex);
	wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc	= (WNDPROC)WindowProcedure;
	wcex.hInstance      = GetModuleHandle(NULL);
    wcex.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wcex.lpszClassName  = "glContextWindow";
	//wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	//wcex.lpszClassName = "window";

	if (!RegisterClassEx(&wcex)) {
		printf("Cannot register window class: ");
        printWindowsError();
		return nullptr;
	}

	// Create fake (helper) window
	HWND fakeWND = CreateWindow("glContextWindow", "Helper Window",
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0, 1, 1, NULL, NULL, GetModuleHandle(NULL), NULL);

	if (fakeWND == NULL) {
		printf("Error creating the fake window: ");
        printWindowsError();
		return nullptr;
	}

	// Create fake contex
	HDC fakeDC = GetDC(fakeWND);

	PIXELFORMATDESCRIPTOR fakePFD;
	ZeroMemory(&fakePFD, sizeof(fakePFD));
	fakePFD.nSize = sizeof(fakePFD);
	fakePFD.nVersion = 1;
	fakePFD.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	fakePFD.iPixelType = PFD_TYPE_RGBA;
	fakePFD.cColorBits = 32;
	fakePFD.cAlphaBits = 8;
	fakePFD.cDepthBits = 24;

	int fakePFDID = ChoosePixelFormat(fakeDC, &fakePFD);
	if (fakePFDID == 0) {
		printf("Could not choose pixel format: ");
        printWindowsError();
		return nullptr;
	}

	if (SetPixelFormat(fakeDC, fakePFDID, &fakePFD) == false) {
		printf("Could not set pixel format: ");
        printWindowsError();
		return nullptr;
	}

	HGLRC fakeRC = wglCreateContext(fakeDC);    // Rendering Contex
	if (fakeRC == 0) {
		printf("Could not create fake context: ");
        printWindowsError();
		return nullptr;
	}

	if (wglMakeCurrent(fakeDC, fakeRC) == false) {
		printf("Could not make fake context current: ");
        printWindowsError();
		return nullptr;
	}

	//---create real context
	//loadOpenGLFunctions();

	WND = CreateWindow(
		"glContextWindow", "OpenGL Window",
		WS_CAPTION | WS_SYSMENU | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0,
		1, 1,
		NULL, NULL,
		GetModuleHandle(NULL), NULL);

	DC = GetDC(WND);
	const int pixelAttribs[] = {
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_ALPHA_BITS_ARB, 8,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
		WGL_SAMPLES_ARB, 4,
		0
	};

	//gladLoadGLLoader((GLADloadproc)wglGetProcAddress);
	//printf("OpenGL %d.%d\n", GLVersion.major, GLVersion.minor);

	int pixelFormatID; UINT numFormats;
	//bool status = wglChoosePixelFormat(DC, pixelAttribs, NULL, 1, &pixelFormatID, &numFormats);
	//PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
	//wglChoosePixelFormatARB = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(wglGetProcAddress("wglChoosePixelFormatARB"));
	//if (wglChoosePixelFormatARB == nullptr) {
	//	return nullptr;
	//}
	LOAD_GL_FUNC(wglChoosePixelFormatARB, PFNWGLCHOOSEPIXELFORMATARBPROC);

	//PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
	//wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
	//if (wglCreateContextAttribsARB == nullptr) {
	//	return nullptr;
	//}
	LOAD_GL_FUNC(wglCreateContextAttribsARB, PFNWGLCREATECONTEXTATTRIBSARBPROC);

	const bool status = wglChoosePixelFormatARB(DC, pixelAttribs, NULL, 1, &pixelFormatID, &numFormats);
	if (status == false || numFormats == 0) {
		printf("Could not choose pixel format: ");
        printWindowsError();
		return 0;
	}

	PIXELFORMATDESCRIPTOR PFD;
	DescribePixelFormat(DC, pixelFormatID, sizeof(PFD), &PFD);
	SetPixelFormat(DC, pixelFormatID, &PFD);


	int  contextAttribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, major,
		WGL_CONTEXT_MINOR_VERSION_ARB, minor,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};

	RC = wglCreateContextAttribsARB(DC, 0, contextAttribs);
	if (RC == NULL) {
		printf("Could not create opengl context: ");
        printWindowsError();
		return nullptr;
	}

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(fakeRC);
	ReleaseDC(fakeWND, fakeDC);
	DestroyWindow(fakeWND);
	if (!wglMakeCurrent(DC, RC)) {
		printf("Could not make opengl context current: ");
        printWindowsError();
		return nullptr;
	}

	return WND;
}

/// <summary>
/// Process and dispatch windows message queue
/// </summary>
void pollEvents()
{
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

/// <summary>
/// Standard Win32 message queue processing
/// </summary>
/// <param name="hWnd"></param>
/// <param name="message"></param>
/// <param name="wParam"></param>
/// <param name="lParam"></param>
/// <returns></returns>
LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
	switch (message) {
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			PostQuitMessage(0);
		}
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

