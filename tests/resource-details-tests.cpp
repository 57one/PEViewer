#include "pe/directoryDetails.h"
#include "fixtures/resources.h"

#include <stdexcept>
#include <vector>

namespace {
void Check(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}

void CheckFormat(bool wide) {
  std::vector<BYTE> image(pe32_fixture::kFileSize);
  PEContext context;
  auto reset = [&]() {
    resource_fixture::Fill(image.data(), wide);
    Check(context.Initialize(image.data(), image.size()), "Resource fixture rejected");
  };
  reset();
  auto directory = const_cast<IMAGE_DATA_DIRECTORY*>(context.Directory(IMAGE_DIRECTORY_ENTRY_RESOURCE));
  BYTE* base = image.data() + resource_fixture::kRaw;
  auto firstData = (PIMAGE_RESOURCE_DATA_ENTRY)(base + 0xE0);
  auto result = ReadResourceDetails(context);
  Check(result.present && result.hasRoot && result.message.empty() && result.nodes.size() == 7 &&
        result.resourceCount == 3, "Resource tree size or status mismatch");
  Check(result.nodes[0].named && result.nodes[0].hasName &&
        result.nodes[0].name == L"\u81EA\u5B9A\u4E49\u7C7B\u578B" &&
        result.nodes[1].name == L"\u914D\u7F6E" && result.nodes[1].parent == 0,
        "Root-relative UTF-16 names or parent identity mismatch");
  Check(result.nodes[2].parent == 1 && result.nodes[2].id == 0x409 && result.nodes[3].id == 0x804 &&
        result.nodes[4].id == 10 && result.nodes[5].id == 101 && result.nodes[6].parent == 5,
        "Resource type/name/language hierarchy mismatch");
  const auto& leaf = result.nodes[2];
  Check(leaf.hasData && leaf.dataValid && leaf.hasFileOffset && leaf.data.OffsetToData == 0x1D00 &&
        leaf.fileOffset == 0xF00 && leaf.data.Size == 4 && leaf.data.CodePage == 1200,
        "Resource payload RVA was treated as a directory-relative offset");

  resource_fixture::Name(base, 0x1A0, L"A\0B", 3);
  result = ReadResourceDetails(context);
  Check(result.nodes[1].name.size() == 3 && result.nodes[1].name[1] == 0 &&
        result.nodes[1].name[2] == L'B', "Resource name incorrectly requires a null terminator");
  *(WORD*)(base + 0x1A0) = MAXWORD;
  result = ReadResourceDetails(context);
  Check(!result.nodes[1].hasName && !result.nodes[1].message.empty() && result.resourceCount == 3,
        "Truncated resource name was read or discarded its children");
  reset();
  resource_fixture::Entry(base, 0x40, 0x800001FF, 0x80000060);
  Check(!ReadResourceDetails(context).nodes[1].hasName, "Unaligned/truncated name offset accepted");

  reset();
  resource_fixture::Entry(base, 0x10, 0x80000180, 0x80000000);
  result = ReadResourceDetails(context);
  Check(result.resourceCount == 1 && result.message.find(L"Cyclic") != std::wstring::npos,
        "Resource cycle was followed or valid sibling was lost");
  reset();
  resource_fixture::Entry(base, 0x18, 10, 0x80000030);
  result = ReadResourceDetails(context);
  Check(result.message.empty() && result.resourceCount == 4, "Shared subtree mistaken for a cycle");
  reset();
  resource_fixture::Entry(base, 0x10, 0x80000180, 0x80000200);
  result = ReadResourceDetails(context);
  Check(!result.nodes[0].hasDirectory && !result.message.empty() && result.resourceCount == 1,
        "Out-of-range subdirectory read or valid sibling lost");

  reset();
  resource_fixture::Entry(base, 0x70, 0x409, resource_fixture::kSize - 8);
  result = ReadResourceDetails(context);
  Check(!result.nodes[2].hasData && !result.nodes[2].message.empty() && result.resourceCount == 2,
        "Truncated resource data descriptor accepted");
  reset();
  firstData->OffsetToData = 0x1FFF;
  result = ReadResourceDetails(context);
  Check(result.nodes[2].hasData && result.nodes[2].hasFileOffset && !result.nodes[2].dataValid &&
        !result.nodes[2].message.empty(), "Truncated resource payload accepted");
  firstData->OffsetToData = MAXDWORD;
  Check(!ReadResourceDetails(context).nodes[2].dataValid, "Resource payload RVA overflow accepted");
  firstData->Size = 0;
  result = ReadResourceDetails(context);
  Check(result.message.empty() && result.nodes[2].dataValid, "Zero-length resource rejected");
  firstData->Reserved = 1;
  Check(!ReadResourceDetails(context).nodes[2].message.empty(), "Nonzero resource Reserved not reported");

  reset();
  directory->Size = 31;
  result = ReadResourceDetails(context);
  Check(result.hasRoot && result.message.find(L"Truncated resource directory entries") != std::wstring::npos,
        "Incomplete resource entry array not reported");
  directory->Size = 15;
  Check(!ReadResourceDetails(context).hasRoot, "Truncated resource root accepted");
  reset();
  directory->VirtualAddress = 0x3000;
  Check(!ReadResourceDetails(context).hasRoot, "Unmapped resource root read");
  directory->VirtualAddress = MAXDWORD;
  Check(!ReadResourceDetails(context).message.empty(), "Resource directory RVA range overflow accepted");

  reset();
  directory->Size = 0x400;
  std::memset(base, 0, 0x400);
  for (DWORD i = 0; i < 32; ++i) {
    resource_fixture::Directory(base, i * 24, 0, 1);
    resource_fixture::Entry(base, i * 24 + 16, i, 0x80000000UL | ((i + 1) * 24));
  }
  result = ReadResourceDetails(context);
  Check(result.nodes.size() == 32 && result.message.find(L"depth limit") != std::wstring::npos,
        "Deep resource tree was not bounded");
  reset();
  resource_fixture::Directory(base, 0, 0, 0);
  result = ReadResourceDetails(context);
  Check(result.hasRoot && result.nodes.empty() && result.message.empty(), "Empty resource root rejected");
  *directory = {};
  result = ReadResourceDetails(context);
  Check(!result.present && result.nodes.empty(), "Missing resource directory became present");
}
}  // namespace

