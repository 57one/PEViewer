#include "messageCallback.h"

void onSize(HWND mainDialogHwnd, LPSIZE mainDialogSize, WPARAM wParam,
            LPARAM lParam) {
  switch (wParam) {
    case SIZE_MAXHIDE:
    case SIZE_MAXSHOW:
    case SIZE_MINIMIZED:
      return;
  }
  int nWidth = LOWORD(lParam);
  int nHeight = HIWORD(lParam);

  // get difference
  int x, y;
  x = nWidth - (*mainDialogSize).cx;
  y = nHeight - (*mainDialogSize).cy;
  HDWP hdwp = BeginDeferWindowPos(7);
  if (hdwp == NULL) return;
  hdwp = moveWidget(mainDialogHwnd, hdwp, IDC_GROUP_LIST, x, y, FALSE, FALSE,
                    TRUE, TRUE);
  hdwp = moveWidget(mainDialogHwnd, hdwp, IDC_LIST_PROCESS, x, y, FALSE, FALSE,
                    TRUE, TRUE);
  hdwp = moveWidget(mainDialogHwnd, hdwp, IDC_LIST_MODULE, x, y, FALSE, TRUE,
                    TRUE, TRUE);
  hdwp = moveWidget(mainDialogHwnd, hdwp, IDC_GROUP_BUTTON, x, y, TRUE, FALSE,
                    TRUE, TRUE);
  hdwp = moveWidget(mainDialogHwnd, hdwp, IDC_BUTTON_PEVIEWER, x, y, TRUE,
                    FALSE, TRUE, FALSE);
  hdwp = moveWidget(mainDialogHwnd, hdwp, IDC_BUTTON_PROFILE, x, y, TRUE, TRUE,
                    TRUE, TRUE);
  hdwp = moveWidget(mainDialogHwnd, hdwp, IDC_BUTTON_QUIT, x, y, TRUE, TRUE,
                    TRUE, TRUE);
  EndDeferWindowPos(hdwp);
  mainDialogSize->cx = nWidth;
  mainDialogSize->cy = nHeight;
}

void onGetMinMaxInfo(MINMAXINFO* lpMinMaxInfo) {
  lpMinMaxInfo->ptMinTrackSize.x = 630;
  lpMinMaxInfo->ptMinTrackSize.y = 300;
}