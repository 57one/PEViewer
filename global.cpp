#include "global.h"

TCHAR szUnknown[] = TEXT("<Unknown>");
TCHAR szNoDebugPriv[] = TEXT("<NoDebugPriv>");
TCHAR szAccessDenied[] = TEXT("<AccessDenied>");
TCHAR szX86[] = TEXT("x86");
TCHAR szX64[] = TEXT("x64");

COLORREF colorGrey = RGB(0xf1, 0xf2, 0xf6);
COLORREF colorWhite = RGB(0xff, 0xff, 0xff);

WORD wMachineTypeLength = 30;

TCHAR szMachineType[][40] = {TEXT("IMAGE_FILE_MACHINE_UNKNOWN"),
                             TEXT("IMAGE_FILE_MACHINE_ALPHA"),
                             TEXT("IMAGE_FILE_MACHINE_ALPHA64"),
                             TEXT("IMAGE_FILE_MACHINE_AM33"),
                             TEXT("IMAGE_FILE_MACHINE_AMD64"),
                             TEXT("IMAGE_FILE_MACHINE_ARM"),
                             TEXT("IMAGE_FILE_MACHINE_ARM64"),
                             TEXT("IMAGE_FILE_MACHINE_ARMNT"),
                             TEXT("IMAGE_FILE_MACHINE_AXP64"),
                             TEXT("IMAGE_FILE_MACHINE_EBC"),
                             TEXT("IMAGE_FILE_MACHINE_I386"),
                             TEXT("IMAGE_FILE_MACHINE_IA64"),
                             TEXT("IMAGE_FILE_MACHINE_LOONGARCH32"),
                             TEXT("IMAGE_FILE_MACHINE_LOONGARCH64"),
                             TEXT("IMAGE_FILE_MACHINE_M32R"),
                             TEXT("IMAGE_FILE_MACHINE_MIPS16"),
                             TEXT("IMAGE_FILE_MACHINE_MIPSFPU"),
                             TEXT("IMAGE_FILE_MACHINE_MIPSFPU16"),
                             TEXT("IMAGE_FILE_MACHINE_POWERPC"),
                             TEXT("IMAGE_FILE_MACHINE_POWERPCFP"),
                             TEXT("IMAGE_FILE_MACHINE_R4000"),
                             TEXT("IMAGE_FILE_MACHINE_RISCV32"),
                             TEXT("IMAGE_FILE_MACHINE_RISCV64"),
                             TEXT("IMAGE_FILE_MACHINE_RISCV128"),
                             TEXT("IMAGE_FILE_MACHINE_SH3"),
                             TEXT("IMAGE_FILE_MACHINE_SH3DSP"),
                             TEXT("IMAGE_FILE_MACHINE_SH4"),
                             TEXT("IMAGE_FILE_MACHINE_SH5"),
                             TEXT("IMAGE_FILE_MACHINE_THUMB"),
                             TEXT("IMAGE_FILE_MACHINE_WCEMIPSV2")};

WORD wMachineType[] = {
    0x0,   0x184,  0x284,  0x1d3,  0x8664, 0x1c0, 0xaa64, 0x1c4, 0x284, 0xebc,
    0x14c, 0x200,  0x6232, 0x6264, 0x9041, 0x266, 0x366,  0x466, 0x1f0, 0x1f1,
    0x166, 0x5032, 0x5064, 0x5128, 0x1a2,  0x1a3, 0x1a6,  0x1a8, 0x1c2, 0x169};

std::map<WORD, WORD> machinetypeToIndex{
    {0x0, 0},    {0x184, 1},   {0x284, 2},   {0x1d3, 3},   {0x8664, 4},
    {0x1c0, 5},  {0xaa64, 6},  {0x1c4, 7},   {0x284, 8},   {0xebc, 9},
    {0x14c, 10}, {0x200, 11},  {0x6232, 12}, {0x6264, 13}, {0x9041, 14},
    {0x266, 15}, {0x366, 16},  {0x466, 17},  {0x1f0, 18},  {0x1f1, 19},
    {0x166, 20}, {0x5032, 21}, {0x5064, 22}, {0x5128, 23}, {0x1a2, 24},
    {0x1a3, 25}, {0x1a6, 26},  {0x1a8, 27},  {0x1c2, 28},  {0x169, 29}};

