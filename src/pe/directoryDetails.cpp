#include "directoryDetails.h"

#include <cstring>
#include <map>
#include <utility>

namespace {
// Bound the UI/model size even for very large or deliberately hostile files.
constexpr SIZE_T kMaxEntries = 100000;

template <int Width>
ULONGLONG ReadValue(const BYTE* data) {
  static_assert(Width == 2 || Width == 4 || Width == 8,
                "ReadValue only supports 2, 4 or 8 bytes");
  ULONGLONG value = 0;
  std::memcpy(&value, data, Width);
  return value;
}

void Note(std::wstring& text, const wchar_t* message) {
  if (text.find(message) != std::wstring::npos) return;
  if (!text.empty()) text += L"; ";
  text += message;
}

ImportSymbol DecodeImport(ULONGLONG value, const PEContext& context) {
  ImportSymbol symbol;
  symbol.kind = ImportKind::Invalid;
  bool wide = context.format == PEFormat::PE32Plus;
  ULONGLONG flag = wide ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32;
  if (!value) {
    symbol.kind = ImportKind::Terminator;
  } else if (value & flag) {
    if (value & ~(flag | 0xFFFFULL)) {
      symbol.message = L"Invalid ordinal reserved bits";
    } else {
      symbol.kind = ImportKind::Ordinal;
      symbol.ordinal = (WORD)value;
    }
  } else if (value > 0x7FFFFFFFULL) {
    symbol.message = L"Invalid name RVA / bound address";
  } else {
    const BYTE* hint = context.ReadRva((DWORD)value, sizeof(WORD));
    const char* name = hint ? context.ReadString((DWORD)value + sizeof(WORD)) : NULL;
    if (name) {
      symbol.kind = ImportKind::Name;
      symbol.hint = (WORD)ReadValue<2>(hint);
      symbol.name = name;
    } else {
      symbol.message = L"Invalid or unterminated import name";
    }
  }
  return symbol;
}

bool ReadLookupSlot(const PEContext& context, ULONGLONG rva, ImportThunk& entry) {
  bool wide = context.format == PEFormat::PE32Plus;
  SIZE_T width = wide ? 8 : 4;
  if (rva > MAXDWORD || !context.RvaToFileOffset((DWORD)rva, width, entry.fileOffset)) return false;
  entry.rva = (DWORD)rva;
  const BYTE* data = context.fileBuffer + entry.fileOffset;
  entry.value = wide ? ReadValue<8>(data) : ReadValue<4>(data);
  entry.symbol = DecodeImport(entry.value, context);
  return true;
}

const wchar_t* RelocationName(WORD type) {
  switch (type) {
    case IMAGE_REL_BASED_ABSOLUTE: return L"ABSOLUTE (0)";
    case IMAGE_REL_BASED_HIGH: return L"HIGH (1)";
    case IMAGE_REL_BASED_LOW: return L"LOW (2)";
    case IMAGE_REL_BASED_HIGHLOW: return L"HIGHLOW (3)";
    case IMAGE_REL_BASED_HIGHADJ: return L"HIGHADJ (4)";
    case IMAGE_REL_BASED_DIR64: return L"DIR64 (10)";
    default: return NULL;
  }
}
}  // namespace

