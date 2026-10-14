# PEViewer 架构与 PE32 / PE32+ 支持计划

本文按当前工作区实现记录程序入口、数据流和类型约定，并列出支持 PE32、PE32+ 文件需要继续完成的工作。当前已完成查看器的 x86 / x64 指针与回调适配；文件解析仍按 PE32 布局实现。

## 入口与文件职责

程序使用 Win32 对话框和公共控件。主窗口显示进程与模块列表；`PE Editor` 按钮打开磁盘文件，再通过子对话框显示 PE 头、节表和数据目录。

| 文件 | 职责 |
| --- | --- |
| [PEViewer.cpp](PEViewer.cpp) | `WinMain`、公共控件初始化、Common Controls manifest、主对话框入口。 |
| [windowProc.cpp](windowProc.cpp)、[windowProc.h](windowProc.h) | 各对话框的消息回调，分派初始化、按钮点击和 ListView 通知。 |
| [messageCallback.cpp](messageCallback.cpp)、[messageCallback.h](messageCallback.h) | 文件选择、子对话框打开、控件初始化和部分字段调整逻辑。 |
| [readPE.cpp](readPE.cpp)、[readPE.h](readPE.h) | 文件读入、PE 头定位、RVA 转文件偏移、字段和目录解析、结果填入控件。 |
| [handleProcessAndModule.cpp](handleProcessAndModule.cpp)、[handleProcessAndModule.h](handleProcessAndModule.h) | 进程枚举、架构判断调用、模块查询和进程 / 模块列表显示。 |
| [utility.cpp](utility.cpp)、[utility.h](utility.h) | 调试权限、错误输出、进程架构判断、时间和窗口布局辅助函数。 |
| [global.cpp](global.cpp)、[global.h](global.h)、[structs.h](structs.h) | 字段说明、控件 ID 对照、颜色以及列表行的显示数据。 |
| [resource.rc](resource.rc)、[resource.h](resource.h) | 对话框布局、控件 ID 和中文资源。 |
| [PEViewer.sln](PEViewer.sln)、[PEViewer.vcxproj](PEViewer.vcxproj) | Visual Studio 的 Debug / Release、Win32 / x64 构建配置。 |

## 数据与页面

```text
WinMain
  → MainDialogProc
  → InitProcessListView / InitModulesListView
  → EnumProcess
  → 点击进程：EnumModules → 模块列表

点击 PE Editor
  → onPeEditor → GetOpenFileName
  → readPeFile → pFileBuffer（磁盘文件的原始字节）
  → initPE → DOS / NT / File / Optional / Section Header 与 DataDirectory
  → PeEditorProc
  → 文件头、可选头、汇总、节表、数据目录对话框
  → 导入表、导出表、绑定导入、调试目录及导入函数列表
```

`readPeFile()` 将文件读入堆缓冲区。此缓冲区保留磁盘文件布局；节的数据仍位于 `PointerToRawData` 指定的位置。它没有被 Windows 加载器映射为可执行映像，所以 RVA 通常需要先转为文件偏移，才能加到缓冲区起点上。

`initPE()` 当前把解析结果保存在全局指针中。它固定设置 `pOptionalHeader32`，并由该结构读取 `NumberOfRvaAndSizes` 和 `DataDirectory`。`readIntThunk()` 当前显式使用 `IMAGE_THUNK_DATA32`，每项前进 4 字节。这些布局假设是 PE32+ 支持的主要改造位置。

界面包含编辑按钮和字段调整逻辑，但尚未形成完整的文件保存 / 写回流程。以下计划先以正确读取和显示为验收范围。

## 查看器位数、文件格式与地址

查看器的编译平台决定本进程的指针宽度；被查看文件的可选头 `Magic` 决定文件结构布局。两者需要分别处理。

| 项目 | 32 位形式 | 64 位形式 | 选择依据 |
| --- | --- | --- | --- |
| 查看器自身 | x86 / Win32，指针 4 字节 | x64，指针 8 字节 | 构建平台。 |
| PE 可选头 | PE32：`Magic = 0x10B`，`IMAGE_OPTIONAL_HEADER32` | PE32+：`Magic = 0x20B`，`IMAGE_OPTIONAL_HEADER64` | 文件内容。 |
| 导入 thunk | `IMAGE_THUNK_DATA32`，每项 4 字节 | `IMAGE_THUNK_DATA64`，每项 8 字节 | 文件内容。 |

