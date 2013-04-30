#include "glwindow_win32.h"

static HGLRC hRC = NULL;
static HDC hDC	  = NULL;
static HWND hWnd = NULL;
static HINSTANCE hInstance;

static HWND hWnd_child = NULL;

bool fullscreen = false;
bool active = TRUE;

extern int initGL();

static std::string *convertLF_to_CRLF(const char *buf);

bool keys[256] = { false };

void window_swapbuffers() {
	SwapBuffers(hDC);
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_ACTIVATE:
		if(!HIWORD(wParam))
		{
			active=TRUE;
		}

		else 
		{
			active=FALSE;
		}
		return 0;

	case WM_SYSCOMMAND:
		switch(wParam)
		{
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			return 0;
		}
		break;

	case WM_CLOSE:
		{
		PostQuitMessage(0);
		return 0;
		}

	case WM_KEYDOWN:
		{
			keys[wParam]=TRUE;
			return 0;
		}
	case WM_KEYUP:
		{
			keys[wParam]=FALSE;
			return 0;
		}
	case WM_SIZE:
		{
			ResizeGLScene(LOWORD(lParam), HIWORD(lParam));
		}
	;
	}

	/* the rest shall be passed to defwindowproc. (default window procedure) */
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


LRESULT CALLBACK WndProc_child(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { 
	switch(uMsg)
	{
	case WM_VSCROLL:
		   SendMessage(hWnd, EM_LINESCROLL, (WPARAM)wParam, (LPARAM)lParam);
			break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam); 
}


BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag)
{
	GLuint PixelFormat;
	WNDCLASS wc;
	DWORD dwExStyle;
	DWORD dwStyle;

	RECT WindowRect;
	WindowRect.left=(long)0;
	WindowRect.right=(long)width;
	WindowRect.top=(long)0;
	WindowRect.bottom=(long)height;

	fullscreen = fullscreenflag;

	hInstance = GetModuleHandle(NULL);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC) WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "OpenGL";

	if (!RegisterClass(&wc))
	{
		MessageBox(NULL, "FAILED TO REGISTER THE WINDOW CLASS.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	DEVMODE dmScreenSettings;
	memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
	dmScreenSettings.dmSize = sizeof(dmScreenSettings);
	dmScreenSettings.dmPelsWidth = width;
	dmScreenSettings.dmPelsHeight = height;
	dmScreenSettings.dmBitsPerPel = bits;
	dmScreenSettings.dmFields= DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

	/*
	 * no need to test this now that fullscreen is turned off
	 *
	if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
	{
		if (MessageBox(NULL, "The requested fullscreen mode is not supported by\nyour video card. Use Windowed mode instead?", "warn", MB_YESNO | MB_ICONEXCLAMATION)==IDYES)
		{
			fullscreen=FALSE;
		}
		else {

			MessageBox(NULL, "Program willl now close.", "ERROR", MB_OK|MB_ICONSTOP);
			return FALSE;
		}
	}*/

	if (fullscreen)
	{
		dwExStyle=WS_EX_APPWINDOW;
		dwStyle=WS_POPUP;

	}

	else {
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle=WS_OVERLAPPEDWINDOW;
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

	if(!(hWnd=CreateWindowEx( dwExStyle, "OpenGL", title,
							  WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dwStyle,
							  0, 0,
							  WindowRect.right-WindowRect.left,
							  WindowRect.bottom-WindowRect.top,
							  NULL,
							  NULL,
							  hInstance,
							  NULL)))
	{
		KillGLWindow();
		MessageBox(NULL, "window creation error.", "ERROR", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}


	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		bits,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		16,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	if (!(hDC=GetDC(hWnd)))
	{
		KillGLWindow();
		MessageBox(NULL, "CANT CREATE A GL DEVICE CONTEXT.", "ERROR", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!(PixelFormat = ChoosePixelFormat(hDC, &pfd)))
	{
		KillGLWindow();
		MessageBox(NULL, "cant find a suitable pixelformat.", "ERROUE", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}


	if(!SetPixelFormat(hDC, PixelFormat, &pfd))
	{
		KillGLWindow();
		MessageBox(NULL, "Can't SET ZE PIXEL FORMAT.", "ERROU", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if(!(hRC=wglCreateContext(hDC)))
	{
		KillGLWindow();
		MessageBox(NULL, "WGLCREATECONTEXT FAILED.", "ERREUHX", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if(!wglMakeCurrent(hDC, hRC))
	{
		KillGLWindow();
		MessageBox(NULL, "Can't activate the gl rendering context.", "ERAIX", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	ShowWindow(hWnd, SW_SHOW);
	SetForegroundWindow(hWnd);
	SetFocus(hWnd);
	ResizeGLScene(width, height);

	// create child window for logging :P
	WNDCLASSEX wc_c;
    static char *className_c = "Log window";
    static char *windowName_c = "aglio-olio: Log window";

    wc_c.cbSize        = sizeof (WNDCLASSEX);
    wc_c.style         = 0;
    wc_c.lpfnWndProc   = WndProc_child;
    wc_c.cbClsExtra    = 0;
    wc_c.cbWndExtra    = 0;
    wc_c.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
    wc_c.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc_c.hbrBackground = (HBRUSH) GetStockObject (GRAY_BRUSH);
    wc_c.lpszMenuName  = NULL;
    wc_c.lpszClassName = className_c;
    wc_c.hInstance     = hInstance;
    wc_c.hIconSm       = LoadIcon (NULL, IDI_APPLICATION);
	
	ATOM child_register = RegisterClassEx (&wc_c);
    DWORD child_get_last_error  = GetLastError ();
 
    
    hWnd_child = CreateWindowEx ( WS_EX_CLIENTEDGE,                      // no extended styles           
                                "EDIT",           // class name                   
                                windowName_c,          // window name                  
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_VSCROLL |
                                ES_LEFT | ES_MULTILINE | ES_READONLY,        
                                CW_USEDEFAULT,          // default horizontal position  
                                CW_USEDEFAULT,          // default vertical position    
                                1024,          // default width                
                                768,          // default height               
                                hWnd, 
                                (HMENU) NULL,           // class menu used              
                                hInstance,              // instance handle              
                                NULL);                  // no window creation data      
 
    if (!hWnd_child) 
        return FALSE; 

	HFONT myFont = CreateFont(12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Consolas");

	SendMessage(hWnd_child, WM_SETFONT, WPARAM(myFont), TRUE);

	ShowWindow (hWnd_child, SW_SHOW); 
    UpdateWindow (hWnd_child);
	// clear log window
	clearLogWindow();

	if (!initGL())
	{
		//KillGLWindow();
		MessageBox(NULL, "initGL() failed.", "ERRROR", MB_OK|MB_ICONEXCLAMATION);
		KillGLWindow();
		return FALSE;
	}
	SetForegroundWindow(hWnd);
	SetFocus(hWnd);
	return TRUE;
}

void KillGLWindow(void)
{
	if(hRC)
	{
		if(!wglMakeCurrent(NULL,NULL))
		{
			MessageBox(NULL, "wglMakeCurrent(NULL,NULL) failed", "erreur", MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))
		{
			MessageBox(NULL, "RELEASE of rendering context failed.", "error", MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;

		if(hDC && !ReleaseDC(hWnd, hDC))
		{
			MessageBox(NULL, "Release DC failed.", "ERREUX", MB_OK | MB_ICONINFORMATION);
			hDC=NULL;
		}

		if(hWnd && !DestroyWindow(hWnd))
		{
			MessageBox(NULL, "couldn't release hWnd.", "erruexz", MB_OK|MB_ICONINFORMATION);
			hWnd=NULL;
		}

		if (!UnregisterClass("OpenGL", hInstance))
		{
			MessageBox(NULL, "couldn't unregister class.", "err", MB_OK | MB_ICONINFORMATION);
			hInstance=NULL;
		}

	}

}

static std::string *convertLF_to_CRLF(const char *buf) {
	std::string *buffer = new std::string(buf);
	size_t start_pos = 0;
	static const std::string LF = "\n";
	static const std::string CRLF = "\r\n";
    while((start_pos = buffer->find(LF, start_pos)) != std::string::npos) {
        buffer->replace(start_pos, LF.length(), CRLF);
        start_pos += LF.length()+1; // +1 to avoid the new \n we just created :P
    }
	return buffer;
}


void logWindowOutput(const char *format, ...) {
	static char msg_buf[2048];
	va_list args;
	va_start(args, format);
	SYSTEMTIME st;
    GetSystemTime(&st);
	std::size_t timestamp_len = sprintf(msg_buf, "%02d:%02d:%02d.%03d > ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	std::size_t msg_len = vsprintf(msg_buf + timestamp_len, format, args);
	std::size_t total_len = timestamp_len + msg_len;
	msg_buf[total_len] = '\0';
	std::string *converted = convertLF_to_CRLF(msg_buf);

    va_end(args);
	int nLength = GetWindowTextLength(hWnd_child); 
   SendMessage(hWnd_child, EM_SETSEL, (WPARAM)nLength, (LPARAM)nLength);
   SendMessage(hWnd_child, EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)converted->c_str());
   SendMessage(hWnd_child, EM_SCROLLCARET, (WPARAM)0, (LPARAM)0);
   	delete converted;
}

static void clearLogWindow() {
	int nLength = GetWindowTextLength(hWnd_child); 
   SendMessage(hWnd_child, EM_SETSEL, (WPARAM)0, (LPARAM)nLength);
   SendMessage(hWnd_child, EM_REPLACESEL, (WPARAM)FALSE, (LPARAM)NULL);
}


GLvoid ResizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0,0,width,height);						// Reset The Current Viewport


	// Calculate The Aspect Ratio Of The Window
	//gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f);

}