TCHAR szMachineTypeDesc[][40] = {TEXT("UNKNOWN"),
                                 TEXT("Alpha AXP, 32-bit address space"),
                                 TEXT("Alpha 64, 64-bit address space"),
                                 TEXT("Matsushita AM33"),
                                 TEXT("x64"),
                                 TEXT("ARM little endian"),
                                 TEXT("ARM64 little endian"),
                                 TEXT("ARM Thumb-2 little endian"),
                                 TEXT("AXP 64 (Same as Alpha 64)"),
                                 TEXT("EFI byte code"),
                                 TEXT("Intel 386"),
                                 TEXT("Intel Itanium processor family"),
                                 TEXT("LoongArch 32-bit processor family"),
                                 TEXT("LoongArch 64-bit processor family"),
                                 TEXT("Mitsubishi M32R little endian"),
                                 TEXT("MIPS16"),
                                 TEXT("MIPS with FPU"),
                                 TEXT("MIPS16 with FPU"),
                                 TEXT("Power PC little endian"),
                                 TEXT("Power PC with floating point support"),
                                 TEXT("MIPS little endian"),
                                 TEXT("RISC-V 32-bit address space"),
                                 TEXT("RISC-V 64-bit address space"),
                                 TEXT("RISC-V 128-bit address space"),
                                 TEXT("Hitachi SH3"),
                                 TEXT("Hitachi SH3 DSP"),
                                 TEXT("Hitachi SH4"),
                                 TEXT("Hitachi SH5"),
                                 TEXT("Thumb"),
                                 TEXT("MIPS little-endian WCE v2")};

DWORD characteristicsBegin = 0x1;
DWORD characteristicsEnd = 0x8000;

INT characteristicsCheckBoxID[] = {
    IDC_CHECK_FILE_RELOCS_STRIPPED,
    IDC_CHECK_FILE_EXECUTABLE_IMAGE,
    IDC_CHECK_FILE_LINE_NUMS_STRIPPED,
    IDC_CHECK_FILE_LOCAL_SYMS_STRIPPED,
    IDC_CHECK_FILE_AGGRESSIVE_WS_TRIM,
    IDC_CHECK_FILE_LARGE_ADDRESS_AWARE,
    IDC_CHECK_CHARA_RESERVED,
    IDC_CHECK_FILE_BYTES_REVERSED_LO,
    IDC_CHECK_FILE_32BIT_MACHINE,
    IDC_CHECK_FILE_DEBUG_STRIPPED,
    IDC_CHECK_FILE_REMOVABLE_RUN_FROM_SWAP,
    IDC_CHECK_FILE_NET_RUN_FROM_SWAP,
    IDC_CHECK_FILE_SYSTEM,
    IDC_CHECK_FILE_DLL,
    IDC_CHECK_FILE_UP_SYSTEM_ONLY,
    IDC_CHECK_FILE_BYTES_REVERSED_HI,
};

TCHAR szCharacteristics[][80] = {
    TEXT("File has no Relocation info"),
    TEXT("File is executable"),
    TEXT("File has no COFF Line number (deprecated)(should be zero)"),
    TEXT("File has no COFF local symbolS (deprecated)(should be zero)"),
    TEXT("Aggressively trim working set (deprecated for Win2000 and later)"),
    TEXT("Application can handle > 2-GB addresses"),
    TEXT("No Flag (0x0040) reserved for future use"),
    TEXT("Little endian: LSB precedes MSB in memory (deprecated)(should be "
         "zero)"),
    TEXT("Machine is based on 32-bit"),
    TEXT("File has no Debugging information"),
    TEXT("If the image is on removable media, fully load it and copy it to the "
         "swap file"),
    TEXT("If the image is on network media, fully load it and copy it to the "
         "swap file"),
    TEXT("File is a system file"),
    TEXT("File is a dynamic-link library (DLL)"),
    TEXT("File should be run only on a uniprocessor machine"),
    TEXT("Big endian: MSB precedes the LSB in memory (deprecated)(should be "
         "zero)")};

WORD wMagicTypeLength = 4;