void CheckResourceDetails() {
  CheckFormat(false);
  CheckFormat(true);
  // A tiny shared graph must not expand into an unbounded model or duplicate huge names.
  std::vector<BYTE> image(pe32_fixture::kFileSize);
  resource_fixture::Fill(image.data(), false);
  PEContext context;
  Check(context.Initialize(image.data(), image.size()), "Resource limit fixture rejected");
  auto directory = const_cast<IMAGE_DATA_DIRECTORY*>(context.Directory(IMAGE_DIRECTORY_ENTRY_RESOURCE));
  directory->Size = 0x400;
  BYTE* base = image.data() + resource_fixture::kRaw;
  std::memset(base, 0, 0x400);
  for (DWORD level = 0; level <= 16; ++level) {
    resource_fixture::Directory(base, level * 32, level == 16 ? 1 : 2, 0);
    DWORD target = level == 16 ? 0x3F0 : 0x80000000UL | ((level + 1) * 32);
    resource_fixture::Entry(base, level * 32 + 16, 0x80000240, target);
    if (level != 16) resource_fixture::Entry(base, level * 32 + 24, 0x80000240, target);
  }
  std::wstring name(50, L'N');
  resource_fixture::Name(base, 0x240, name.c_str(), 50);
  auto result = ReadResourceDetails(context);
  Check(result.nodes.size() == 100000 && result.message.find(L"100000 nodes") != std::wstring::npos &&
        result.message.find(L"name character limit") != std::wstring::npos,
        "Shared resource graph bypassed node or name-memory limits");
}