ImportDetails ReadImportDetails(const PEContext& context) {
  ImportDetails result;
  const auto directory = context.Directory(IMAGE_DIRECTORY_ENTRY_IMPORT);
  if (!directory) {
    result.message = L"No import directory.";
    return result;
  }
  result.present = true;
  const BYTE* data = context.ReadRva(directory->VirtualAddress, directory->Size);
  if (!data) {
    result.message = L"Import descriptors are outside the file-backed RVA range.";
    return result;
  }
  SIZE_T count = directory->Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);
  for (SIZE_T i = 0; i < count; ++i) {
    if (i >= kMaxEntries) {
      Note(result.message, L"Import descriptor display limited to 100000 entries");
      break;
    }
    ImportModule module;
    module.descriptorIndex = (DWORD)i;
    auto& descriptor = module.descriptor;
    std::memcpy(&descriptor, data + i * sizeof(descriptor), sizeof(descriptor));
    if (!descriptor.OriginalFirstThunk && !descriptor.FirstThunk && !descriptor.Name &&
        !descriptor.TimeDateStamp && !descriptor.ForwarderChain) {
      result.terminated = true;
      break;
    }
    const char* name = descriptor.Name ? context.ReadString(descriptor.Name) : NULL;
    if (name) module.name = name;
    else {
      module.message = L"Invalid or unterminated DLL name";
      Note(result.message, module.message.c_str());
    }
    result.modules.push_back(std::move(module));
  }
  if (!result.terminated) Note(result.message, L"Import descriptor terminator not found");
  return result;
}

ImportThunkDetails ReadImportThunks(const PEContext& context,
                                   const IMAGE_IMPORT_DESCRIPTOR& descriptor) {
  ImportThunkDetails result;
  if (!context.IsValid()) {
    result.message = L"Invalid PE context.";
    return result;
  }
  if (!descriptor.OriginalFirstThunk && descriptor.TimeDateStamp) {
    result.message = L"Bound IAT; INT missing, name unavailable";
    return result;
  }
  result.fallback = !descriptor.OriginalFirstThunk;
  DWORD start = result.fallback ? descriptor.FirstThunk : descriptor.OriginalFirstThunk;
  if (!start) {
    result.message = L"No import lookup table.";
    return result;
  }
  SIZE_T width = context.format == PEFormat::PE32Plus ? 8 : 4;
  for (SIZE_T i = 0; i < kMaxEntries; ++i) {
    ImportThunk entry;
    if (!ReadLookupSlot(context, (ULONGLONG)start + i * width, entry)) {
      result.message = L"Import lookup slot is outside the file-backed RVA range; terminator not found.";
      return result;
    }
    if (entry.symbol.kind == ImportKind::Terminator) {
      result.terminated = true;
      return result;
    }
    if (entry.symbol.kind == ImportKind::Invalid) {
      result.message = entry.symbol.message;
      return result;
    }
    result.entries.push_back(std::move(entry));
  }
  result.message = L"Import function display limited to 100000 entries; terminator not found.";
  return result;
}