TCHAR szMagicTypeDesc[][15] = {
    TEXT("UNKNOWN"),
    TEXT("PE32"),
    TEXT("PE32+/PE64"),
    TEXT("ROM"),
};

WORD wMagicType[] = {
    0x0,
    0x10b,
    0x20b,
    0x107,
};

std::map<WORD, WORD> magicTypeToIndex{
    {0x0, 0},
    {0x10b, 1},
    {0x20b, 2},
    {0x107, 3},
};

TCHAR szSubsystemDesc[][50] = {
    TEXT("IMAGE_SUBSYSTEM_UNKNOWN"),
    TEXT("IMAGE_SUBSYSTEM_NATIVE"),
    TEXT("IMAGE_SUBSYSTEM_WINDOWS_GUI"),
    TEXT("IMAGE_SUBSYSTEM_WINDOWS_CUI"),
    TEXT("IMAGE_SUBSYSTEM_OS2_CUI"),
    TEXT("IMAGE_SUBSYSTEM_POSIX_CUI"),
    TEXT("IMAGE_SUBSYSTEM_NATIVE_WINDOWS"),
    TEXT("IMAGE_SUBSYSTEM_WINDOWS_CE_GUI"),
    TEXT("IMAGE_SUBSYSTEM_EFI_APPLICATION"),
    TEXT("IMAGE_SUBSYSTEM_EFI_BOOT_ SERVICE_DRIVER"),
    TEXT("IMAGE_SUBSYSTEM_EFI_RUNTIME_ DRIVER"),
    TEXT("IMAGE_SUBSYSTEM_EFI_ROM"),
    TEXT("IMAGE_SUBSYSTEM_XBOX"),
    TEXT("IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION")};

WORD wSubsystemLength = 14;

WORD wSubsystem[] = {0, 1, 2, 3, 5, 7, 8, 9, 10, 11, 12, 13, 14, 16};

std::map<WORD, WORD> subsystemToIndex = {
    {0, 0}, {1, 1},  {2, 2},  {3, 3},   {5, 4},   {7, 5},   {8, 6},
    {9, 7}, {10, 8}, {11, 9}, {12, 10}, {13, 11}, {14, 12}, {16, 13}};

DWORD dllCharacteristicsBegin = 0x1;
DWORD dllCharacteristicsEnd = 0x8000;

std::map<WORD, WORD> dllCharacToIndex = {
    {0x0001, 0},  {0x0002, 1},  {0x0004, 2},  {0x0008, 3},  {0x0020, 4},
    {0x0040, 5},  {0x0080, 6},  {0x0100, 7},  {0x0200, 8},  {0x0400, 9},
    {0x0800, 10}, {0x1000, 11}, {0x2000, 12}, {0x4000, 13}, {0x8000, 14}};

INT dllCharacteristicsCheckBoxID[] = {IDC_CHECK_UNKNOWN,
                                      IDC_CHECK_UNKNOWN,
                                      IDC_CHECK_UNKNOWN,
                                      IDC_CHECK_UNKNOWN,
                                      IDC_CHECK_HIGH_ENTROPY_VA,
                                      IDC_CHECK_DYNAMIC_BASE,
                                      IDC_CHECK_CODE_FORCE_INTEGRITY,
                                      IDC_CHECK_NX_COMPAT,
                                      IDC_CHECK_NO_ISOLATION,
                                      IDC_CHECK_NO_SEH,
                                      IDC_CHECK_NO_BIND,
                                      IDC_CHECK_APPCONTAINER,
                                      IDC_CHECK_WDM_DRIVER,
                                      IDC_CHECK_GUARD_CF,
                                      IDC_CHECK_TERMINAL_SERVER_AWARE};

INT directoryRVAEditID[] = {
    IDC_EDIT_EXPORT_TABLE_RVA,
    IDC_EDIT_IMPORT_TABLE_RVA,
    IDC_EDIT_RESOURCE_TABLE_RVA,
    IDC_EDIT_EXCEPTION_TABLE_RVA,
    IDC_EDIT_CERTIFICATE_TABLE_RVA,
    IDC_EDIT_BASE_RELOC_TABLE_RVA,
    IDC_EDIT_DEBUG_RVA,
    IDC_EDIT_ARCHITECTURE_RVA,
    IDC_EDIT_GLOBAL_PTR_RVA,
    IDC_EDIT_TLS_TABLE_RVA,
    IDC_EDIT_LOAD_CONFIG_TABLE_RVA,
    IDC_EDIT_BOUND_IMPORT_RVA,
    IDC_EDIT_IAT_RVA,
    IDC_EDIT_DELAY_IMPORT_RVA,
    IDC_EDIT_CLR_RUNTIMER_HEADER_RVA,
    IDC_EDIT_RESERVED_RVA,
};

