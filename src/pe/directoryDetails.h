#pragma once

#include "PEContext.h"

#include <string>
#include <vector>

// Structured disk-file directory results: imports/exports, resources, TLS, etc.
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

struct DelayImportAddress {
  DWORD raw = 0;
  DWORD rva = 0;
  SIZE_T fileOffset = 0;
  bool hasRva = false;
  bool hasFileOffset = false;
  std::wstring message;
};

struct DelayImportModule {
  DWORD descriptorIndex = 0;
  DWORD descriptorRva = 0;
  IMAGE_DELAYLOAD_DESCRIPTOR descriptor = {};
  bool rvaBased = false;
  bool supported = false;
  std::string name;
  DelayImportAddress dllName;
  DelayImportAddress moduleHandle;
  DelayImportAddress iat;
  DelayImportAddress lookup;
  DelayImportAddress boundIat;
  DelayImportAddress unloadIat;
  std::wstring message;
};

struct DelayImportDetails {
  bool present = false;
  bool terminated = false;
  DWORD rva = 0;
  DWORD size = 0;
  std::vector<DelayImportModule> modules;
  std::wstring message;
};

struct DelayImportSlot {
  bool hasRva = false;
  bool hasValue = false;
  DWORD rva = 0;
  SIZE_T fileOffset = 0;
  ULONGLONG value = 0;
  std::wstring message;
};

struct DelayImportThunk {
  DelayImportSlot lookup;
  ImportSymbol symbol;
  DelayImportSlot iat;
  DelayImportSlot boundIat;
  DelayImportSlot unloadIat;
  std::wstring message;
};

struct DelayImportThunkDetails {
  bool terminated = false;
  std::vector<DelayImportThunk> entries;
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
  bool isForwarder = false;
  bool hasFileOffset = false;
  SIZE_T fileOffset = 0;
  std::string name;
  std::string forwarder;
  std::wstring message;
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

struct ExceptionEntry {
  DWORD recordRva = 0;
  SIZE_T recordFileOffset = 0;
  DWORD beginRva = 0;
  DWORD endRva = 0;  // Exclusive end of the function/fragment range.
  DWORD unwindRva = 0;
  bool rangeValid = false;
  bool hasCodeFileOffset = false;
  SIZE_T codeFileOffset = 0;
  bool hasUnwindFileOffset = false;
  SIZE_T unwindFileOffset = 0;
  std::wstring message;
};

struct ExceptionDetails {
  bool present = false;
  bool supported = false;
  bool complete = false;  // All declared records read; entries can still have errors.
  WORD machine = 0;
  DWORD rva = 0;
  DWORD size = 0;
  SIZE_T totalEntries = 0;
  std::vector<ExceptionEntry> entries;
  std::wstring message;
};

struct LoadConfigField {
  std::wstring name;
  DWORD offset = 0;  // Offset within IMAGE_LOAD_CONFIG_DIRECTORY32/64.
  BYTE size = 0;
  bool available = false;
  ULONGLONG value = 0;
  bool isVa = false;
  bool hasRva = false;
  DWORD rva = 0;
  bool hasFileOffset = false;
  SIZE_T fileOffset = 0;
  std::wstring message;
};

struct SafeSehEntry {
  DWORD slotRva = 0;
  SIZE_T slotFileOffset = 0;
  DWORD handlerRva = 0;
  bool hasVa = false;
  ULONGLONG handlerVa = 0;
  bool hasFileOffset = false;
  SIZE_T fileOffset = 0;
  std::wstring message;
};

struct SafeSehDetails {
  bool applicable = false;  // Only I386 / PE32 has an x86 SafeSEH table.
  bool hasFields = false;
  bool complete = false;  // All declared slots read, independently of row errors.
  ULONGLONG tableVa = 0;
  ULONGLONG count = 0;
  std::vector<SafeSehEntry> entries;
  std::wstring message;
};

struct LoadConfigDetails {
  bool present = false;
  bool hasSize = false;
  DWORD rva = 0;
  DWORD size = 0;  // DataDirectory size, distinct from the structure's Size.
  DWORD declaredSize = 0;
  DWORD knownSize = 0;  // Layout supported by the build's Windows SDK.
  std::vector<LoadConfigField> fields;
  SafeSehDetails safeSeh;
  std::wstring message;
};

struct TlsAddress {
  ULONGLONG va = 0;
  DWORD rva = 0;
  SIZE_T fileOffset = 0;
  bool hasRva = false;
  bool hasFileOffset = false;
  std::wstring message;
};

struct TlsCallback {
  DWORD slotRva = 0;
  SIZE_T slotFileOffset = 0;
  TlsAddress target;
};

struct TlsDetails {
  bool present = false;
  bool hasHeader = false;
  bool terminated = false;
  DWORD rva = 0;
  DWORD size = 0;
  ULONGLONG imageBase = 0;
  TlsAddress rawDataStart;
  TlsAddress rawDataEnd;  // Exclusive end of the initialization template.
  TlsAddress index;
  TlsAddress callbackArray;
  ULONGLONG rawDataSize = 0;
  bool rawDataValid = false;
  DWORD sizeOfZeroFill = 0;
  DWORD characteristics = 0;
  std::vector<TlsCallback> callbacks;
  std::wstring message;
};

struct ResourceNode {
  SIZE_T parent = (SIZE_T)-1;
  DWORD depth = 0;
  DWORD entryOffset = 0;
  bool named = false;
  bool hasName = false;
  DWORD id = 0;
  std::wstring name;
  bool isDirectory = false;
  DWORD targetOffset = 0;
  bool hasDirectory = false;
  IMAGE_RESOURCE_DIRECTORY directory = {};
  bool hasData = false;
  IMAGE_RESOURCE_DATA_ENTRY data = {};
  bool hasFileOffset = false;
  SIZE_T fileOffset = 0;
  bool dataValid = false;
  std::wstring message;
};

struct ResourceDetails {
  bool present = false;
  bool hasRoot = false;
  DWORD rva = 0;
  DWORD size = 0;
  IMAGE_RESOURCE_DIRECTORY root = {};
  // Preorder nodes; parent is an earlier index, or SIZE_T(-1) for root children.
  std::vector<ResourceNode> nodes;
  SIZE_T resourceCount = 0;
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
DelayImportDetails ReadDelayImportDetails(const PEContext& context);
DelayImportThunkDetails ReadDelayImportThunks(const PEContext& context,
                                            const DelayImportModule& module);
ExportDetails ReadExportDetails(const PEContext& context);
BoundImportDetails ReadBoundImportDetails(const PEContext& context);
DebugDetails ReadDebugDetails(const PEContext& context);
ExceptionDetails ReadExceptionDetails(const PEContext& context);
LoadConfigDetails ReadLoadConfigDetails(const PEContext& context);
TlsDetails ReadTlsDetails(const PEContext& context);
ResourceDetails ReadResourceDetails(const PEContext& context);
RelocationDetails ReadRelocationDetails(const PEContext& context);