IatDetails ReadIatDetails(const PEContext& context) {
  IatDetails result;
  const auto directory = context.Directory(IMAGE_DIRECTORY_ENTRY_IAT);
  if (!directory) {
    result.message = L"No IAT directory.";
    return result;
  }
  SIZE_T baseOffset = 0;
  if (!context.RvaToFileOffset(directory->VirtualAddress, directory->Size, baseOffset)) {
    result.message = L"IAT directory is outside the file-backed RVA range.";
    return result;
  }
  bool wide = context.format == PEFormat::PE32Plus;
  SIZE_T width = wide ? 8 : 4;
  SIZE_T count = directory->Size / width;
  if (directory->Size % width) Note(result.message, L"IAT ends with a truncated slot");
  if (count > kMaxEntries) {
    count = kMaxEntries;
    Note(result.message, L"IAT display limited to 100000 slots");
  }
  for (SIZE_T i = 0; i < count; ++i) {
    IatEntry entry;
    entry.rva = directory->VirtualAddress + (DWORD)(i * width);
    entry.fileOffset = baseOffset + i * width;
    const BYTE* bytes = context.fileBuffer + entry.fileOffset;
    entry.value = wide ? ReadValue<8>(bytes) : ReadValue<4>(bytes);
    entry.note = entry.value ? L"Unassociated slot" : L"Null / padding";
    result.entries.push_back(std::move(entry));
  }

  const ImportDetails imports = ReadImportDetails(context);
  if (!imports.present) return result;
  if (!imports.message.empty()) Note(result.message, imports.message.c_str());
  SIZE_T work = 0;
  for (const auto& module : imports.modules) {
    if (++work > kMaxEntries) {
      Note(result.message, L"Import association limit reached");
      break;
    }
    const auto& descriptor = module.descriptor;
    if (descriptor.FirstThunk < directory->VirtualAddress ||
        (descriptor.FirstThunk - directory->VirtualAddress) % width ||
        (descriptor.FirstThunk - directory->VirtualAddress) / width >= count) {
      Note(result.message, L"An import FirstThunk lies outside the IAT slots");
      continue;
    }
    bool fallback = !descriptor.OriginalFirstThunk && !descriptor.TimeDateStamp;
    bool boundWithoutInt = !descriptor.OriginalFirstThunk && descriptor.TimeDateStamp;
    SIZE_T first = (descriptor.FirstThunk - directory->VirtualAddress) / width;
    bool foundNull = false;
    for (SIZE_T slot = first; slot < count; ++slot) {
      if (++work > kMaxEntries) {
        Note(result.message, L"Import association limit reached");
        break;
      }
      IatEntry& entry = result.entries[slot];
      if (entry.associated) {
        Note(result.message, L"Overlapping import IAT ranges");
        break;
      }
      entry.associated = true;
      entry.dll = module.name;
      entry.note.clear();
      if (boundWithoutInt) {
        entry.note = entry.value ? L"Bound IAT; INT missing, name unavailable" : L"Terminator";
      } else {
        ULONGLONG lookup = (ULONGLONG)(fallback ? descriptor.FirstThunk :
                                      descriptor.OriginalFirstThunk) + (slot - first) * width;
        ImportThunk thunk;
        if (ReadLookupSlot(context, lookup, thunk)) {
          entry.hasLookup = true;
          entry.lookupRva = thunk.rva;
          entry.lookupValue = thunk.value;
          entry.symbol = std::move(thunk.symbol);
          entry.note = entry.symbol.message;
          if (entry.symbol.kind == ImportKind::Terminator)
            entry.note = entry.value ? L"INT terminator but IAT is nonzero" : L"Terminator";
          if (!entry.value && thunk.value) Note(entry.note, L"IAT terminator before INT");
          if (fallback) Note(entry.note, L"FirstThunk fallback (INT missing)");
        } else {
          entry.note = L"INT slot is outside the file-backed RVA range";
        }
      }
      if (!module.message.empty()) Note(entry.note, module.message.c_str());
      if (!entry.value) {
        foundNull = true;
        break;
      }
    }
    if (!foundNull) Note(result.message, L"An imported DLL has no IAT terminator within the directory");
  }
  return result;
}

