#include "directoryDialogUi.h"
#include "resource.h"

#include <memory>
#include <vector>

namespace {
constexpr UINT_PTR kLayoutSubclass = 1;
constexpr UINT kSelectionCleared = WM_APP + 0x3D0;
const wchar_t kUpdating[] = L"PEViewer.DirectoryUpdating";
const wchar_t kColumnsFitted[] = L"PEViewer.DirectoryColumnsFitted";

struct ChildLayout {
  HWND window;
  RECT original;
  int left = 0, right = 100, top = 0, bottom = 0;
};

struct DialogLayout {
  SIZE client;
  SIZE minimum;
  std::vector<ChildLayout> children;
};

RECT ChildRect(HWND dialog, HWND child) {
  RECT rect = {};
  GetWindowRect(child, &rect);
  MapWindowPoints(NULL, dialog, (POINT*)&rect, 2);
  return rect;
}

bool ClassIs(HWND window, const wchar_t* expected) {
  wchar_t name[32] = {};
  GetClassName(window, name, _countof(name));
  return lstrcmpi(name, expected) == 0;
}

void ResizeChildren(HWND dialog, const DialogLayout& layout) {
  RECT client = {};
  GetClientRect(dialog, &client);
  int dx = client.right - layout.client.cx, dy = client.bottom - layout.client.cy;
  auto place = [dx, dy](const ChildLayout& child, HDWP batch) {
    int x = child.original.left + MulDiv(dx, child.left, 100);
    int y = child.original.top + MulDiv(dy, child.top, 100);
    int width = child.original.right + MulDiv(dx, child.right, 100) - x;
    int height = child.original.bottom + MulDiv(dy, child.bottom, 100) - y;
    if (batch) return DeferWindowPos(batch, child.window, NULL, x, y,
        width > 0 ? width : 1, height > 0 ? height : 1,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
    SetWindowPos(child.window, NULL, x, y, width > 0 ? width : 1, height > 0 ? height : 1,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
    return (HDWP)NULL;
  };
  HDWP batch = BeginDeferWindowPos((int)layout.children.size());
  if (batch) {
    for (const auto& child : layout.children) {
      batch = place(child, batch);
      if (!batch) break;
    }
    if (batch && EndDeferWindowPos(batch)) {
      RedrawWindow(dialog, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
      return;
    }
  }
  for (const auto& child : layout.children) place(child, NULL);
  RedrawWindow(dialog, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

LRESULT CALLBACK LayoutProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam,
                            UINT_PTR id, DWORD_PTR reference) {
  auto layout = (DialogLayout*)reference;
  if (message == WM_ERASEBKGND) {
    RECT client = {};
    GetClientRect(dialog, &client);
    FillRect((HDC)wParam, &client, GetSysColorBrush(COLOR_BTNFACE));
    return TRUE;
  }
  if (message == WM_CTLCOLORDLG) return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
  if (message == WM_CTLCOLORSTATIC && ClassIs((HWND)lParam, L"Static")) {
    HDC dc = (HDC)wParam;
    SetBkMode(dc, TRANSPARENT);
    SetBkColor(dc, GetSysColor(COLOR_BTNFACE));
    SetTextColor(dc, GetSysColor(IsWindowEnabled((HWND)lParam) ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
    return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
  }
  if (message == WM_SIZE && wParam != SIZE_MINIMIZED) ResizeChildren(dialog, *layout);
  if (message == WM_GETMINMAXINFO) {
    LRESULT result = DefSubclassProc(dialog, message, wParam, lParam);
    auto limits = (MINMAXINFO*)lParam;
    limits->ptMinTrackSize = {layout->minimum.cx, layout->minimum.cy};
    return result;
  }
  if (message == WM_NCDESTROY) {
    RemoveWindowSubclass(dialog, LayoutProc, id);
    delete layout;
  }
  return DefSubclassProc(dialog, message, wParam, lParam);
}
}  // namespace

void InitDirectoryDialogLayout(HWND dialog, DirectoryDialogLayout kind) {
  DWORD_PTR existing = 0;
  if (GetWindowSubclass(dialog, LayoutProc, kLayoutSubclass, &existing)) return;
  auto layout = std::make_unique<DialogLayout>();
  RECT client = {}, window = {};
  GetClientRect(dialog, &client);
  GetWindowRect(dialog, &window);
  layout->client = {client.right, client.bottom};
  layout->minimum = {window.right - window.left, window.bottom - window.top};

  int upperId = kind == DirectoryDialogLayout::Import ? IDC_LIST_IMPORT_DLL :
                kind == DirectoryDialogLayout::Relocations ? IDC_LIST_RELOC_BLOCKS :
                kind == DirectoryDialogLayout::Tls ? IDC_LIST_TLS_FIELDS :
                kind == DirectoryDialogLayout::LoadConfig ? IDC_LIST_LOAD_CONFIG_FIELDS :
                kind == DirectoryDialogLayout::DelayImport ? IDC_LIST_DELAY_MODULES :
                kind == DirectoryDialogLayout::Debug ? IDC_LIST_DEBUG_DIRECTORY : 0;
  int lowerId = kind == DirectoryDialogLayout::Import ? IDC_LIST_INT_THUNK :
                kind == DirectoryDialogLayout::Relocations ? IDC_LIST_RELOC_ENTRIES :
                kind == DirectoryDialogLayout::Tls ? IDC_LIST_TLS_CALLBACKS :
                kind == DirectoryDialogLayout::LoadConfig ? IDC_LIST_SAFE_SEH :
                kind == DirectoryDialogLayout::DelayImport ? IDC_LIST_DELAY_FUNCTIONS :
                kind == DirectoryDialogLayout::Debug ? IDC_LIST_DEBUG_DIRECTORY_INFO : 0;
  RECT upper = upperId ? ChildRect(dialog, GetDlgItem(dialog, upperId)) : RECT{};
  RECT resourceDetails = kind == DirectoryDialogLayout::Resources ?
      ChildRect(dialog, GetDlgItem(dialog, IDC_LIST_RESOURCE_DETAILS)) : RECT{};
  std::vector<HWND> groups;

  for (HWND child = GetWindow(dialog, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
    ChildLayout item = {child, ChildRect(dialog, child)};
    int id = GetDlgCtrlID(child);
    bool button = ClassIs(child, L"Button");
    bool group = button && (GetWindowLongPtr(child, GWL_STYLE) & BS_TYPEMASK) == BS_GROUPBOX;
    if (group) groups.push_back(child);
    bool list = ClassIs(child, WC_LISTVIEW);
    bool tree = ClassIs(child, WC_TREEVIEW);
    if (list) ListView_SetExtendedListViewStyleEx(child, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);
    if (button && !group) item.left = item.top = item.bottom = 100;
    else if (group || list || tree) item.bottom = 100;
    else if (id == IDC_STATIC_DIRECTORY_STATUS || id == IDC_STATIC_DELAY_FUNCTION_STATUS ||
             id == IDC_STATIC_SAFE_SEH_STATUS)
      item.top = item.bottom = 100;

    if (upperId) {
      if (id == upperId) item.bottom = 50;
      else if (id == lowerId) item.top = 50;
      else if (!button && item.original.top >= upper.bottom &&
               item.original.top < ChildRect(dialog, GetDlgItem(dialog, lowerId)).bottom)
        item.top = item.bottom = 50;
    }
    if (kind == DirectoryDialogLayout::Resources) {
      if (tree) item.right = 40;
      else if (!button && item.original.left >= resourceDetails.left) item.left = 40;
      else if (id != IDC_STATIC_DIRECTORY_STATUS && !button) item.right = 40;
    }
    if (kind == DirectoryDialogLayout::Export && id != IDC_LIST_EXPORT_FUNC &&
        id != IDC_STATIC_DIRECTORY_STATUS) {
      item.left = item.right = (button && !group) || id == IDC_GROUP_INFO ? 100 : 0;
      item.top = item.bottom = 0;
    }
    if (kind == DirectoryDialogLayout::Overview && !button)
      item.left = item.right = 50;
    layout->children.push_back(item);
  }
  // Group boxes are siblings of the controls they surround. Clipping their full
  // rectangles out of the parent also skips their transparent interiors, leaving
  // unpainted patches and copied list/header pixels after a resize.
  LONG_PTR style = GetWindowLongPtr(dialog, GWL_STYLE);
  SetWindowLongPtr(dialog, GWL_STYLE, groups.empty() ? style | WS_CLIPCHILDREN : style & ~WS_CLIPCHILDREN);
  for (HWND group : groups) {
    SetWindowLongPtr(group, GWL_STYLE, GetWindowLongPtr(group, GWL_STYLE) | WS_CLIPSIBLINGS);
    SetWindowPos(group, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  }
  if (SetWindowSubclass(dialog, LayoutProc, kLayoutSubclass, (DWORD_PTR)layout.get())) layout.release();
}

bool DirectorySelectionChanged(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam, int listId) {
  if (message == kSelectionCleared && wParam == (WPARAM)listId)
    return ListView_GetSelectedCount(GetDlgItem(dialog, listId)) == 0;
  if (message != WM_NOTIFY || !lParam) return false;
  auto change = (const NMLISTVIEW*)lParam;
  if (change->hdr.idFrom != (UINT_PTR)listId || change->hdr.code != LVN_ITEMCHANGED ||
      !(change->uChanged & LVIF_STATE) ||
      !((change->uOldState ^ change->uNewState) & LVIS_SELECTED)) return false;
  if (change->uNewState & LVIS_SELECTED) return true;
  // Switching rows deselects the old item before selecting the new one. Do not
  // clear and redraw the detail pane in between those two notifications.
  PostMessage(dialog, kSelectionCleared, listId, 0);
  return false;
}

void FitDirectoryColumns(HWND list, int maximumWidth) {
  bool fitted = GetProp(list, kColumnsFitted) != NULL;
  int count = Header_GetItemCount(ListView_GetHeader(list));
  for (int i = 0; i < count; ++i) {
    int previous = ListView_GetColumnWidth(list, i);
    ListView_SetColumnWidth(list, i, LVSCW_AUTOSIZE);
    int width = ListView_GetColumnWidth(list, i);
    wchar_t text[256] = {};
    LVCOLUMN column = {};
    column.mask = LVCF_TEXT;
    column.pszText = text;
    column.cchTextMax = _countof(text);
    ListView_GetColumn(list, i, &column);
    int heading = ListView_GetStringWidth(list, text) + 24;
    if (width < heading) width = heading;
    if (width > maximumWidth) width = maximumWidth;
    if (fitted && width < previous) width = previous;
    ListView_SetColumnWidth(list, i, width);
  }
  SetProp(list, kColumnsFitted, (HANDLE)1);
}

ScopedDirectoryUpdate::ScopedDirectoryUpdate(HWND control) {
  if (!control || GetProp(control, kUpdating) || !(GetWindowLongPtr(control, GWL_STYLE) & WS_VISIBLE)) return;
  if (!SetProp(control, kUpdating, (HANDLE)this)) return;
  control_ = control;
  list_ = ClassIs(control, WC_LISTVIEW);
  if (list_) {
    horizontal_ = GetScrollPos(control, SB_HORZ);
    header_ = ListView_GetHeader(control);
    if (header_) SendMessage(header_, WM_SETREDRAW, FALSE, 0);
  }
  SendMessage(control, WM_SETREDRAW, FALSE, 0);
}

ScopedDirectoryUpdate::~ScopedDirectoryUpdate() {
  if (!control_ || !IsWindow(control_)) return;
  if (list_) ListView_Scroll(control_, horizontal_ - GetScrollPos(control_, SB_HORZ), 0);
  if (header_ && IsWindow(header_)) SendMessage(header_, WM_SETREDRAW, TRUE, 0);
  SendMessage(control_, WM_SETREDRAW, TRUE, 0);
  RemoveProp(control_, kUpdating);
  RedrawWindow(control_, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ALLCHILDREN);
}
