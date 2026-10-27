#pragma once

#include <windows.h>
#include "pe/PEContext.h"

// UI adapters: parse the supplied context and render the structured results.
void showDirectory(HWND hwnd, const PEContext& context);
void showImportDirectory(HWND hwnd, HWND list, const PEContext& context);
void showImportThunks(HWND hwnd, HWND dllList, HWND list, const PEContext& context);
void showExportDirectory(HWND hwnd, const PEContext& context);
void showBoundImportDirectory(HWND hwnd, HWND list, const PEContext& context);
void showDebugDirectory(HWND hwnd, HWND list, const PEContext& context);
void showResourceDirectory(HWND hwnd, const PEContext& context);
void showTlsDirectory(HWND hwnd, const PEContext& context);
void showExceptionDirectory(HWND hwnd, const PEContext& context);
void showDelayImportDirectory(HWND hwnd, const PEContext& context);