ExportDetails ReadExportDetails(const PEContext& context) {
  ExportDetails result;
  const auto directory = context.Directory(IMAGE_DIRECTORY_ENTRY_EXPORT);
  if (!directory) {
    result.message = L"No export directory.";
    return result;
  }
  result.present = true;
  const BYTE* data = directory->Size >= sizeof(result.header) ?
      context.ReadRva(directory->VirtualAddress, sizeof(result.header)) : NULL;
  if (!data) {
    result.message = L"Invalid or truncated export directory header.";
    return result;
  }
  std::memcpy(&result.header, data, sizeof(result.header));
  result.hasHeader = true;
  if ((ULONGLONG)directory->VirtualAddress + directory->Size > (ULONGLONG)MAXDWORD + 1) {
    result.message = L"Export directory RVA range overflows 32 bits.";
    return result;
  }
  const auto& header = result.header;
  if (header.NumberOfFunctions > kMaxEntries || header.NumberOfNames > kMaxEntries ||
      header.NumberOfFunctions > context.fileSize / sizeof(DWORD) ||
      header.NumberOfNames > context.fileSize / sizeof(DWORD)) {
    result.message = L"Export arrays exceed the file size or the 100000-entry limit.";
    return result;
  }
  if (!header.NumberOfFunctions) {
    if (header.NumberOfNames) result.message = L"Export names exist without a function array.";
    return result;
  }
  const BYTE* functions = context.ReadRva(header.AddressOfFunctions,
      (SIZE_T)header.NumberOfFunctions * sizeof(DWORD));
  if (!functions) {
    result.message = L"Export function array is outside the file-backed RVA range.";
    return result;
  }
  std::map<DWORD, DWORD> namesByFunction;
  if (header.NumberOfNames) {
    const BYTE* names = context.ReadRva(header.AddressOfNames,
        (SIZE_T)header.NumberOfNames * sizeof(DWORD));
    const BYTE* ordinals = context.ReadRva(header.AddressOfNameOrdinals,
        (SIZE_T)header.NumberOfNames * sizeof(WORD));
    if (!names || !ordinals) {
      result.message = L"Export name/ordinal arrays are outside the file-backed RVA range.";
      return result;
    }
    for (DWORD i = 0; i < header.NumberOfNames; ++i) {
      WORD index = (WORD)ReadValue<2>(ordinals + i * sizeof(WORD));
      if (index >= header.NumberOfFunctions) {
        result.message = L"Export name ordinal index exceeds the function array.";
        return result;
      }
      namesByFunction.insert({index, (DWORD)ReadValue<4>(names + i * sizeof(DWORD))});
    }
  }
  for (DWORD i = 0; i < header.NumberOfFunctions; ++i) {
    DWORD rva = (DWORD)ReadValue<4>(functions + i * sizeof(DWORD));
    if (!rva) continue;
    if (i > MAXDWORD - header.Base) {
      result.message = L"Export ordinal overflow.";
      break;
    }
    ExportEntry entry;
    entry.rva = rva;
    entry.ordinal = header.Base + i;
    auto name = namesByFunction.find(i);
    if (name != namesByFunction.end()) {
      const char* text = context.ReadString(name->second);
      if (!text) {
        result.message = L"Invalid or unterminated export name.";
        break;
      }
      entry.name = text;
    }
    entry.hasFileOffset = context.RvaToFileOffset(rva, 1, entry.fileOffset) != FALSE;
    // EAT entries inside the export data-directory range point to forwarder strings.
    // Subtraction avoids wrapping the upper bound in 32-bit RVA arithmetic.
    entry.isForwarder = rva >= directory->VirtualAddress &&
                        rva - directory->VirtualAddress < directory->Size;
    if (entry.isForwarder) {
      SIZE_T remaining = directory->Size - (rva - directory->VirtualAddress);
      const char* target = context.ReadString(rva, remaining);
      if (target && *target) {
        entry.forwarder = target;
      } else {
        entry.message = L"Invalid or unterminated export forwarder within directory/file range";
        Note(result.message, entry.message.c_str());
      }
    }
    result.entries.push_back(std::move(entry));
  }
  return result;
}

