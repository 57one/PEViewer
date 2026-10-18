#include "windowProc.h"
#include "pe/directoryDetails.h"

#include <memory>
#include <string>
#include <vector>

namespace {
std::wstring Hex(ULONGLONG value, int width = 8) {
  wchar_t text[32] = {};
  swprintf_s(text, L"%0*I64X", width, value);
  return text;
}

const wchar_t* TypeName(DWORD id) {
  switch (id) {
    case 1: return L"CURSOR";
    case 2: return L"BITMAP";
    case 3: return L"ICON";
    case 4: return L"MENU";
    case 5: return L"DIALOG";
    case 6: return L"STRING";
    case 7: return L"FONTDIR";
    case 8: return L"FONT";
    case 9: return L"ACCELERATOR";
    case 10: return L"RCDATA";
    case 11: return L"MESSAGETABLE";
    case 12: return L"GROUP_CURSOR";
    case 14: return L"GROUP_ICON";
    case 16: return L"VERSION";
    case 17: return L"DLGINCLUDE";
    case 19: return L"PLUGPLAY";
    case 20: return L"VXD";
    case 21: return L"ANICURSOR";
    case 22: return L"ANIICON";
    case 23: return L"HTML";
    case 24: return L"MANIFEST";
    default: return NULL;
  }
}

std::wstring DisplayName(const ResourceNode& node) {
  if (!node.named) return L"#" + std::to_wstring(node.id);
  if (!node.hasName) return L"<invalid name>";
  std::wstring text = L"\"";
  // Resource names are length-prefixed UTF-16, so embedded nulls must remain visible.
  for (wchar_t c : node.name) {
    if (c == L'"' || c == L'\\') { text += L'\\'; text += c; }
    else if (c < L' ') text += L"\\u" + Hex(c, 4);
    else text += c;
  }
  return text + L"\"";
}

std::wstring Label(const ResourceNode& node) {
  std::wstring prefix = node.depth == 0 ? L"Type: " : node.depth == 1 ? L"Name: " :
                        node.depth == 2 ? L"Language: " : L"Level " + std::to_wstring(node.depth + 1) + L": ";
  const wchar_t* type = node.depth == 0 && !node.named ? TypeName(node.id) : NULL;
  std::wstring text = prefix + (type ? std::wstring(type) + L" (" + DisplayName(node) + L")" : DisplayName(node));
  if (node.depth == 2 && !node.named) text += L" (0x" + Hex(node.id, 4) + L")";
  if (!node.message.empty()) text += L" [!]";
  return text;
}

void Field(HWND list, const wchar_t* name, const std::wstring& value) {
  LVITEM item = {};
  item.mask = LVIF_TEXT;
  item.iItem = ListView_GetItemCount(list);
  item.pszText = const_cast<wchar_t*>(name);
  int row = ListView_InsertItem(list, &item);
  ListView_SetItemText(list, row, 1, const_cast<wchar_t*>(value.c_str()));
}

void HeaderFields(HWND list, const IMAGE_RESOURCE_DIRECTORY& header) {
  Field(list, L"Characteristics (hex)", Hex(header.Characteristics));
  Field(list, L"TimeDateStamp (hex)", Hex(header.TimeDateStamp));
  Field(list, L"MajorVersion (hex)", Hex(header.MajorVersion, 4));
  Field(list, L"MinorVersion (hex)", Hex(header.MinorVersion, 4));
  Field(list, L"Named entries", std::to_wstring(header.NumberOfNamedEntries));
  Field(list, L"ID entries", std::to_wstring(header.NumberOfIdEntries));
}

void ShowSelection(HWND hwnd) {
  HWND list = GetDlgItem(hwnd, IDC_LIST_RESOURCE_DETAILS);
  ListView_DeleteAllItems(list);
  auto details = (const ResourceDetails*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  if (!details) return;
  TVITEM item = {};
  item.mask = TVIF_PARAM;
  item.hItem = TreeView_GetSelection(GetDlgItem(hwnd, IDC_TREE_RESOURCES));
  if (!item.hItem || !TreeView_GetItem(GetDlgItem(hwnd, IDC_TREE_RESOURCES), &item)) return;
  if (!item.lParam) {
    Field(list, L"Directory RVA (hex)", Hex(details->rva));
    Field(list, L"Directory size (hex)", Hex(details->size));
    if (details->hasRoot) HeaderFields(list, details->root);
    Field(list, L"Resource entries", std::to_wstring(details->resourceCount));
    Field(list, L"Status", details->message.empty() ? L"OK" : details->message);
    return;
  }
  SIZE_T index = (SIZE_T)item.lParam - 1;
  if (index >= details->nodes.size()) return;
  const auto& node = details->nodes[index];
  std::wstring path;
  for (SIZE_T current = index; current != (SIZE_T)-1; current = details->nodes[current].parent)
    path = DisplayName(details->nodes[current]) + (path.empty() ? L"" : L" / " + path);
  Field(list, L"Path", path);
  Field(list, L"Kind", node.isDirectory ? L"Directory" : L"Resource data");
  Field(list, L"Entry offset (hex)", Hex(node.entryOffset));
  Field(list, L"Target offset (hex)", Hex(node.targetOffset));
  if (node.hasDirectory) HeaderFields(list, node.directory);
  if (node.hasData) {
    Field(list, L"Data RVA (hex)", Hex(node.data.OffsetToData));
    Field(list, L"File offset (hex)", node.hasFileOffset ? Hex(node.fileOffset) : L"-");
    Field(list, L"Size (hex)", Hex(node.data.Size));
    Field(list, L"CodePage", std::to_wstring(node.data.CodePage));
    Field(list, L"Reserved (hex)", Hex(node.data.Reserved));
    Field(list, L"Data range", node.dataValid ? L"Valid" : L"Invalid");
  }
  Field(list, L"Status", node.message.empty() ? L"OK" : node.message);
}

HTREEITEM Insert(HWND tree, HTREEITEM parent, const std::wstring& text, LPARAM id) {
  TVINSERTSTRUCT item = {};
  item.hParent = parent;
  item.hInsertAfter = TVI_LAST;
  item.item.mask = TVIF_TEXT | TVIF_PARAM;
  item.item.pszText = const_cast<wchar_t*>(text.c_str());
  item.item.lParam = id;
  return TreeView_InsertItem(tree, &item);
}
}  // namespace

void showResourceDirectory(HWND hwnd, const PEContext& context) {
  auto details = std::make_unique<ResourceDetails>(ReadResourceDetails(context));
  HWND tree = GetDlgItem(hwnd, IDC_TREE_RESOURCES);
  auto previous = (ResourceDetails*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
  TreeView_DeleteAllItems(tree);
  ListView_DeleteAllItems(GetDlgItem(hwnd, IDC_LIST_RESOURCE_DETAILS));
  delete previous;
  SetWindowText(hwnd, context.format == PEFormat::PE32Plus ? L"Resource Directory - PE32+" : L"Resource Directory - PE32");
  SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)details.get());
  SendMessage(tree, WM_SETREDRAW, FALSE, 0);
  if (details->hasRoot) {
    HTREEITEM root = Insert(tree, TVI_ROOT, L"Resources", 0);
    std::vector<HTREEITEM> handles(details->nodes.size(), NULL);
    for (SIZE_T i = 0; i < details->nodes.size(); ++i) {
      const auto& node = details->nodes[i];
      HTREEITEM parent = node.parent == (SIZE_T)-1 ? root : handles[node.parent];
      if (parent) handles[i] = Insert(tree, parent, Label(node), (LPARAM)(i + 1));
    }
    TreeView_Expand(tree, root, TVE_EXPAND);
    TreeView_SelectItem(tree, root);
  }
  SendMessage(tree, WM_SETREDRAW, TRUE, 0);
  InvalidateRect(tree, NULL, TRUE);
  std::wstring status = std::to_wstring(details->resourceCount) + L" resource entries, " +
      std::to_wstring(details->nodes.size()) + L" nodes. " + details->message;
  SetDlgItemText(hwnd, IDC_STATIC_DIRECTORY_STATUS, status.c_str());
  details.release();  // Owned by this dialog until reload or WM_DESTROY.
}

INT_PTR CALLBACK ResourceDirectoryProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      HWND list = GetDlgItem(hwnd, IDC_LIST_RESOURCE_DETAILS);
      ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
      const wchar_t* columns[] = {L"Field", L"Value"};
      for (int i = 0; i < 2; ++i) {
        LVCOLUMN column = {};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.pszText = const_cast<wchar_t*>(columns[i]);
        column.cx = i ? 350 : 160;
        ListView_InsertColumn(list, i, &column);
      }
      showResourceDirectory(hwnd, g_peContext);
      return TRUE;
    }
    case WM_NOTIFY: {
      auto notification = (const NMHDR*)lParam;
      if (notification && notification->idFrom == IDC_TREE_RESOURCES && notification->code == TVN_SELCHANGED) {
        ShowSelection(hwnd);
        return TRUE;
      }
      break;
    }
    case WM_COMMAND:
      if (LOWORD(wParam) != IDOK && LOWORD(wParam) != IDCANCEL) break;
      EndDialog(hwnd, 0);
      return TRUE;
    case WM_CLOSE: EndDialog(hwnd, 0); return TRUE;
    case WM_DESTROY:
      delete (ResourceDetails*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
      SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
      return TRUE;
  }
  return FALSE;
}
