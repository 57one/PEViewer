#include "pe/directoryDetails.h"
#include "fixtures/tls.h"

#include <stdexcept>
#include <vector>

namespace {
void Check(bool value, const char* message) {
  if (!value) throw std::runtime_error(message);
}

void CheckFormat(bool wide) {
  std::vector<BYTE> image(pe32_fixture::kFileSize);
  PEContext context;
  ULONGLONG base = wide ? pe64_fixture::kImageBase : pe32_fixture::kImageBase;
  SIZE_T width = wide ? 8 : 4;
  SIZE_T headerSize = wide ? sizeof(IMAGE_TLS_DIRECTORY64) : sizeof(IMAGE_TLS_DIRECTORY32);
  auto reset = [&]() {
    tls_fixture::Fill(image.data(), wide);
    Check(context.Initialize(image.data(), image.size()), "TLS fixture rejected");
  };
  auto field = [&](int index, ULONGLONG value) {
    tls_fixture::WriteValue(image.data() + tls_fixture::kRaw + index * width, value, wide);
  };
  auto slot = [&](SIZE_T index, ULONGLONG value) {
    tls_fixture::WriteValue(image.data() + tls_fixture::kCallbacksRaw + index * width, value, wide);
  };
  reset();
  auto directory = const_cast<IMAGE_DATA_DIRECTORY*>(context.Directory(IMAGE_DIRECTORY_ENTRY_TLS));
  auto result = ReadTlsDetails(context);
  Check(result.present && result.hasHeader && result.terminated && result.message.empty() &&
        result.callbacks.size() == 2, "TLS header, callback count or status mismatch");
  Check(result.imageBase == base && result.rawDataStart.va == base + tls_fixture::kDataRva &&
        result.rawDataEnd.va == base + tls_fixture::kDataRva + 16 &&
        result.rawDataValid && result.rawDataSize == 16 && result.sizeOfZeroFill == 32 &&
        result.characteristics == IMAGE_SCN_ALIGN_16BYTES, "TLS fields lost values or widths");
  Check(result.index.rva == tls_fixture::kIndexRva && result.index.hasFileOffset &&
        result.callbackArray.rva == tls_fixture::kCallbacksRva && result.callbackArray.fileOffset == 0xB80,
        "TLS VAs were treated as RVAs or buffer addresses");
  Check(result.callbacks[0].slotRva == tls_fixture::kCallbacksRva &&
        result.callbacks[1].slotRva == tls_fixture::kCallbacksRva + width &&
        result.callbacks[1].target.va == base + 0x1810 && result.callbacks[1].target.rva == 0x1810 &&
        result.callbacks[1].target.fileOffset == 0xA10, "TLS array stride or target conversion mismatch");

  slot(0, 0);
  result = ReadTlsDetails(context);
  Check(result.terminated && result.callbacks.empty() && result.message.empty(), "Empty TLS callback array rejected");
  field(3, 0);
  Check(ReadTlsDetails(context).terminated, "Absent callback array is not an empty list");
  field(0, 0); field(1, 0);
  result = ReadTlsDetails(context);
  Check(result.rawDataValid && result.rawDataSize == 0 && result.message.empty(), "TLS without an initialization template rejected");

  reset();
  field(0, base + 0x2000); field(1, base + 0x2000);
  result = ReadTlsDetails(context);
  Check(result.rawDataValid && result.rawDataSize == 0, "Empty TLS template at image end rejected");
  field(0, base + 0x1FF0);
  Check(ReadTlsDetails(context).rawDataValid, "Exclusive TLS template end treated as a data byte");
  field(0, base + 0x1A10); field(1, base + 0x1A00);
  Check(!ReadTlsDetails(context).message.empty(), "Reversed TLS template range accepted");
  field(0, base + 0x1FF0); field(1, base + 0x2010);
  Check(!ReadTlsDetails(context).rawDataValid, "TLS template outside image accepted");

  reset();
  // File-backed data ends at RVA 2000; the rest of the section exists only in memory.
  const_cast<IMAGE_SECTION_HEADER*>(context.sectionHeaders)->Misc.VirtualSize = 0x2000;
  if (wide) ((PIMAGE_NT_HEADERS64)(image.data() + 0x80))->OptionalHeader.SizeOfImage = 0x3000;
  else ((PIMAGE_NT_HEADERS32)(image.data() + 0x80))->OptionalHeader.SizeOfImage = 0x3000;
  field(2, base + 0x2000);
  result = ReadTlsDetails(context);
  Check(result.index.hasRva && !result.index.hasFileOffset && result.message.empty(), "TLS index in zero-fill memory rejected");
  field(0, base + 0x2000); field(1, base + 0x2010);
  Check(!ReadTlsDetails(context).rawDataValid, "TLS initialization template in zero-fill bytes accepted");
  field(3, base + 0x2000);
  result = ReadTlsDetails(context);
  Check(!result.terminated && result.callbacks.empty() && !result.message.empty(), "Callback array without disk bytes accepted");

  reset();
  slot(0, base - 1);
  result = ReadTlsDetails(context);
  Check(result.callbacks.size() == 2 && result.terminated && !result.callbacks[0].target.hasRva &&
        !result.callbacks[0].target.message.empty() && result.callbacks[1].target.hasFileOffset,
        "Invalid callback target lost its row or valid sibling");
  slot(0, base + 0x3000);
  Check(!ReadTlsDetails(context).callbacks[0].target.message.empty(), "Callback outside image accepted");
  if (wide) {
    slot(0, base + 0x100000000ULL);
    Check(!ReadTlsDetails(context).callbacks[0].target.hasRva, "64-bit callback RVA was truncated");
    slot(0, ~0ULL);
    Check(!ReadTlsDetails(context).callbacks[0].target.message.empty(), "Maximum VA overflow accepted");
  }
  field(3, base - 1);
  Check(!ReadTlsDetails(context).terminated, "Callback array VA underflow accepted");
  field(2, 0);
  Check(!ReadTlsDetails(context).index.message.empty(), "Zero TLS index address not reported");

  reset();
  // A complete nonzero final slot with no terminator, followed by a truncated slot.
  field(3, base + 0x2000 - width);
  tls_fixture::WriteValue(image.data() + image.size() - width, base + 0x1800, wide);
  result = ReadTlsDetails(context);
  Check(result.callbacks.size() == 1 && !result.terminated && !result.message.empty(), "Missing TLS terminator not reported");
  field(3, base + 0x2000 - width + 1);
  Check(ReadTlsDetails(context).callbacks.empty(), "Partial TLS callback slot read past EOF");
  field(3, base + 0x2000);
  Check(!ReadTlsDetails(context).terminated, "Callback array at exclusive image end accepted");

  reset();
  // The same small RVA can still describe a range that wraps the VA width.
  ULONGLONG nearLimit = wide ? 0xFFFFFFFFFFFFE000ULL : 0xFFFFE000ULL;
  if (wide) ((PIMAGE_NT_HEADERS64)(image.data() + 0x80))->OptionalHeader.ImageBase = nearLimit;
  else ((PIMAGE_NT_HEADERS32)(image.data() + 0x80))->OptionalHeader.ImageBase = (DWORD)nearLimit;
  field(3, wide ? ~0ULL - width + 1 : (ULONGLONG)MAXDWORD - width + 1);
  tls_fixture::WriteValue(image.data() + image.size() - width, nearLimit + 0x1800, wide);
  field(2, wide ? ~0ULL - 1 : (ULONGLONG)MAXDWORD - 1);
  result = ReadTlsDetails(context);
  Check(result.index.message.find(L"overflows address width") != std::wstring::npos,
        "TLS index VA byte range wraps its address width");
  Check(result.callbacks.size() == 1 && !result.terminated &&
        result.message.find(L"slot VA range overflows") != std::wstring::npos,
        "TLS callback slot VA arithmetic wrapped");

  reset();
  directory->Size = (DWORD)headerSize - 1;
  Check(!ReadTlsDetails(context).hasHeader, "Short TLS directory header accepted");
  directory->Size = (DWORD)headerSize;
  directory->VirtualAddress = 0x2000 - (DWORD)headerSize + 1;
  Check(!ReadTlsDetails(context).hasHeader, "Truncated TLS disk header read");
  directory->VirtualAddress = 0x3000;
  Check(!ReadTlsDetails(context).hasHeader, "Unmapped TLS directory read");
  directory->VirtualAddress = MAXDWORD;
  Check(!ReadTlsDetails(context).hasHeader, "TLS directory RVA range overflow accepted");
  *directory = {};
  result = ReadTlsDetails(context);
  Check(!result.present && !result.hasHeader && result.callbacks.empty(), "Missing TLS directory became present");
}
}  // namespace