BoundImportDetails ReadBoundImportDetails(const PEContext& context) {
  BoundImportDetails result;
  const auto directory = context.Directory(IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT);
  if (!directory) {
    result.message = L"No bound import directory.";
    return result;
  }
  result.present = true;
  const BYTE* data = context.ReadRva(directory->VirtualAddress, directory->Size);
  if (!data) {
    result.message = L"Bound import directory is outside the file-backed RVA range.";
    return result;
  }
  auto readName = [&](WORD offset, std::string& name) {
    if (!offset || offset >= directory->Size ||
        !std::memchr(data + offset, 0, directory->Size - offset)) return false;
    name = (const char*)(data + offset);
    return true;
  };
  SIZE_T pos = 0, work = 0;
  while (directory->Size - pos >= sizeof(IMAGE_BOUND_IMPORT_DESCRIPTOR)) {
    if (++work > kMaxEntries) {
      Note(result.message, L"Bound import display limited to 100000 records");
      break;
    }
    BoundImportEntry entry;
    auto& descriptor = entry.descriptor;
    std::memcpy(&descriptor, data + pos, sizeof(descriptor));
    if (!descriptor.TimeDateStamp && !descriptor.OffsetModuleName && !descriptor.NumberOfModuleForwarderRefs) {
      result.terminated = true;
      break;
    }
    if (!readName(descriptor.OffsetModuleName, entry.name)) {
      entry.message = L"Invalid or unterminated bound import name";
      Note(result.message, entry.message.c_str());
    }
    SIZE_T step = sizeof(descriptor) + (SIZE_T)descriptor.NumberOfModuleForwarderRefs *
                                      sizeof(IMAGE_BOUND_FORWARDER_REF);
    if (step > directory->Size - pos) {
      Note(result.message, L"Truncated bound import forwarder records");
      result.entries.push_back(std::move(entry));
      break;
    }
    for (WORD i = 0; i < descriptor.NumberOfModuleForwarderRefs; ++i) {
      if (++work > kMaxEntries) {
        Note(result.message, L"Bound import display limited to 100000 records");
        break;
      }
      BoundForwarder forwarder;
      std::memcpy(&forwarder.descriptor, data + pos + sizeof(descriptor) +
                  (SIZE_T)i * sizeof(forwarder.descriptor), sizeof(forwarder.descriptor));
      if (!readName(forwarder.descriptor.OffsetModuleName, forwarder.name)) {
        forwarder.message = L"Invalid or unterminated bound forwarder name";
        Note(result.message, forwarder.message.c_str());
      }
      entry.forwarders.push_back(std::move(forwarder));
    }
    result.entries.push_back(std::move(entry));
    if (work > kMaxEntries) break;
    pos += step;
  }
  if (!result.terminated) Note(result.message, L"Bound import descriptor terminator not found");
  return result;
}

DebugDetails ReadDebugDetails(const PEContext& context) {
  DebugDetails result;
  const auto directory = context.Directory(IMAGE_DIRECTORY_ENTRY_DEBUG);
  if (!directory) {
    result.message = L"No debug directory.";
    return result;
  }
  result.present = true;
  const BYTE* data = context.ReadRva(directory->VirtualAddress, directory->Size);
  if (!data) {
    result.message = L"Debug directory is outside the file-backed RVA range.";
    return result;
  }
  SIZE_T count = directory->Size / sizeof(IMAGE_DEBUG_DIRECTORY);
  if (directory->Size % sizeof(IMAGE_DEBUG_DIRECTORY))
    Note(result.message, L"Truncated debug directory record");
  if (count > kMaxEntries) {
    count = kMaxEntries;
    Note(result.message, L"Debug display limited to 100000 records");
  }
  for (SIZE_T i = 0; i < count; ++i) {
    IMAGE_DEBUG_DIRECTORY entry = {};
    std::memcpy(&entry, data + i * sizeof(entry), sizeof(entry));
    result.entries.push_back(entry);
  }
  return result;
}

