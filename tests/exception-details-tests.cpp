#include "pe/directoryDetails.h"
#include "fixtures/exceptions.h"

#include <stdexcept>
#include <vector>

namespace {
void Check(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}
}

void CheckExceptionDetails() {
  PEContext empty;
  Check(!ReadExceptionDetails(empty).present, "Empty context has exception records");
  std::vector<BYTE> image(pe32_fixture::kFileSize);
  PEContext context;
  auto reset = [&]() {
    exception_fixture::Fill(image.data());
    Check(context.Initialize(image.data(), image.size()), "Exception fixture rejected");
  };
  reset();
  auto directory = const_cast<IMAGE_DATA_DIRECTORY*>(context.Directory(IMAGE_DIRECTORY_ENTRY_EXCEPTION));
  auto nt = (PIMAGE_NT_HEADERS64)(image.data() + 0x80);
  auto result = ReadExceptionDetails(context);
  Check(result.present && result.supported && result.complete && result.entries.size() == 3 &&
        result.totalEntries == 3 && result.message.empty(), "AMD64 exception table count/status mismatch");
  const auto& first = result.entries[0];
  Check(first.recordRva == 0x1900 && first.recordFileOffset == 0xB00 &&
        first.beginRva == 0x1800 && first.endRva == 0x1810 && first.unwindRva == 0x1A00 &&
        first.rangeValid && first.hasCodeFileOffset && first.codeFileOffset == 0xA00 &&
        first.hasUnwindFileOffset && first.unwindFileOffset == 0xC00 &&
        result.entries[1].recordRva == 0x190C && result.entries[2].recordFileOffset == 0xB18,
        "Exception records must use three DWORD RVAs, not host pointers or VAs");

  exception_fixture::Record(image.data(), 0, 0x1800, 0x1800, 0);
  result = ReadExceptionDetails(context);
  Check(result.entries.size() == 3 && !result.entries[0].rangeValid &&
        !result.entries[0].message.empty() && result.entries[1].message.empty(),
        "Invalid exception row discarded a valid sibling");
  exception_fixture::Record(image.data(), 0, 0, 0, 0);
  Check(ReadExceptionDetails(context).entries.size() == 3, "Zero exception record mistaken for a terminator");
  exception_fixture::Record(image.data(), 0, 0x1810, 0x1800, 0x1A00);
  Check(!ReadExceptionDetails(context).entries[0].rangeValid, "Reversed exception range accepted");
  reset();
  exception_fixture::Record(image.data(), 1, 0x1700, 0x1710, 0x1A04);
  Check(ReadExceptionDetails(context).entries[1].message.find(L"sorted") != std::wstring::npos,
        "Unsorted exception records not reported");
  reset();
  exception_fixture::Record(image.data(), 2, 0x1FF0, 0x2000, 0x1FFC);
  result = ReadExceptionDetails(context);
  Check(result.entries[2].message.empty() && result.entries[2].rangeValid &&
        result.entries[2].hasUnwindFileOffset, "Exclusive image/file end incorrectly rejected");
  exception_fixture::Record(image.data(), 2, 0x1FF0, 0x2001, MAXDWORD);
  Check(!ReadExceptionDetails(context).entries[2].rangeValid, "Out-of-image function range accepted");
  exception_fixture::Record(image.data(), 2, 0x1840, 0x1860, 0x1A01);
  Check(!ReadExceptionDetails(context).entries[2].hasUnwindFileOffset, "Unaligned unwind reference accepted");
  exception_fixture::Record(image.data(), 2, 0x1840, 0x1860, 0x2000);
  Check(!ReadExceptionDetails(context).entries[2].hasUnwindFileOffset, "Unwind header beyond image accepted");

  reset();
  nt->OptionalHeader.SizeOfImage = 0x3000;
  const_cast<IMAGE_SECTION_HEADER*>(context.sectionHeaders)->Misc.VirtualSize = 0x2000;
  exception_fixture::Record(image.data(), 2, 0x1FF0, 0x2010, 0x2000);
  result = ReadExceptionDetails(context);
  Check(result.entries[2].rangeValid && result.entries[2].hasCodeFileOffset &&
        !result.entries[2].hasUnwindFileOffset && !result.entries[2].message.empty(),
        "Virtual zero-fill was treated as file-backed code/unwind data");
  exception_fixture::Record(image.data(), 2, 0x2100, 0x2120, 0x1A00);
  Check(!ReadExceptionDetails(context).entries[2].hasCodeFileOffset, "Unmapped code has a file offset");

  reset();
  directory->Size = exception_fixture::kSize + 1;
  result = ReadExceptionDetails(context);
  Check(!result.complete && result.entries.size() == 3 && !result.message.empty(), "Partial trailing exception record ignored");
  directory->Size = 11;
  Check(ReadExceptionDetails(context).entries.empty() && !ReadExceptionDetails(context).complete,
        "Short exception record read");
  reset();
  std::memmove(image.data() + exception_fixture::kRaw + 1, image.data() + exception_fixture::kRaw,
               exception_fixture::kSize);
  ++directory->VirtualAddress;
  result = ReadExceptionDetails(context);
  Check(result.entries.size() == 3 && result.message.find(L"aligned") != std::wstring::npos,
        "Unaligned directory was silently accepted or unsafe casts used");

  reset();
  const DWORD finalRecord[] = {0x1800, 0x1810, 0x1A00};
  std::memcpy(image.data() + image.size() - 12, finalRecord, sizeof(finalRecord));
  directory->VirtualAddress = 0x1FF4;
  directory->Size = 24;
  result = ReadExceptionDetails(context);
  Check(result.entries.size() == 1 && !result.complete, "Truncated table lost a readable prefix or overread EOF");
  directory->VirtualAddress = MAXDWORD - 3;
  Check(!ReadExceptionDetails(context).complete && ReadExceptionDetails(context).entries.empty(),
        "Exception directory RVA overflow accepted");
  reset();
  directory->VirtualAddress = 0x3000;
  Check(ReadExceptionDetails(context).entries.empty(), "Unmapped exception directory dereferenced");
  *directory = {};
  Check(!ReadExceptionDetails(context).present, "Missing exception directory became present");

  reset();
  for (WORD machine : {IMAGE_FILE_MACHINE_ARM64, IMAGE_FILE_MACHINE_I386, IMAGE_FILE_MACHINE_UNKNOWN}) {
    nt->FileHeader.Machine = machine;
    result = ReadExceptionDetails(context);
    Check(result.present && !result.supported && result.entries.empty() && !result.message.empty(),
          "Non-AMD64 exception data decoded using AMD64 layout");
  }
  std::vector<BYTE> pe32Image(pe32_fixture::kFileSize);
  exception_fixture::Fill(pe32Image.data(), false);
  PEContext pe32;
  Check(pe32.Initialize(pe32Image.data(), pe32Image.size()), "PE32 exception fixture rejected");
  auto pe32Nt = (PIMAGE_NT_HEADERS32)(pe32Image.data() + 0x80);
  pe32Nt->FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
  Check(!ReadExceptionDetails(pe32).supported, "AMD64/PE32 mismatch accepted");
  reset();
  const auto saved = ReadExceptionDetails(context);
  ReadExceptionDetails(pe32);
  *const_cast<IMAGE_DATA_DIRECTORY*>(pe32.Directory(IMAGE_DIRECTORY_ENTRY_EXCEPTION)) = {};
  Check(!ReadExceptionDetails(pe32).present && ReadExceptionDetails(context).entries.size() == 3 &&
        saved.entries.size() == 3 && saved.entries[0].beginRva == 0x1800,
        "Exception contexts or returned results share mutable state");

  constexpr DWORD count = 100001;
  image.resize(0x1000 + (SIZE_T)count * 12);
  exception_fixture::Fill(image.data());
  nt = (PIMAGE_NT_HEADERS64)(image.data() + 0x80);
  auto section = IMAGE_FIRST_SECTION(nt);
  section->SizeOfRawData = (DWORD)image.size() - pe32_fixture::kSectionRaw;
  section->Misc.VirtualSize = section->SizeOfRawData;
  nt->OptionalHeader.SizeOfImage = pe32_fixture::kSectionRva + section->SizeOfRawData;
  nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION] = {0x1E00, count * 12};
  for (DWORD i = 0; i < count; ++i) std::memcpy(image.data() + 0x1000 + (SIZE_T)i * 12, finalRecord, 12);
  Check(context.Initialize(image.data(), image.size()), "Large exception fixture rejected");
  result = ReadExceptionDetails(context);
  Check(result.entries.size() == 100000 && !result.complete && result.message.find(L"100000") != std::wstring::npos,
        "Exception entry limit bypassed");
}
