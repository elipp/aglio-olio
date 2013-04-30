#ifndef WINDOW_INIT_WIN32_H
#define WINDOW_INIT_WIN32_H
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <stdarg.h>

#include <cassert>
#include <signal.h>

#include <Windows.h>
#define GLEW_STATIC 
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/wglew.h>

void window_swapbuffers();

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// declare wndproc for main window
LRESULT CALLBACK WndProc_child(HWND, UINT, WPARAM, LPARAM);	// and the same for the child window.

BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag);
void KillGLWindow(void);
GLvoid ResizeGLScene(GLsizei width, GLsizei height);

void logWindowOutput(const char *format, ...);
static void clearLogWindow();

#endif