RelocationDetails ReadRelocationDetails(const PEContext& context) {
  RelocationDetails result;
  const auto directory = context.Directory(IMAGE_DIRECTORY_ENTRY_BASERELOC);
  if (!directory) {
    result.message = L"No base relocation directory.";
    return result;
  }
  SIZE_T baseOffset = 0;
  if (!context.RvaToFileOffset(directory->VirtualAddress, directory->Size, baseOffset)) {
    result.message = L"Relocation directory is outside the file-backed RVA range.";
    return result;
  }
  SIZE_T pos = 0, total = 0;
  while (pos < directory->Size) {
    if (result.blocks.size() >= kMaxEntries || total >= kMaxEntries) {
      result.message = L"Relocation display limited to 100000 blocks/entries.";
      break;
    }
    if (((ULONGLONG)directory->VirtualAddress + pos) % 4 ||
        directory->Size - pos < sizeof(IMAGE_BASE_RELOCATION)) {
      result.message = L"Truncated or unaligned relocation block header.";
      break;
    }
    IMAGE_BASE_RELOCATION header = {};
    const BYTE* bytes = context.fileBuffer + baseOffset + pos;
    std::memcpy(&header, bytes, sizeof(header));
    if (header.SizeOfBlock < sizeof(header) || header.SizeOfBlock > directory->Size - pos ||
        (header.SizeOfBlock - sizeof(header)) % sizeof(WORD)) {
      result.message = L"Invalid relocation SizeOfBlock (too small, truncated or odd).";
      break;
    }
    if (header.VirtualAddress % 0x1000) {
      result.message = L"Relocation Page RVA is not aligned to 4 KB.";
      break;
    }
    RelocationBlock block;
    block.rva = directory->VirtualAddress + (DWORD)pos;
    block.fileOffset = baseOffset + pos;
    block.pageRva = header.VirtualAddress;
    block.size = header.SizeOfBlock;
    SIZE_T slots = (header.SizeOfBlock - sizeof(header)) / sizeof(WORD);
    for (SIZE_T i = 0; i < slots; ++i) {
      if (total++ >= kMaxEntries) {
        result.message = L"Relocation display limited to 100000 blocks/entries.";
        break;
      }
      RelocationEntry entry;
      std::memcpy(&entry.raw, bytes + sizeof(header) + i * sizeof(WORD), sizeof(WORD));
      entry.type = entry.raw >> 12;
      entry.offset = entry.raw & 0xFFF;
      const wchar_t* name = RelocationName(entry.type);
      entry.typeName = name ? name : L"Type " + std::to_wstring(entry.type) + L" (machine-specific/unknown)";
      if (entry.type == IMAGE_REL_BASED_HIGHADJ) {
        // HIGHADJ consumes the next WORD as a signed adjustment, not another relocation.
        if (++i >= slots) {
          entry.note = L"Missing HIGHADJ adjustment WORD";
          result.message = L"Truncated HIGHADJ pair.";
          block.entries.push_back(std::move(entry));
          break;
        }
        entry.hasAdjustment = true;
        std::memcpy(&entry.adjustment, bytes + sizeof(header) + i * sizeof(WORD), sizeof(WORD));
      }
      if (entry.type == IMAGE_REL_BASED_ABSOLUTE) {
        entry.note = L"Padding (skipped)";
      } else if (header.VirtualAddress > MAXDWORD - entry.offset) {
        entry.note = L"Target RVA overflow";
      } else {
        entry.hasTarget = true;
        entry.targetRva = header.VirtualAddress + entry.offset;
        switch (entry.type) {
          case IMAGE_REL_BASED_HIGH:
          case IMAGE_REL_BASED_LOW:
          case IMAGE_REL_BASED_HIGHADJ: entry.valueSize = 2; break;
          case IMAGE_REL_BASED_HIGHLOW: entry.valueSize = 4; break;
          case IMAGE_REL_BASED_DIR64: entry.valueSize = 8; break;
        }
        entry.hasFileOffset = context.RvaToFileOffset(entry.targetRva,
            entry.valueSize ? entry.valueSize : 1, entry.fileOffset) != FALSE;
        if (entry.hasFileOffset && entry.valueSize) {
          const BYTE* target = context.fileBuffer + entry.fileOffset;
          switch (entry.valueSize) {
            case 2: entry.value = ReadValue<2>(target); entry.hasValue = true; break;
            case 4: entry.value = ReadValue<4>(target); entry.hasValue = true; break;
            case 8: entry.value = ReadValue<8>(target); entry.hasValue = true; break;
            default: entry.note = L"Value decoding unavailable for this type"; break;
          }
        } else {
          entry.note = entry.hasFileOffset ? L"Value decoding unavailable for this type" :
                                             L"Target has no complete file-backed bytes";
        }
      }
      block.entries.push_back(std::move(entry));
    }
    result.blocks.push_back(std::move(block));
    if (!result.message.empty()) break;
    pos += header.SizeOfBlock;
  }
  return result;
}