void CheckTlsDetails() {
  PEContext empty;
  Check(!ReadTlsDetails(empty).present, "Empty context has TLS data");
  CheckFormat(false);
  CheckFormat(true);

  // Alternating calls must keep all intermediate state local to each parse.
  std::vector<BYTE> pe32Image(pe32_fixture::kFileSize), pe64Image(pe32_fixture::kFileSize);
  tls_fixture::Fill(pe32Image.data(), false);
  tls_fixture::Fill(pe64Image.data(), true);
  PEContext pe32, pe64;
  Check(pe32.Initialize(pe32Image.data(), pe32Image.size()) &&
        pe64.Initialize(pe64Image.data(), pe64Image.size()), "TLS isolation fixtures rejected");
  const auto pe32Result = ReadTlsDetails(pe32);
  const auto pe64Result = ReadTlsDetails(pe64);
  Check(pe32Result.imageBase == pe32_fixture::kImageBase && pe32Result.callbacks.size() == 2 &&
        pe32Result.callbacks[1].target.va == pe32_fixture::kImageBase + 0x1810 &&
        pe64Result.imageBase == pe64_fixture::kImageBase && pe64Result.callbacks.size() == 2 &&
        pe64Result.callbacks[1].target.va == pe64_fixture::kImageBase + 0x1810,
        "TLS parses mixed context state or modified an earlier result");
  auto pe64Directory = const_cast<IMAGE_DATA_DIRECTORY*>(pe64.Directory(IMAGE_DIRECTORY_ENTRY_TLS));
  pe64Directory->Size = 1;
  const auto invalid = ReadTlsDetails(pe64);
  const auto repeated = ReadTlsDetails(pe32);
  Check(!invalid.hasHeader && invalid.callbacks.empty() &&
        repeated.hasHeader && repeated.terminated && repeated.message.empty() &&
        repeated.callbacks.size() == pe32Result.callbacks.size() &&
        repeated.callbacks[1].target.va == pe32Result.callbacks[1].target.va,
        "TLS failed parse leaked state into the next context");

  // Bound model size even when the file contains a huge valid-looking array.
  constexpr SIZE_T count = 100001;
  std::vector<BYTE> image(0x1000 + (count + 1) * 8);
  tls_fixture::Fill(image.data(), true);
  auto nt = (PIMAGE_NT_HEADERS64)(image.data() + 0x80);
  auto section = IMAGE_FIRST_SECTION(nt);
  section->SizeOfRawData = (DWORD)image.size() - pe32_fixture::kSectionRaw;
  section->Misc.VirtualSize = section->SizeOfRawData;
  nt->OptionalHeader.SizeOfImage = pe32_fixture::kSectionRva + section->SizeOfRawData;
  auto header = (PIMAGE_TLS_DIRECTORY64)(image.data() + tls_fixture::kRaw);
  header->AddressOfCallBacks = pe64_fixture::kImageBase + 0x1E00;
  for (SIZE_T i = 0; i < count; ++i)
    tls_fixture::WriteValue(image.data() + 0x1000 + i * 8, pe64_fixture::kImageBase + 0x1800, true);
  PEContext context;
  Check(context.Initialize(image.data(), image.size()), "Large TLS fixture rejected");
  auto result = ReadTlsDetails(context);
  Check(result.callbacks.size() == 100000 && !result.terminated &&
        result.message.find(L"100000") != std::wstring::npos, "TLS callback limit bypassed");
}