目标是让 x86、x64 两个查看器都能离线读取 PE32、PE32+ 文件。`Machine` 用于显示目标 CPU 架构；布局仍应由 `Magic` 判断。不能使用 `_WIN64` 决定外部文件格式，也不能依赖随编译平台变化的 `IMAGE_NT_HEADERS`、`IMAGE_THUNK_DATA` 别名选择文件布局。

当前项目采用以下类型约定，继续保持已有的 C 风格强制转换：

| 数据 | 类型与处理方式 |
| --- | --- |
| 本进程缓冲区地址、需要保存地址的整数 | 指针或 `DWORD_PTR`。地址运算使用 `(DWORD_PTR)pFileBuffer + fileOffset`，不得先存进 `DWORD`。 |
| 通过控件消息传递结构体指针 | `LPARAM`，例如 `(LPARAM)&lv`、`(LPARAM)&vitem`。 |
| 对话框回调 | `INT_PTR CALLBACK`，声明与定义一致，直接传给 `DialogBox` / `DialogBoxParam`。 |
| PE 的 RVA、32 位字段、导出函数 / 名称 RVA 数组 | `DWORD`，PE32+ 中这些字段也保持 32 位。 |
| PE32+ 的 `ImageBase`、栈 / 堆 Reserve、Commit、64 位 thunk 值 | `ULONGLONG`。x86 查看器里的 `DWORD_PTR` 仍为 32 位，不能用于保存这些文件字段。 |
| 实际缓冲区长度、索引和访问范围 | `SIZE_T`；读取原始文件长度时使用能容纳 64 位长度的接口，并检查本进程可分配范围。 |

进程列表里的 Image Base 来自 `MODULEINFO.lpBaseOfDll`，表示模块的实际加载基址。PE 文件头里的 `ImageBase` 是首选加载地址；重定位时两者可能不同。`SizeOfImage` 表示映像加载后的虚拟地址范围大小，包含头部并按 `SectionAlignment` 对齐；该字段在 PE32 和 PE32+ 中均为 `DWORD`，不表示磁盘文件大小或当前驻留的物理内存量。

## 当前已完成的适配

- 地址运算和保存地址的中间变量改用 `DWORD_PTR`，RVA 与本进程地址分别保存。
- ListView 消息的结构体指针参数改用 `LPARAM`。
- 20 个对话框回调改为 `INT_PTR CALLBACK`，移除调用处的 `DLGPROC` 强转。
- 已有模块基址显示改为按本进程指针宽度格式化，避免 `DWORD` 截断。
- 本地 `build.ps1` 支持 `x86`、`x64`、`Win32` 平台参数，并按配置和平台分别设置输出、中间目录。
- 中文源码与资源已使用 UTF-8 BOM；`resource.rc` 使用代码页 65001。修改时需要保留编码，避免再次把中文注释保存为 `U+FFFD` 替换字符。

以上解决了双平台指针和调用签名问题，PE32+ 的可选头与导入 thunk 解析仍待实现。

## 后续工作：PE32 / PE32+ 文件读取

### 1. 文件读取、格式识别与访问边界

主要文件：[readPE.cpp](readPE.cpp)、[readPE.h](readPE.h)、[messageCallback.cpp](messageCallback.cpp)。

- [ ] 改善 `readPeFile()`：正确检查 `*pFileBuffer` 的分配结果，验证文件长度和实际读取字节数，失败时清空输出指针；打开失败后不要继续执行 `initPE()`。重新打开或关闭文件时释放旧缓冲区。
- [ ] 在解析状态中明确记录文件长度、PE 格式、可选头 32 / 64 指针、节表、节数量和有效数据目录数量。可逐步将分散的全局指针收拢为一个解析上下文，供读取函数使用。
- [ ] 验证 DOS 签名、非负且在文件范围内的 `e_lfanew`、NT 签名、File Header、Optional Header 和整个节表的可读范围；检查偏移加法、数量乘法是否溢出。
- [ ] 读取 `Magic` 后选择 `IMAGE_OPTIONAL_HEADER32` 或 `IMAGE_OPTIONAL_HEADER64`。不支持的格式明确报错；节表位置使用 File Header 的 `SizeOfOptionalHeader`。
- [ ] 从对应可选头取得数据目录；结合 `SizeOfOptionalHeader`、`NumberOfRvaAndSizes` 和文件边界验证，避免读越界或超过当前 UI 支持的 16 项。
- [ ] 改造 `rvaToFileOffset()`，将成功 / 失败与偏移结果分开。头部 RVA、找不到所属节、节的虚拟零填充尾部、缺失原始数据、越界字符串都要有明确处理；遍历使用局部节指针，避免临时移动全局 `pSectionHeader`。