INT directorySizeEditID[] = {
    IDC_EDIT_EXPORT_TABLE_SIZE,
    IDC_EDIT_IMPORT_TABLE_SIZE,
    IDC_EDIT_RESOURCE_TABLE_SIZE,
    IDC_EDIT_EXCEPTION_TABLE_SIZE,
    IDC_EDIT_CERTIFICATE_TABLE_SIZE,
    IDC_EDIT_BASE_RELOC_TABLE_SIZE,
    IDC_EDIT_DEBUG_SIZE,
    IDC_EDIT_ARCHITECTURE_SIZE,
    IDC_EDIT_GLOBAL_PTR_SIZE,
    IDC_EDIT_TLS_TABLE_SIZE,
    IDC_EDIT_LOAD_CONFIG_TABLE_SIZE,
    IDC_EDIT_BOUND_IMPORT_SIZE,
    IDC_EDIT_IAT_SIZE,
    IDC_EDIT_DELAY_IMPORT_SIZE,
    IDC_EDIT_CLR_RUNTIMER_HEADER_SIZE,
    IDC_EDIT_RESERVED_SIZE,
};

INT directoryMoreInfoButtonID[] = {
    IDC_BUTTON_DIRECTORY_EXPORT,      IDC_BUTTON_DIRECTORY_IMPORT,
    IDC_BUTTON_DIRECTORY_RESOURCE,    IDC_BUTTON_DIRECTORY_EXCEPTION,
    IDC_BUTTON_DIRECTORY_CERTIFICATE, IDC_BUTTON_DIRECTORY_BASE_RELOC,
    IDC_BUTTON_DIRECTORY_DEBUG,       IDC_BUTTON_DIRECTORY_ARCH,
    IDC_BUTTON_DIRECTORY_GLOBAL_PTR,  IDC_BUTTON_DIRECTORY_TLS,
    IDC_BUTTON_DIRECTORY_LOAD_CONFIG, IDC_BUTTON_DIRECTORY_BOUND_IMPORT,
    IDC_BUTTON_DIRECTORY_IAT,         IDC_BUTTON_DIRECTORY_DELAY_IMPORT,
    IDC_BUTTON_DIRECTORY_CLR_RUNTIME, IDC_BUTTON_DIRECTORY_RESERVED};

INT directoryHexButtonID[] = {
    IDC_BUTTON_DIRECTORY_EXPORT_HEX,      IDC_BUTTON_DIRECTORY_IMPORT_HEX,
    IDC_BUTTON_DIRECTORY_RESOURCE_HEX,    IDC_BUTTON_DIRECTORY_EXCEPTION_HEX,
    IDC_BUTTON_DIRECTORY_CERTIFICATE_HEX, IDC_BUTTON_DIRECTORY_BASE_RELOC_HEX,
    IDC_BUTTON_DIRECTORY_DEBUG_HEX,       IDC_BUTTON_DIRECTORY_ARCH_HEX,
    IDC_BUTTON_DIRECTORY_GLOBAL_PTR_HEX,  IDC_BUTTON_DIRECTORY_TLS_HEX,
    IDC_BUTTON_DIRECTORY_LOAD_CONFIG_HEX, IDC_BUTTON_DIRECTORY_BOUND_IMPORT_HEX,
    IDC_BUTTON_DIRECTORY_IAT_HEX,         IDC_BUTTON_DIRECTORY_DELAY_IMPORT_HEX,
    IDC_BUTTON_DIRECTORY_CLR_RUNTIME_HEX, IDC_BUTTON_DIRECTORY_RESERVED_HEX,
};

DWORD sectionCharacteristicsBegin = 0x1;
DWORD sectionCharacteristicsEnd = 0x80000000;
DWORD sectionCharacteristicsSize = 41;

