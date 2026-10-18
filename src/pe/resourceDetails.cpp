#include "directoryDetails.h"

#include <cstring>
#include <set>
#include <utility>

namespace {
constexpr SIZE_T kRoot = (SIZE_T)-1;
constexpr SIZE_T kMaxNodes = 100000;
constexpr DWORD kMaxDepth = 32;
constexpr SIZE_T kMaxNameCharacters = 1024 * 1024;

// https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#the-rsrc-section
class ResourceReader {
 public:
  explicit ResourceReader(const PEContext& source) : context(source) {}

  ResourceDetails Read() {
    const auto directory = context.Directory(IMAGE_DIRECTORY_ENTRY_RESOURCE);
    if (!directory) {
      result.message = L"No resource directory.";
      return std::move(result);
    }
    result.present = true;
    result.rva = directory->VirtualAddress;
    result.size = directory->Size;
    if ((ULONGLONG)result.rva + result.size > (ULONGLONG)MAXDWORD + 1) {
      result.message = L"Resource directory RVA range overflows 32 bits.";
    } else {
      ReadDirectory(0, kRoot, 0);
    }
    return std::move(result);
  }

 private:
  const PEContext& context;
  ResourceDetails result;
  std::set<DWORD> activeDirectories;
  SIZE_T nameCharacters = 0;

  const BYTE* ReadRelative(DWORD offset, SIZE_T size) const {
    if (offset > result.size || size > result.size - offset || offset > MAXDWORD - result.rva)
      return NULL;
    return context.ReadRva(result.rva + offset, size);
  }

  void Problem(SIZE_T node, const wchar_t* message) {
    auto append = [&](std::wstring& text) {
      if (text.find(message) != std::wstring::npos) return;
      if (!text.empty()) text += L"; ";
      text += message;
    };
    if (node != kRoot) append(result.nodes[node].message);
    append(result.message);
  }

  void ReadName(SIZE_T index, DWORD value) {
    auto& node = result.nodes[index];
    node.named = (value & 0x80000000UL) != 0;
    if (!node.named) {
      node.id = value;
      return;
    }
    DWORD offset = value & 0x7FFFFFFFUL;
    const BYTE* data = ReadRelative(offset, sizeof(WORD));
    if (!data || offset % sizeof(WORD)) {
      Problem(index, L"Invalid or unaligned resource name offset");
      return;
    }
    WORD length = 0;
    std::memcpy(&length, data, sizeof(length));
    data = ReadRelative(offset, sizeof(WORD) + (SIZE_T)length * sizeof(WCHAR));
    if (!data) {
      Problem(index, L"Truncated resource name");
      return;
    }
    if (length > kMaxNameCharacters - nameCharacters) {
      Problem(index, L"Resource name character limit reached");
      return;
    }
    nameCharacters += length;
    node.name.resize(length);
    if (length) std::memcpy(&node.name[0], data + sizeof(WORD), (SIZE_T)length * sizeof(WCHAR));
    node.hasName = true;
  }

  void ReadData(SIZE_T index) {
    auto& node = result.nodes[index];
    const BYTE* data = ReadRelative(node.targetOffset, sizeof(node.data));
    if (!data) {
      Problem(index, L"Resource data entry is outside the directory/file range");
      return;
    }
    std::memcpy(&node.data, data, sizeof(node.data));
    node.hasData = true;
    ++result.resourceCount;
    // OffsetToData in this leaf record is an RVA, not a root-relative offset.
    node.hasFileOffset = context.RvaToFileOffset(node.data.OffsetToData, 1, node.fileOffset) != FALSE;
    SIZE_T offset = 0;
    node.dataValid = !node.data.Size ||
        context.RvaToFileOffset(node.data.OffsetToData, node.data.Size, offset) != FALSE;
    if (!node.dataValid) Problem(index, L"Resource data is outside the file-backed RVA range");
    if (node.data.Reserved) Problem(index, L"Resource data Reserved field is nonzero");
  }

  void ReadDirectory(DWORD offset, SIZE_T parent, DWORD depth) {
    if (depth >= kMaxDepth) {
      Problem(parent, L"Resource directory depth limit reached (32 levels)");
      return;
    }
    if (activeDirectories.count(offset)) {
      Problem(parent, L"Cyclic resource directory reference");
      return;
    }
    IMAGE_RESOURCE_DIRECTORY header = {};
    const BYTE* data = ReadRelative(offset, sizeof(header));
    if (!data) {
      Problem(parent, L"Resource directory header is outside the directory/file range");
      return;
    }
    std::memcpy(&header, data, sizeof(header));
    if (parent == kRoot) {
      result.hasRoot = true;
      result.root = header;
    } else {
      result.nodes[parent].hasDirectory = true;
      result.nodes[parent].directory = header;
    }
    activeDirectories.insert(offset);
    DWORD count = (DWORD)header.NumberOfNamedEntries + header.NumberOfIdEntries;
    for (DWORD i = 0; i < count; ++i) {
      if (result.nodes.size() >= kMaxNodes) {
        Problem(parent, L"Resource display limited to 100000 nodes");
        break;
      }
      DWORD position = offset + (DWORD)(sizeof(header) + (SIZE_T)i * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY));
      IMAGE_RESOURCE_DIRECTORY_ENTRY entry = {};
      data = ReadRelative(position, sizeof(entry));
      if (!data) {
        Problem(parent, L"Truncated resource directory entries");
        break;
      }
      std::memcpy(&entry, data, sizeof(entry));
      SIZE_T index = result.nodes.size();
      ResourceNode node;
      node.parent = parent;
      node.depth = depth;
      node.entryOffset = position;
      node.isDirectory = (entry.OffsetToData & 0x80000000UL) != 0;
      node.targetOffset = entry.OffsetToData & 0x7FFFFFFFUL;
      result.nodes.push_back(std::move(node));
      ReadName(index, entry.Name);
      if (result.nodes[index].named != (i < header.NumberOfNamedEntries))
        Problem(index, L"Resource name/ID entry order mismatch");
      if (result.nodes[index].isDirectory)
        ReadDirectory(result.nodes[index].targetOffset, index, depth + 1);
      else
        ReadData(index);
    }
    activeDirectories.erase(offset);
  }
};
}  // namespace

ResourceDetails ReadResourceDetails(const PEContext& context) {
  return ResourceReader(context).Read();
}