阶段验收：x86、x64 查看器均能正确识别两种格式，定位 File Header、节表和数据目录；空文件、截断头部和非法偏移得到错误提示。

### 2. 可选头、汇总页面与 64 位字段显示

主要文件：[readPE.cpp](readPE.cpp)、[readPE.h](readPE.h)、[messageCallback.cpp](messageCallback.cpp)、[windowProc.cpp](windowProc.cpp)、[windowProc.h](windowProc.h)、[resource.rc](resource.rc)、[resource.h](resource.h)。

- [ ] 让 `readOptinalHeader32()`、`readHeaderInfo()` 及调用它们的页面按解析格式选择字段来源。公共字段可使用统一读取函数；`BaseOfData` 只在 PE32 中显示。
- [ ] 保留 `writeToText()` 的 32 位字段通路，另增接收 `ULONGLONG` 的 64 位显示通路，并同步头文件声明。格式与参数类型配套，例如 64 位十六进制采用 `%016I64X`；不要把参数统一加宽后继续使用 `%X`。
- [ ] PE32+ 的 `ImageBase`、`SizeOfStackReserve/Commit`、`SizeOfHeapReserve/Commit` 全程保留 64 位值，包括控件输入、扫描和后续计算。
- [ ] 检查控件宽度、输入限制、可选头标题和所有直接引用 `pOptionalHeader32` 的字段调整函数。共享的 `SizeOfImage`、对齐、RVA 等字段仍按规范保持 32 位。

阶段验收：两种查看器都能完整显示高于 4 GB 的 PE32+ 首选基址和 64 位字段；PE32 的原有显示正确。

### 3. 导入表与其他已实现的数据目录

主要文件：[readPE.cpp](readPE.cpp)、[readPE.h](readPE.h)、[messageCallback.cpp](messageCallback.cpp)。

- [ ] 导入 thunk 根据文件格式分别使用 `IMAGE_THUNK_DATA32/64`，步长为 4 / 8；显示值时保留对应宽度。
- [ ] 使用 `IMAGE_ORDINAL_FLAG32/64`，将判定写为 `(value & flag) != 0`。当前表达式 `value & 0x80000000 == 0x80000000` 存在优先级问题，需要一起修正。
- [ ] 支持名称导入和序号导入；`OriginalFirstThunk` 为零时按未绑定文件的有效信息处理 `FirstThunk` 回退；缺失或无法解释的数据明确提示。循环、名称字符串和终止项必须位于可读范围内。
- [ ] 导出表继续使用 32 位 RVA 数组和 16 位名称序号索引；修正无名导出的索引 / 哨兵处理，导出序号使用 `Base + functionIndex`，识别转发导出，并检查所有数组和字符串边界。
- [ ] 复核节表、绑定导入和调试目录的范围与错误处理。它们的大部分字段布局在 PE32 / PE32+ 间共用，不能因为目标为 64 位就把所有字段加宽。
- [ ] 接入其他目录时分别核对规范：Security / Certificate 目录地址是文件偏移；TLS、Load Config 等结构具有需要区分的 32 / 64 位形式。

阶段验收：两种查看器均能显示两种文件格式的节表、导入 DLL / 函数、导出函数、绑定导入和调试目录，并正确处理不存在的目录。

### 4. 建立可重复的验收用例

- [ ] 保存可重复生成的 PE32 和 PE32+ 样本、对应预期值及测试入口；临时 `obj` 文件夹内的验证程序不会随仓库提交，不能替代正式回归测试。
- [ ] 四种“查看器位数 × 文件格式”组合都验证头部、首选基址、节表、目录定位、名称 / 序号导入、无名 / 转发导出和字段宽度。
- [ ] x64 测试将文件缓冲区放在 4 GB 以上；x86 测试读取首选基址高于 4 GB 的 PE32+ 文件，分别检测本进程指针截断和文件字段截断。
- [ ] 增加缺失目录、异常长度、截断 thunk、无终止符字符串等失败用例，确认错误路径不崩溃、不遗留悬空指针。
- [ ] 以 `dumpbin /headers /imports /exports` 等工具交叉核对真实 EXE / DLL，并验证 Debug / Release 两种构建。

