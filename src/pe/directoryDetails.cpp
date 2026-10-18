#include "directoryDetails.h"

#include <cstring>
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

void DecodeImport(IatEntry& entry, const PEContext& context, bool wide) {
  ULONGLONG flag = wide ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32;
  ULONGLONG value = entry.lookupValue;
  if (!value) {
    entry.note = entry.value ? L"INT terminator but IAT is nonzero" : L"Terminator";
  } else if (value & flag) {
    if (value & ~(flag | 0xFFFFULL)) {
      Note(entry.note, L"Invalid ordinal reserved bits");
    } else {
      entry.symbol = "Ordinal #" + std::to_string((WORD)value);
    }
  } else if (value > 0x7FFFFFFFULL) {
    Note(entry.note, L"Invalid name RVA / bound address");
  } else {
    const BYTE* hint = context.ReadRva((DWORD)value, sizeof(WORD));
    const char* name = hint ? context.ReadString((DWORD)value + sizeof(WORD)) : NULL;
    if (name) entry.symbol = name;
    else Note(entry.note, L"Invalid or unterminated import name");
  }
  if (!entry.value && value) Note(entry.note, L"IAT terminator before INT");
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

  const auto imports = context.Directory(IMAGE_DIRECTORY_ENTRY_IMPORT);
  if (!imports) return result;
  const BYTE* data = context.ReadRva(imports->VirtualAddress, imports->Size);
  if (!data) {
    Note(result.message, L"Import descriptors are invalid; DLL/name association unavailable");
    return result;
  }
  bool terminated = false;
  SIZE_T descriptors = imports->Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);
  SIZE_T work = 0;
  for (SIZE_T i = 0; i < descriptors; ++i) {
    if (++work > kMaxEntries) {
      Note(result.message, L"Import association limit reached");
      break;
    }
    IMAGE_IMPORT_DESCRIPTOR descriptor = {};
    std::memcpy(&descriptor, data + i * sizeof(descriptor), sizeof(descriptor));
    if (!descriptor.OriginalFirstThunk && !descriptor.FirstThunk && !descriptor.Name &&
        !descriptor.TimeDateStamp && !descriptor.ForwarderChain) {
      terminated = true;
      break;
    }
    if (descriptor.FirstThunk < directory->VirtualAddress ||
        (descriptor.FirstThunk - directory->VirtualAddress) % width ||
        (descriptor.FirstThunk - directory->VirtualAddress) / width >= count) {
      Note(result.message, L"An import FirstThunk lies outside the IAT slots");
      continue;
    }
    const char* dll = context.ReadString(descriptor.Name);
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
      if (!entry.dll.empty()) {
        Note(result.message, L"Overlapping import IAT ranges");
        break;
      }
      entry.dll = dll ? dll : "<invalid DLL name>";
      entry.note.clear();
      if (boundWithoutInt) {
        entry.note = entry.value ? L"Bound IAT; INT missing, name unavailable" : L"Terminator";
      } else {
        ULONGLONG lookup = (ULONGLONG)(fallback ? descriptor.FirstThunk :
                                      descriptor.OriginalFirstThunk) + (slot - first) * width;
        const BYTE* bytes = lookup <= MAXDWORD ? context.ReadRva((DWORD)lookup, width) : NULL;
        if (bytes) {
          entry.hasLookup = true;
          entry.lookupRva = (DWORD)lookup;
          entry.lookupValue = wide ? ReadValue<8>(bytes) : ReadValue<4>(bytes);
          DecodeImport(entry, context, wide);
          if (fallback) Note(entry.note, L"FirstThunk fallback (INT missing)");
        } else {
          entry.note = L"INT slot is outside the file-backed RVA range";
        }
      }
      if (!entry.value) {
        foundNull = true;
        break;
      }
    }
    if (!foundNull) Note(result.message, L"An imported DLL has no IAT terminator within the directory");
  }
  if (!terminated) Note(result.message, L"Import descriptor terminator not found");
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
