#pragma once
#include <windows.h>
#include <gdiplus.h>

#include "global.h"
#include "handleProcessAndModule.h"
#include "messageCallback.h"
#include "resource.h"
using namespace Gdiplus;

BOOL CALLBACK MainDialogProc(HWND hwnd,      // handle to dialog box
                             UINT message,   // message
                             WPARAM wParam,  // first message parameter
                             LPARAM lParam   // second message parameter
);

BOOL CALLBACK AboutProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK PeEditorProc(HWND hwnd, UINT message, WPARAM wParam,
                           LPARAM lParam);