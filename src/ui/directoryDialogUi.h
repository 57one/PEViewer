#pragma once

#include <windows.h>
#include <commctrl.h>

enum class DirectoryDialogLayout {
  Overview, Import, Export, BoundImport, Iat, Relocations, Resources, Tls, DelayImport, Debug
};

// Captures the resource layout once; resize state is independent of GWLP_USERDATA.
void InitDirectoryDialogLayout(HWND dialog, DirectoryDialogLayout layout);

// Refresh on selection, and defer deselection until a possible new selection is known.
bool DirectorySelectionChanged(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam, int listId);

// Fit once, then only grow columns so switching rows preserves user widths.
void FitDirectoryColumns(HWND list, int maximumWidth = 480);

// Batch changes to a control and its header, retaining the horizontal scroll position.
// Nested scopes and controls in hidden dialogs keep their original visibility.
class ScopedDirectoryUpdate {
 public:
  explicit ScopedDirectoryUpdate(HWND control);
  ~ScopedDirectoryUpdate();
  ScopedDirectoryUpdate(const ScopedDirectoryUpdate&) = delete;
  ScopedDirectoryUpdate& operator=(const ScopedDirectoryUpdate&) = delete;

 private:
  HWND control_ = NULL;
  HWND header_ = NULL;
  bool list_ = false;
  int horizontal_ = 0;
};
