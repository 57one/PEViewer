#pragma once

#include "PEContext.h"

#include <string>
#include <vector>

// Microsoft PE format: import address table and base relocation block/types.
// https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
// Disk-file values only; these parsers do not resolve addresses in a process.
struct IatEntry {
  DWORD rva = 0;
  SIZE_T fileOffset = 0;
  ULONGLONG value = 0;
  std::string dll;
  std::string symbol;
  bool hasLookup = false;
  DWORD lookupRva = 0;
  ULONGLONG lookupValue = 0;
  std::wstring note;
};

struct IatDetails {
  std::vector<IatEntry> entries;
  std::wstring message;
};

struct RelocationEntry {
  WORD raw = 0;
  WORD type = 0;
  WORD offset = 0;
  DWORD targetRva = 0;
  SIZE_T fileOffset = 0;
  ULONGLONG value = 0;
  BYTE valueSize = 0;
  bool hasTarget = false;
  bool hasFileOffset = false;
  bool hasValue = false;
  bool hasAdjustment = false;
  SHORT adjustment = 0;
  std::wstring typeName;
  std::wstring note;
};

struct RelocationBlock {
  DWORD rva = 0;
  SIZE_T fileOffset = 0;
  DWORD pageRva = 0;
  DWORD size = 0;
  std::vector<RelocationEntry> entries;
};

struct RelocationDetails {
  std::vector<RelocationBlock> blocks;
  std::wstring message;
};

IatDetails ReadIatDetails(const PEContext& context);
RelocationDetails ReadRelocationDetails(const PEContext& context);
