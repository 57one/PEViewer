#pragma once

#include <stdio.h>
#include <windows.h>

#include "global.h"
#include "resource.h"
#include "utility.h"

// WM_SIZE
void onSize(HWND mainDialogHwnd, LPSIZE mainDialogSize, WPARAM wParam,
            LPARAM lParam);

void onGetMinMaxInfo(MINMAXINFO* lpMMI);