DWORD sectionCharacterisValues[] = {
    0x00000000, 0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010,
    0x00000020, 0x00000040, 0x00000080, 0x00000100, 0x00000200, 0x00000400,
    0x00000800, 0x00001000, 0x00008000, 0x00020000, 0x00020000, 0x00040000,
    0x00080000, 0x00100000, 0x00200000, 0x00300000, 0x00400000, 0x00500000,
    0x00600000, 0x00700000, 0x00800000, 0x00900000, 0x00A00000, 0x00B00000,
    0x00C00000, 0x00D00000, 0x00E00000, 0x01000000, 0x02000000, 0x04000000,
    0x08000000, 0x10000000, 0x20000000, 0x40000000, 0x80000000};

std::map<DWORD, DWORD> sectionCharacToIndex = {
    {0x00000000, 0},  {0x00000001, 1},  {0x00000002, 2},  {0x00000004, 3},
    {0x00000008, 4},  {0x00000010, 5},  {0x00000020, 6},  {0x00000040, 7},
    {0x00000080, 8},  {0x00000100, 9},  {0x00000200, 10}, {0x00000400, 11},
    {0x00000800, 12}, {0x00001000, 13}, {0x00008000, 14}, {0x00020000, 15},
    {0x00020000, 16}, {0x00040000, 17}, {0x00080000, 18}, {0x00100000, 0},
    {0x00200000, 0},  {0x00300000, 0},  {0x00400000, 0},  {0x00500000, 0},
    {0x00600000, 0},  {0x00700000, 0},  {0x00800000, 0},  {0x00900000, 0},
    {0x00A00000, 0},  {0x00B00000, 0},  {0x00C00000, 0},  {0x00D00000, 0},
    {0x00E00000, 0},  {0x01000000, 19}, {0x02000000, 20}, {0x04000000, 21},
    {0x08000000, 22}, {0x10000000, 23}, {0x20000000, 24}, {0x40000000, 25},
    {0x80000000, 26}};

INT sectionCharacteristicsCheckBoxID[] = {
    IDC_CHECK_UNKNOWN,
    IDC_CHECK_UNKNOWN,
    IDC_CHECK_UNKNOWN,
    IDC_CHECK_UNKNOWN,
    IDC_CHECK_TYPE_NO_PAD,
    IDC_CHECK_UNKNOWN,
    IDC_CHECK_SCN_CNT_CODE,
    IDC_CHECK_CNT_INITIALIZED_DATA,
    IDC_CHECK_CNT_UNINITIALIZED_DATA,
    IDC_CHECK_UNKNOWN,
    IDC_CHECK_LNK_INFO,
    IDC_CHECK_UNKNOWN,
    IDC_CHECK_LNK_REMOVE,
    IDC_CHECK_LNK_COMDAT,
    IDC_CHECK_SCN_GPREL,
    IDC_CHECK_UNKNOWN,
    IDC_CHECK_UNKNOWN,
    IDC_CHECK_UNKNOWN,
    IDC_CHECK_UNKNOWN,
    IDC_CHECK_LNK_NRELOC_OVFL,
    IDC_CHECK_MEM_DISCARDABLE,
    IDC_CHECK_MEM_NOT_CACHED,
    IDC_CHECK_MEM_NOT_PAGED,
    IDC_CHECK_MEM_SHARED,
    IDC_CHECK_MEM_EXECUTE,
    IDC_CHECK_MEM_READ,
    IDC_CHECK_MEM_WRITE,
};

WORD wSectionAlignLength = 13;

TCHAR szSectionAlignBytesDesc[][6] = {
    TEXT("1"),    TEXT("2"),    TEXT("4"),   TEXT("8"),   TEXT("16"),
    TEXT("32"),   TEXT("64"),   TEXT("128"), TEXT("256"), TEXT("512"),
    TEXT("1024"), TEXT("2048"), TEXT("4096")};

TCHAR szDebugInfoType[][10]{
    TEXT("UNKNOWN"), TEXT("COFF"),      TEXT("CODEVIEW"), TEXT("FPO"),
    TEXT("MISC"),    TEXT("EXCEPTION"), TEXT("FIXUP"),    TEXT("BORLAND"),
};

// WORD szSectionAlignValue[] = {
//
//};