| 查看器 | PE32 文件 | PE32+ 文件 |
| --- | --- | --- |
| x86 | 头部、节表、目录、显示与错误路径 | 同左，并验证完整的 64 位文件字段。 |
| x64 | 同左，并验证 4 GB 以上的缓冲区地址 | 同左，并验证 64 位 thunk。 |

## 独立待办：进程与已加载模块

离线读取 PE32+ 文件不依赖 x64 辅助程序。下面的工作属于主窗口的进程查询功能，可以与文件格式解析分开推进。

- [ ] 在 [handleProcessAndModule.cpp](handleProcessAndModule.cpp) 的 x64 分支补模块信息查询。目前它只查进程路径，Image Base 和 Size of Image 直接填写 `<Unknown>`；加载基址格式化已经修正，查询尚未补齐。
- [ ] 在 `OpenProcess`、`EnumProcessModules/Ex`、`GetModuleInformation` 和模块路径查询失败后立即记录错误码；区分拒绝访问、进程退出、架构限制和其他错误。仅查询名称时可采用较低的查询权限。
- [ ] 检查模块数组容量与 `cbNeeded`，避免模块较多时越界；统一关闭进程句柄并处理所有失败路径。
- [ ] 在 [utility.cpp](utility.cpp) 修正 `setDebugPrivilege()` 的返回值和失败路径资源清理。目前 `GetLastError()` 非零后仍会返回 `TRUE`。权限调整成功也不保证可以访问 System 或受保护进程。
- [ ] x64 查看器可用 `EnumProcessModulesEx` 查询普通 x86 / x64 进程；x86 查看器调用这套 API 查询 x64 进程会失败，通常返回 299。若保留 x86 主程序并要求该能力，可由独立 x64 EXE 查询，通过管道等返回模块名称、路径及固定 64 位的基址。辅助程序同样受进程权限限制。

## 构建与当前验证结果

Visual Studio 解决方案提供 `Debug/Release × x86/x64`，其中项目的 x86 平台名是 `Win32`。需要 Visual Studio C++ 工具集和项目指定的 Windows SDK。

当前本地构建脚本要求 PowerShell 7，可在仓库根目录执行：

```powershell
pwsh -File .\build.ps1 -Configuration Debug -Platform x86
pwsh -File .\build.ps1 -Configuration Debug -Platform x64
pwsh -File .\build.ps1 -Configuration Release -Platform x86
pwsh -File .\build.ps1 -Configuration Release -Platform x64
```

脚本默认 `Debug/x86`，接受 `Win32` 作为 x86 别名；输出位于 `bin/<配置>/Win32` 或 `bin/<配置>/x64`，中间文件位于对应的 `obj` 目录。它还会合并子进程环境中仅大小写不同的 PATH 项。

`build.ps1` 当前按 [.gitignore](.gitignore) 的 `/build.ps1` 规则忽略，不随仓库克隆分发。若决定将命令行构建作为正式项目入口，需要先解除忽略并提交脚本。

已完成的验证：

- x86、x64 的 Debug 和 Release 四组构建通过，`C4311/C4312/C4302` 指针转换相关警告为零；其他既有警告仍需按后续任务处理。
- 两种平台使用合成 PE32 数据创建了 10 个文件查看对话框，检查了节表、导入函数、导出、绑定导入和调试目录的列表及预期输出。
- x64 验证中的缓冲区位于 4 GB 以上，头部 / 节表指针、RVA 转换、目录列表名称和条目数正确；修改后的源码中文注释、编码与差异空白检查通过。

上述验证覆盖已完成的指针和回调适配。完整 PE32+ 支持以及真实文件、损坏输入的完整矩阵仍按前面的验收计划推进。

## 规范参考

- [Microsoft PE / COFF 格式](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format)：文件布局、字段宽度、导入 / 导出与数据目录。
- [Windows 指针使用规则](https://learn.microsoft.com/en-us/windows/win32/winprog64/rules-for-using-pointers)：本进程地址和消息参数的双平台类型选择。
- [DLGPROC](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-dlgproc)、[MODULEINFO](https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-moduleinfo)：回调签名与实际加载映像信息。
- [EnumProcessModules](https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-enumprocessmodules)、[EnumProcessModulesEx](https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-enumprocessmodulesex)：进程模块枚举的跨位数限制。
