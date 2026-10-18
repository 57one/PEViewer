#pragma once

#include "PEContext.h"

#include <string>
#include <vector>

// Microsoft PE format: import address table and base relocation block/types.
// https://learn.microsoft.com/en-us/windows/win32/debug/pe-format
// Disk-file values only; these parsers do not resolve addresses in a process.
enum class ImportKind { Unavailable, Terminator, Name, Ordinal, Invalid };

struct ImportSymbol {
  ImportKind kind = ImportKind::Unavailable;
  std::string name;
  WORD hint = 0;
  WORD ordinal = 0;
  std::wstring message;
};

struct ImportModule {
  DWORD descriptorIndex = 0;
  IMAGE_IMPORT_DESCRIPTOR descriptor = {};
  std::string name;
  std::wstring message;
};

struct ImportDetails {
  bool present = false;
  bool terminated = false;
  std::vector<ImportModule> modules;
  std::wstring message;
};

struct ImportThunk {
  DWORD rva = 0;
  SIZE_T fileOffset = 0;
  ULONGLONG value = 0;
  ImportSymbol symbol;
};

struct ImportThunkDetails {
  bool fallback = false;
  bool terminated = false;
  std::vector<ImportThunk> entries;
  std::wstring message;
};

struct IatEntry {
  DWORD rva = 0;
  SIZE_T fileOffset = 0;
  ULONGLONG value = 0;
  std::string dll;
  ImportSymbol symbol;
  bool associated = false;
  bool hasLookup = false;
  DWORD lookupRva = 0;
  ULONGLONG lookupValue = 0;
  std::wstring note;
};

struct IatDetails {
  std::vector<IatEntry> entries;
  std::wstring message;
};

struct ExportEntry {
  DWORD ordinal = 0;
  DWORD rva = 0;
  bool hasFileOffset = false;
  SIZE_T fileOffset = 0;
  std::string name;
};

struct ExportDetails {
  bool present = false;
  bool hasHeader = false;
  IMAGE_EXPORT_DIRECTORY header = {};
  std::vector<ExportEntry> entries;
  std::wstring message;
};

struct BoundForwarder {
  IMAGE_BOUND_FORWARDER_REF descriptor = {};
  std::string name;
  std::wstring message;
};

struct BoundImportEntry {
  IMAGE_BOUND_IMPORT_DESCRIPTOR descriptor = {};
  std::string name;
  std::vector<BoundForwarder> forwarders;
  std::wstring message;
};

struct BoundImportDetails {
  bool present = false;
  bool terminated = false;
  std::vector<BoundImportEntry> entries;
  std::wstring message;
};

struct DebugDetails {
  bool present = false;
  std::vector<IMAGE_DEBUG_DIRECTORY> entries;
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
ImportDetails ReadImportDetails(const PEContext& context);
ImportThunkDetails ReadImportThunks(const PEContext& context,
                                   const IMAGE_IMPORT_DESCRIPTOR& descriptor);
ExportDetails ReadExportDetails(const PEContext& context);
BoundImportDetails ReadBoundImportDetails(const PEContext& context);
DebugDetails ReadDebugDetails(const PEContext& context);
RelocationDetails ReadRelocationDetails(const PEContext& context);
