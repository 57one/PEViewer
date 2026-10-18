#pragma once
#include <windows.h>
#include <gdiplus.h>

#include "common/global.h"
#include "process/handleProcessAndModule.h"
#include "messageCallback.h"
#include "resource.h"
using namespace Gdiplus;

INT_PTR CALLBACK MainDialogProc(HWND hwnd,      // handle to dialog box
                             UINT message,   // message
                             WPARAM wParam,  // first message parameter
                             LPARAM lParam   // second message parameter
);

INT_PTR CALLBACK AboutProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK PeEditorProc(HWND hwnd, UINT message, WPARAM wParam,
                           LPARAM lParam);

INT_PTR CALLBACK FileHeaderProc(HWND hwnd, UINT message, WPARAM wParam,
                             LPARAM lParam);

INT_PTR CALLBACK MachineTypeProc(HWND hwnd, UINT message, WPARAM wParam,
                              LPARAM lParam);

INT_PTR CALLBACK CharacteristicsProc(HWND hwnd, UINT message, WPARAM wParam,
                                  LPARAM lParam);

INT_PTR CALLBACK TimeDateStampProc(HWND hwnd, UINT message, WPARAM wParam,
                                  LPARAM lParam);

INT_PTR CALLBACK OptionalHeader32Proc(HWND hwnd, UINT message, WPARAM wParam,
                                LPARAM lParam);

INT_PTR CALLBACK MagicTypeProc(HWND hwnd, UINT message, WPARAM wParam,
                            LPARAM lParam);

INT_PTR CALLBACK SubsystemProc(HWND hwnd, UINT message, WPARAM wParam,
                            LPARAM lParam);

INT_PTR CALLBACK DllCharacteristicsProc(HWND hwnd, UINT message, WPARAM wParam,
                            LPARAM lParam);

INT_PTR CALLBACK HeaderInfoProc(HWND hwnd, UINT message, WPARAM wParam,
                                     LPARAM lParam);

INT_PTR CALLBACK SectionProc(HWND hwnd, UINT message, WPARAM wParam,
                             LPARAM lParam);

INT_PTR CALLBACK DirectoryProc(HWND hwnd, UINT message, WPARAM wParam,
                            LPARAM lParam);

INT_PTR CALLBACK SectionEditProc(HWND hwnd, UINT message, WPARAM wParam,
                              LPARAM lParam);

INT_PTR CALLBACK SectionCharacteristicsProc(HWND hwnd, UINT message, WPARAM wParam,
                                         LPARAM lParam);

INT_PTR CALLBACK ImportDirectoryProc(HWND hwnd, UINT message, WPARAM wParam,
                                  LPARAM lParam);

INT_PTR CALLBACK ExportDirectoryProc(HWND hwnd, UINT message, WPARAM wParam,
                                  LPARAM lParam);

INT_PTR CALLBACK BoundImportDirectoryProc(HWND hwnd, UINT message, WPARAM wParam,
                                       LPARAM lParam);

INT_PTR CALLBACK DebugDirectoryProc(HWND hwnd, UINT message, WPARAM wParam,
                                 LPARAM lParam);

INT_PTR CALLBACK IatDirectoryProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK RelocationDirectoryProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ResourceDirectoryProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
