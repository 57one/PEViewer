# PEViewer 架构与 PE32 / PE32+ 支持计划

本文按当前工作区实现记录程序入口、数据流和类型约定，并列出支持 PE32、PE32+ 文件需要继续完成的工作。当前已完成查看器的 x86 / x64 指针与回调适配，使用统一 PEContext；正常文件入口已开放 PE32 / PE32+，支持头部、节表、已有目录页面和 32 / 64 位导入 thunk。

## 入口与文件职责

程序使用 Win32 对话框和公共控件。主窗口显示进程与模块列表；`PE Editor` 按钮打开磁盘文件，再通过子对话框显示 PE 头、节表和数据目录。

| 文件 | 职责 |
| --- | --- |
| [PEViewer.cpp](src/app/PEViewer.cpp) | `WinMain`、公共控件初始化、Common Controls manifest、主对话框入口。 |
| [windowProc.cpp](src/ui/windowProc.cpp)、[windowProc.h](src/ui/windowProc.h) | 各对话框的消息回调，分派初始化、按钮点击和 ListView 通知。 |
| [messageCallback.cpp](src/ui/messageCallback.cpp)、[messageCallback.h](src/ui/messageCallback.h) | 文件选择、子对话框打开、控件初始化和部分字段调整逻辑。 |
| [PEContext.cpp](src/pe/PEContext.cpp)、[PEContext.h](src/pe/PEContext.h) | 文件所有权、长度、格式、头部与节表校验，以及有边界检查的 RVA / 字符串访问；独立于界面。 |
| [readPE.cpp](src/pe/readPE.cpp)、[readPE.h](src/pe/readPE.h) | 现阶段保留头部、汇总和节表的读取 / 显示适配；目录解析和填表逻辑已迁出。 |
| [directoryDetails.cpp](src/pe/directoryDetails.cpp)、[directoryDetails.h](src/pe/directoryDetails.h) | 独立于控件的导入、IAT、导出、绑定导入、调试、TLS 和重定位解析；各目录使用普通函数，返回数值字段、名称、条目集合和异常说明。 |
| [resourceDetails.cpp](src/pe/resourceDetails.cpp) | 资源目录解析，保留内部 `ResourceReader` 类，集中管理递归路径、循环检查和名称总量；模型声明复用 `directoryDetails.h`，返回树节点、目录头、数据描述符和异常说明。 |
| [directoryDialogs.cpp](src/ui/directoryDialogs.cpp)、[directoryDialogs.h](src/ui/directoryDialogs.h) | 数据目录总览及详情的结果显示、十六进制格式化、列表填充和状态提示；IAT / 重定位页面还在此处理初始化、选择和关闭。旧页面的列初始化和消息分派继续由 messageCallback / windowProc 组织。 |
| [resourceDialog.cpp](src/ui/resourceDialog.cpp) | 资源树、节点详情和状态显示；树项使用数值模型索引，窗口拥有解析结果，切换选择不从显示文字反推地址。 |
| [tlsDialog.cpp](src/ui/tlsDialog.cpp) | TLS 字段和回调列表的独立对话框，负责格式标题、32 / 64 位十六进制显示、列宽和错误状态。 |
| [handleProcessAndModule.cpp](src/process/handleProcessAndModule.cpp)、[handleProcessAndModule.h](src/process/handleProcessAndModule.h) | 进程枚举、架构判断调用、模块查询和进程 / 模块列表显示。 |
| [utility.cpp](src/common/utility.cpp)、[utility.h](src/common/utility.h) | 调试权限、错误输出、进程架构判断、时间和窗口布局辅助函数。 |
| [global.cpp](src/common/global.cpp)、[global.h](src/common/global.h)、[structs.h](src/common/structs.h) | 字段说明、控件 ID 对照、颜色以及列表行的显示数据。 |
| [resource.rc](resources/resource.rc)、[resource.h](resources/resource.h) | 对话框布局、控件 ID 和中文资源。 |
| [pe-directories.md](docs/pe-directories.md)、[figures](docs/figures/) | PE 数据目录说明、资源目录结构图及 PNG / SVG 图片。 |
| [PEViewer.sln](PEViewer.sln)、[PEViewer.vcxproj](PEViewer.vcxproj) | Visual Studio 的 Debug / Release、Win32 / x64 构建配置。 |

## 目录与构建布局

源码、头文件和资源脚本已按职责归类，继续由一个 `PEViewer.vcxproj` 编译主程序，生成 `PEViewer.exe`；`src` 下的子目录属于同一个工程。相关 `.cpp`、`.h` 放在同一模块目录中，解决方案和工程文件保留在根目录。

以下是当前目录结构。`tests` 使用独立工程编译，验证源码和样本生成代码不加入主程序的 `ClCompile` 列表。

```text
PEViewer/
├─ PEViewer.sln
├─ PEViewer.vcxproj
├─ PEViewer.vcxproj.filters
├─ architecture.md
├─ .gitignore
├─ docs/
│  ├─ pe-directories.md          # 数据目录入口与资源结构说明
│  └─ figures/
│     ├─ pe-resource-layout.png
│     └─ pe-resource-layout.svg
├─ src/
│  ├─ app/
│  │  └─ PEViewer.cpp
│  ├─ ui/
│  │  ├─ windowProc.cpp
│  │  ├─ windowProc.h
│  │  ├─ messageCallback.cpp
│  │  ├─ messageCallback.h
│  │  ├─ directoryDialogs.cpp
│  │  ├─ directoryDialogs.h
│  │  ├─ resourceDialog.cpp
│  │  └─ tlsDialog.cpp
│  ├─ pe/
│  │  ├─ PEContext.cpp
│  │  ├─ PEContext.h
│  │  ├─ readPE.cpp
│  │  ├─ readPE.h
│  │  ├─ directoryDetails.cpp
│  │  ├─ directoryDetails.h
│  │  └─ resourceDetails.cpp
│  ├─ process/
│  │  ├─ handleProcessAndModule.cpp
│  │  └─ handleProcessAndModule.h
│  └─ common/
│     ├─ global.cpp
│     ├─ global.h
│     ├─ structs.h
│     ├─ utility.cpp
│     └─ utility.h
├─ resources/
│  ├─ resource.rc
│  ├─ resource.h
│  └─ bmp/
│     └─ 7eviewer.bmp
├─ scripts/
│  └─ build.ps1
├─ tests/
│  ├─ README.md
│  ├─ portability-smoke.vcxproj
│  ├─ portability-smoke.cpp
│  ├─ pe-context-tests.cpp
│  ├─ directory-details-tests.cpp
│  ├─ resource-details-tests.cpp
│  ├─ tls-details-tests.cpp
│  └─ fixtures/
│     ├─ pe32.h                 # PE32 页面与解析样本
│     ├─ pe64.h                 # PE32+ 页面与解析样本
│     ├─ resources.h            # 双格式资源树与多语言样本
│     └─ tls.h                  # 双格式 TLS 字段、VA 与回调数组样本
├─ bin/                         # 构建产物，忽略提交
└─ obj/                         # 中间文件和临时样本，忽略提交
```

### 第一阶段：文件归类与构建路径（已完成）

这一阶段完成物理目录整理，保持现有函数职责和程序行为，便于独立检查迁移变更。

- [x] 源码、头文件和资源脚本已迁入目标目录，保持现有文件名与编码；`resources/bmp/7eviewer.bmp` 保留在原位置，文档链接同步更新。
- [x] 工程文件与 `.filters` 已更新文件路径和模块分组。解决方案仍指向根目录工程文件。
- [x] 已配置 C++ 的 `src` / `resources` 搜索路径和资源编译的 `resources` 搜索路径。跨模块引用使用模块前缀，同目录引用保持原形式。
- [x] `resource.rc` 引用同目录 `resource.h` 和 `bmp/7eviewer.bmp`，保留 UTF-8 BOM、代码页 65001 和资源编辑器的 `TEXTINCLUDE`。
- [x] 新增可随仓库分发的 `scripts/build.ps1`。脚本从自身目录定位根目录，并初始化对应 Visual Studio 的编译环境。
- [x] 工程统一设置 `OutDir = $(ProjectDir)bin\$(Configuration)\$(Platform)\`、`IntDir = $(ProjectDir)obj\$(Configuration)\$(Platform)\`；VS 和脚本使用相同目录。
- [x] `bin`、`obj`、`.vs` 等生成文件继续忽略；移除过时的根目录 `/build.ps1` 忽略规则，正式源码、资源、文档和脚本不被忽略。
- [x] `tests/portability-smoke.vcxproj` 独立编译验证程序，`tests/fixtures/pe32.h` 保存可重复生成的样本和预期值。生成样本、日志与验证产物位于 `obj` / `bin`。

阶段验收：Debug / Release × Win32 / x64 四组构建及独立验证通过；工程与 `.filters` 的路径、分组和资源引用有效；PE32 页面结果符合确定性样本的预期。源码迁移只调整跨模块引用和资源相对路径，生成文件不进入提交。

建议将这一阶段作为单独提交；之后再推进解析接口和 PE32+ 功能，便于区分路径问题与功能问题。

### 第二阶段：明确模块职责与依赖

目录归类完成后，结合后面的 PE32 / PE32+ 支持计划逐步调整函数职责。

| 模块 | 目标职责 |
| --- | --- |
| `app` | 程序入口、初始化和整体生命周期。 |
| `ui` | 对话框消息、控件更新、显示格式化和用户操作；消费解析与进程查询结果。 |
| `pe` | 文件读取、格式识别、边界检查和目录解析；返回结构化结果及错误信息。可使用 Windows 的 PE 类型定义，接口不依赖 `HWND`、控件 ID 或 ListView。 |
| `process` | 进程 / 模块查询、权限和句柄管理；返回查询结果，列表绘制移入 `ui`。 |
| `common` | 模块间共用且职责明确的基础类型和辅助函数。 |
| `resources` | 界面布局、资源 ID 和图片。 |
| `docs` | PE 文件格式学习说明和结构示意图；功能进度与实现计划集中记录在本文。 |
| `scripts` / `tests` | 可复用构建、验证入口、样本生成和预期结果。 |

- [x] 已建立统一 `PEContext`，集中管理文件所有权、长度、格式和头部 / 目录信息；UI 持有当前文档上下文，读取函数显式接收上下文，原分散全局解析指针已移除。
- [x] 已实现的目录解析与显示已分离：`directoryDetails` 返回结构化数据，`directoryDialogs` 负责格式化、填表和错误状态。导入行通过 ListView 的数值 `lParam` 保存描述符索引，排序和修改显示文字不会改变解析对象。
- [ ] 继续将 `readPE.cpp` 保留的头部 / 节表显示适配迁入 `ui`；这些旧接口仍接收 `HWND`，不能将整个 `pe` 目录视为已经完全独立于 UI。
- [ ] `global`、`structs`、`utility` 放入 `common` 是初次迁移时的归类。后续把控件 ID 对照、颜色、列表行显示结构和窗口布局辅助函数移入 `ui`；权限、进程架构和句柄辅助函数移入 `process`。
- [ ] 根据功能规模将 `pe` 下的头部、导入、导出、调试等解析拆为独立文件。`pe`、`process` 和共用基础代码保持对界面模块的独立性，使验证程序能够直接调用解析与查询接口。

## 统一解析上下文（已实现）

`PEContext` 的接口位于 [PEContext.h](src/pe/PEContext.h)，不依赖窗口、资源 ID 或 ListView。`LoadFile()` 拥有并释放文件缓冲区；`Initialize()` 借用外部缓冲区，调用者需保证其有效期，并在释放外部内存前重置上下文。上下文禁止复制，`Reset()` 可重复调用；加载或解析失败会清空旧状态并保存错误说明。

当前 UI 在模态查看窗口结束后重置上下文。字段和目录接口显式接收 `const PEContext&`；RVA 转换和节显示不会移动共享节表指针。已实现目录统一返回结构化结果，不接收 `HWND`、资源 ID 或 ListView；界面适配使用调用者传入的上下文。头部 / 节表显示适配继续按后续计划迁移。

[pe-context-tests.cpp](tests/pe-context-tests.cpp) 覆盖两种格式的磁盘加载、头部、64 位字段、上下文隔离、所有权与重载、失败清理、无效头部、目录数量、RVA 越界和无终止符字符串。当前页面回归覆盖两种格式的 14 种页面、64 位字段输入 / 布局、导入 thunk、名称 / 序号转发导出、资源树和 TLS 字段 / 回调；更多真实文件用例继续推进。

## 数据与页面

```text
WinMain
  → MainDialogProc
  → InitProcessListView / InitModulesListView
  → EnumProcess
  → 点击进程：EnumModules → 模块列表

点击 PE Editor
  → onPeEditor → GetOpenFileName
  → g_peContext.LoadFile → 文件读取、格式识别和头部校验
  → PEContext → 文件缓冲区、长度、DOS / File / Optional / Section Header 与 DataDirectory
  → PeEditorProc
  → 文件头、可选头、汇总、节表、数据目录对话框
  → 导入表、导出表、绑定导入、调试目录及导入函数列表
```

`PEContext::LoadFile()` 将文件读入其拥有的缓冲区，记录实际字节长度并校验头部。缓冲区保留磁盘文件布局，节的数据仍位于 `PointerToRawData` 指定的位置；RVA 需要转换为文件偏移。

`PEContext` 按 `Magic` 选择 `optionalHeader32` / `optionalHeader64`，记录目录数量和稳定的节表起点。`RvaToFileOffset()` 分开返回成功状态与偏移，`ReadRva()` / `ReadString()` 检查文件和节的原始数据边界；`ReadString(rva, maxBytes)` 还可限制包含结尾 NULL 的最大读取范围，用于约束转发字符串不得越过导出数据目录。文件选择入口支持两种格式；头部页面使用相应字段宽度，导入查找表使用显式 32 / 64 位 thunk。

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
| 本进程缓冲区地址、需要保存地址的整数 | 指针或 `DWORD_PTR`。地址运算使用 `context.fileBuffer + fileOffset`，不得先存进 `DWORD`。 |
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
- `scripts/build.ps1` 支持 `x86`、`x64`、`Win32` 平台参数；`-All` 批量构建四组配置，`-Test` 编译并运行独立验证程序。VS 和脚本使用工程中的统一输出路径。
- 中文源码与资源已使用 UTF-8 BOM；`resource.rc` 使用代码页 65001。修改时需要保留编码，避免再次把中文注释保存为 `U+FFFD` 替换字符。

以上覆盖双平台指针、调用签名和解析上下文基础。PE32+ 的头部页面、宽字段输入、正常入口、64 位 thunk、转发导出及已有目录错误提示已接入；其他数据目录和调试载荷继续按后续计划推进。

## 后续工作：PE32 / PE32+ 文件读取

### 1. 文件读取、格式识别与访问边界

主要文件：[readPE.cpp](src/pe/readPE.cpp)、[readPE.h](src/pe/readPE.h)、[messageCallback.cpp](src/ui/messageCallback.cpp)。

- [x] `PEContext::LoadFile()` 使用 64 位文件长度接口，检查分配和实际读取字节数；失败清空状态，UI 停止打开页面。重载、关闭和析构统一释放拥有的缓冲区。
- [x] 统一解析上下文记录文件长度、格式、32 / 64 位可选头、节表、节数量和有效数据目录数量；移除分散的全局解析指针。
- [x] 校验 DOS / NT 签名、非负且有效的 `e_lfanew`、文件头、可选头、节表和 `SizeOfHeaders`；使用减法 / 除法检查访问范围，避免数量与偏移溢出。
- [x] 按 `Magic` 选择 `IMAGE_OPTIONAL_HEADER32/64`，未知格式返回错误；节表位置使用 `SizeOfOptionalHeader`。
- [x] 从对应可选头取得目录，验证声明数量和可选头容量；最多提供 16 项，缺失目录的控件清零并禁用。
- [x] `PEContext::RvaToFileOffset()` 分开返回成功状态与偏移；拒绝无所属节、虚拟零填充、缺失原始数据与越界访问。字符串搜索有范围限制，节遍历使用局部指针。

阶段验收：x86、x64 构建中的解析层均能识别两种格式并定位头部、节表和目录；非法头部返回失败并清空上下文。PE32+ 的页面显示由下一阶段实现。

### 2. 可选头、汇总页面与 64 位字段显示

主要文件：[readPE.cpp](src/pe/readPE.cpp)、[readPE.h](src/pe/readPE.h)、[messageCallback.cpp](src/ui/messageCallback.cpp)、[windowProc.cpp](src/ui/windowProc.cpp)、[windowProc.h](src/ui/windowProc.h)、[resource.rc](resources/resource.rc)、[resource.h](resources/resource.h)。

- [x] `readOptionalHeader()`（原 `readOptinalHeader32()`）、`readHeaderInfo()` 按解析格式选择字段来源，公共字段复用显示逻辑；PE32+ 的 `BaseOfData` 显示 `N/A` 并禁用，PE32 恢复原值和启用状态。可选头标题随格式更新。
- [x] `writeToText()` 保留接收格式字符串的 32 位通路，并增加三参数的 `DWORD` / `ULONGLONG` 重载，分别默认显示 8 / 16 位十六进制。64 位格式固定使用 `%016I64X`，与参数类型配套；声明和调用已同步。
- [x] `ImageBase`、栈 / 堆 Reserve、Commit 保留完整 64 位值；失焦后按文件格式解析并规范化十六进制输入，拒绝符号、无效字符与溢出，失败恢复文件原值。编辑仍为界面预览。
- [x] 可选头与汇总页面加宽数值控件，设置 32 / 64 位输入限制，标题显示文件格式。SizeOfImage / SizeOfHeaders 按钮从对应头部取得原值，检查零对齐和加法溢出；共享字段仍保持 32 位。

阶段验收：两种查看器完整显示和解析高于 4 GB 的 PE32+ 字段；PE32 原有显示正确。正常文件入口已开放 PE32+，页面回归覆盖文本、控件实际字体宽度、输入限制、标题与按钮行为。

### 3. 导入表与其他已实现的数据目录

主要文件：[directoryDetails.cpp](src/pe/directoryDetails.cpp)、[directoryDetails.h](src/pe/directoryDetails.h)、[directoryDialogs.cpp](src/ui/directoryDialogs.cpp)、[directoryDialogs.h](src/ui/directoryDialogs.h)。

- [x] 根据文件格式按 `IMAGE_THUNK_DATA32/64` 对应的 4 / 8 字节布局读取，统一用 `ULONGLONG` 保存原值；`ReadValue<Width>()` 用 `static_assert` 限制宽度为 2、4、8。UI 按 8 / 16 位十六进制显示数值，列表列宽自动调整。
- [x] `ReadImportDetails()` 共用导入描述符读取；`ReadImportThunks()` 与 `ReadIatDetails()` 共用 `ReadLookupSlot()` / `DecodeImport()`。`ImportSymbol` 保留导入种类、名称、16 位序号和 Hint，`Ordinal #...` 等显示字符串由 UI 生成。
- [x] 导入解析仅依赖 Import Directory 和描述符中的查找表地址，不要求 IAT Directory 存在。导入页保留 Hint 列；缺少 INT 时只对未绑定条目允许 FirstThunk 回退，绑定 IAT 不作为名称 RVA 解析。IAT Directory 缺失与 IAT 字节不存在分别处理。
- [x] 按文件格式使用 32 / 64 位序号标志，校验保留位并保留低 16 位序号；名称导入验证 31 位 RVA，避免将绑定后的地址截断成名称 RVA。
- [x] 目录页 IAT 的 `...` 按钮打开独立详情：按 4 / 8 字节列出所有槽位（含终止项 / 填充）的 RVA、文件偏移、磁盘原始值、DLL、函数名 / 序号，以及 INT 地址和值。绑定文件通过 INT 关联名称；缺少 INT 时仅对未绑定 IAT 回退，绑定地址不当作名称 RVA。
- [x] Base Reloc Table 的 `...` 按钮打开重定位块 / 条目页面：显示 Page RVA、SizeOfBlock、类型、页内偏移、目标 RVA / 文件偏移和原值。支持 HIGHLOW / DIR64、ABSOLUTE 填充与 HIGHADJ 双字条目；未知类型保留数值，不猜测原值宽度。
- [x] 新解析器检查目录边界、槽位 / 块截断、块长度与对齐、名称和 HIGHADJ 配对；页面显示目录错误及条目状态，无文件字节的重定位目标保留条目信息。每次最多显示 100000 个槽位 / 条目，达到上限会提示。
- [x] 导入、导出、绑定导入和调试页面增加状态文本，显示缺失目录、非法范围、截断记录、无终止项、序号越界等解析说明；重新显示时清空旧列表，导出头部失败时清空旧字段。
- [x] `ReadExportDetails()` 一次返回目录头和导出条目，检查函数 / 名称 / 序号数组、字符串及 `Base + index` 溢出，保留无名导出。数组仍是 32 位 RVA、16 位名称序号索引。
- [x] `ReadExportDetails()` 遍历 EAT 时，用 Export DataDirectory 的 `[VirtualAddress, VirtualAddress + Size)` 区分普通导出与转发导出，校验 32 位 RVA 范围溢出；PE32 / PE32+ 的 EAT 项均保持 4 字节。`ExportEntry` 保留原始 RVA / 文件偏移，增加 `isForwarder`、原始转发目标和条目错误信息。转发字符串必须非空、在目录及文件支持的 RVA 范围内以 NULL 结束；异常条目保留并显示错误，不影响后续条目。
- [x] 导出页面保留序号、RVA、文件偏移和名称，增加 Type、Forwarder target、Status 列，区分 `Export` / `Forwarder`，显示 `DLL.name` 或 `DLL.#ordinal` 原始目标。扩大资源模板中的窗口、列表和状态区域，列宽随内容调整；不加载目标 DLL 或递归解析转发链。
- [x] `ReadBoundImportDetails()` 返回主描述符及 forwarder refs 子记录，检查相对目录起点的名称偏移和子记录边界；页面保持主记录列表。`ReadDebugDetails()` 返回原始调试目录记录，检查目录范围及非完整记录长度，类型名称由 UI 格式化。
- [x] Resource 的详情按钮打开资源树，按类型、名称 / ID、语言展示，支持数字 ID、带长度的 UTF-16 名称及多语言叶子。`ReadResourceDetails()` 在 `resourceDetails.cpp` 中独立解析，目录 / 名称偏移相对于 Resource DataDirectory 起点；`IMAGE_RESOURCE_DATA_ENTRY.OffsetToData` 单独按 RVA 转换，可指向资源目录元数据范围之外的文件数据。
- [x] 资源页选中根 / 子目录时显示目录字段，选中叶子时显示数据 RVA、文件偏移、大小、CodePage 和状态。树项用数值索引关联 `ResourceNode`，窗口拥有 `ResourceDetails`；重载和关闭释放旧模型，异常重载清空旧树和详情。布局在 `resource.rc` 中定义，数值格式化及标准资源类型名称在 `ui/resourceDialog.cpp` 中处理。
- [x] 资源解析检查目录范围和 RVA 溢出、完整表头 / 条目 / 数据描述符、名称长度 / 对齐、文件支持的完整数据范围及循环引用；保留异常节点和有效兄弟分支。深度最多 32 层、节点最多 100000 个、名称累计最多 1048576 个 UTF-16 代码单元，达到上限提供状态提示；空目录与零长度资源可读取。
- [x] TLS 详情按钮打开独立字段 / 回调页面；解析入口 `ReadTlsDetails()` 与 `CopyTlsHeader`、`ResolveTlsAddress`、`ReadTlsTemplate`、`ReadTlsCallbacks` 普通辅助函数统一位于 `directoryDetails.cpp`，使用局部变量和显式参数传递状态，不维护解析器对象。按 `Magic` 选择 `IMAGE_TLS_DIRECTORY32/64`，保留四个地址字段、零填充大小与 Characteristics。回调列表按 4 / 8 字节读取，显示槽位 RVA / 文件偏移、回调 VA、目标 RVA / 文件偏移和状态。
- [x] TLS 内部地址以文件的首选 `ImageBase` 从 VA 换算到 RVA，检查减法、地址位宽、32 位 RVA 和 `SizeOfImage` 范围；初始化模板的结束地址按排他边界处理，验证实际数据范围。TLS 索引按 DWORD 范围检查，允许索引位于加载后零填充区域。
- [x] 回调数组独立检查文件支持的 RVA 范围，不受 TLS 目录头的 Size 限制；零地址或首项 NULL 表示无回调，缺少终止项 / 截断数组提供状态提示。保留无效目标及后续有效项，每次最多显示 100000 个回调。页面重载清空旧字段与列表，仅查看磁盘数据，不执行回调。
- [ ] 资源内容继续增加图标、版本信息等预览及原始数据导出。
- [ ] 调试数据载荷（例如 CodeView / PDB）及绑定导入子记录的独立详情展示继续扩展；节表的显示适配与错误状态仍需后续整理。
- [ ] 接入其他目录时分别核对规范：Security / Certificate 目录地址是文件偏移；Load Config 等结构仍需区分 32 / 64 位布局，TLS 的双格式已单独实现。

阶段验收：两种查看器均能显示两种文件格式的节表、导入 DLL / 函数、导出函数、绑定导入、调试目录和资源树，并正确处理不存在的目录。

### 其他数据目录的实现顺序

当前已有详情的 8 个目录是：导入、导出、IAT、重定位、绑定导入、调试、资源和 TLS。资源树与 TLS 字段 / 回调的第一版查看已完成，接下来优先补齐其余尚未实现的目录，下一项是 Delay Import 延迟导入。目录入口与资源结构说明见 [PE 数据目录文档](docs/pe-directories.md)。

| 顺序 | 目录 | 当前状态 | 第一版实现范围 |
| --- | --- | --- | --- |
| 1 | Resource 资源目录 | 已完成 | “类型 → 名称 / ID → 语言”树；显示叶子数据的 RVA、文件偏移、大小和 CodePage。[规范](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#the-rsrc-section) |
| 2 | TLS 目录 | 已完成 | 区分 PE32 / PE32+ 字段宽度，显示 TLS 字段及回调地址列表，按首选 ImageBase 转换 VA，并检查范围与终止项。[规范](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#the-tls-directory) |
| 3 | Delay Import 延迟导入 | 下一项 | DLL、函数名称 / 序号及查找表；复用已有 thunk 解码，按描述符属性区分 RVA / VA。 |
| 4 | Exception 异常目录 | 待实现 | 先支持 x64 的函数起止 RVA、展开信息 RVA；根据 Machine 选择结构，不把所有 PE32+ 都当作 x64。 |
| 5 | Load Config 加载配置 | 待实现 | 根据结构的 Size 和 PE32 / PE32+ 格式读取字段，兼容较短版本，不越过有效长度。 |
| 6 | Security / Certificate 证书目录 | 待实现 | 显示证书条目、长度、版本和类型；按 8 字节对齐遍历，目录地址使用文件偏移。[规范](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#the-attribute-certificate-table-image-only) |
| 7 | CLR Runtime Header | 待实现 | 先显示 CLR 头部及元数据入口，检查头部长度和元数据范围。 |

每个新增目录继续采用“解析结果模型 → UI 显示”的接口，并补充 PE32 / PE32+、缺失目录与损坏输入的适用用例。Architecture 和 Reserved 属于保留入口，Global Ptr 属于特殊入口，不按普通目录表逐项扩展。资源内容预览、调试载荷和绑定导入子记录详情仍保留为已有页面的后续增强。

### 4. 建立可重复的验收用例

- [x] PE32 / PE32+ 页面样本、上下文回归与预期值位于 `tests`；PE32+ 样本包含独立的 8 字节查找表 / IAT，以及名称、序号、名称三个导入项。生成产物放入 `obj` / `bin`。
- [x] 四种“查看器位数 × 文件格式”组合都验证头部、首选基址、节表、目录定位、名称 / 序号导入、无名 / 转发导出和字段宽度。
- [x] x64 测试将两种格式的文件缓冲区放在 4 GB 以上；x86 测试加载首选基址和栈 / 堆字段高于 4 GB 的 PE32+ 磁盘文件，分别检测指针与文件字段截断。
- [ ] 增加缺失目录、异常长度、截断 thunk、无终止符字符串等失败用例，确认错误路径不崩溃、不遗留悬空指针。
- [ ] 以 `dumpbin /headers /imports /exports` 等工具交叉核对真实 EXE / DLL，并验证 Debug / Release 两种构建。

| 查看器 | PE32 文件 | PE32+ 文件 |
| --- | --- | --- |
| x86 | 头部、节表、目录、显示与错误路径 | 同左，并验证完整的 64 位文件字段。 |
| x64 | 同左，并验证 4 GB 以上的缓冲区地址 | 同左，并验证 64 位 thunk。 |

## 独立待办：进程与已加载模块

离线读取 PE32+ 文件不依赖 x64 辅助程序。下面的工作属于主窗口的进程查询功能，可以与文件格式解析分开推进。

- [ ] 在 [handleProcessAndModule.cpp](src/process/handleProcessAndModule.cpp) 的 x64 分支补模块信息查询。目前它只查进程路径，Image Base 和 Size of Image 直接填写 `<Unknown>`；加载基址格式化已经修正，查询尚未补齐。
- [ ] 在 `OpenProcess`、`EnumProcessModules/Ex`、`GetModuleInformation` 和模块路径查询失败后立即记录错误码；区分拒绝访问、进程退出、架构限制和其他错误。仅查询名称时可采用较低的查询权限。
- [ ] 检查模块数组容量与 `cbNeeded`，避免模块较多时越界；统一关闭进程句柄并处理所有失败路径。
- [ ] 在 [utility.cpp](src/common/utility.cpp) 修正 `setDebugPrivilege()` 的返回值和失败路径资源清理。目前 `GetLastError()` 非零后仍会返回 `TRUE`。权限调整成功也不保证可以访问 System 或受保护进程。
- [ ] x64 查看器可用 `EnumProcessModulesEx` 查询普通 x86 / x64 进程；x86 查看器调用这套 API 查询 x64 进程会失败，通常返回 299。若保留 x86 主程序并要求该能力，可由独立 x64 EXE 查询，通过管道等返回模块名称、路径及固定 64 位的基址。辅助程序同样受进程权限限制。

## 构建与当前验证结果

Visual Studio 解决方案提供 `Debug/Release × x86/x64`，其中项目的 x86 平台名是 `Win32`。需要 Visual Studio C++ 工具集和项目指定的 Windows SDK。

可直接在 Visual Studio 中打开 `PEViewer.sln`，选择配置和平台编译主程序。命令行入口要求 PowerShell 7，可在仓库根目录执行：

```powershell
pwsh -File .\scripts\build.ps1 -Configuration Debug -Platform x86
pwsh -File .\scripts\build.ps1 -Configuration Debug -Platform x64
pwsh -File .\scripts\build.ps1 -Configuration Release -Platform x86
pwsh -File .\scripts\build.ps1 -Configuration Release -Platform x64

# 一次构建全部配置，并运行独立验证程序
pwsh -File .\scripts\build.ps1 -All -Test
```

脚本默认 `Debug/x86`，接受 `Win32` 作为 x86 别名；输出位于 `bin/<配置>/Win32` 或 `bin/<配置>/x64`，中间文件位于对应的 `obj` 目录。它会初始化对应 VS 的开发环境，合并子进程环境中仅大小写不同的 PATH 项；`-MSBuildPath` 支持显式指定 MSBuild，`-Rebuild` 强制重新构建。脚本支持从其他工作目录调用。

验证源码、样本生成代码和使用说明位于 [tests](tests/README.md)。`-Test` 为独立验证工程传入主程序对应配置的工具集及 SDK，将验证程序写入 `bin/<配置>/<平台>/tests`，将中间文件、生成样本和构建日志写入 `obj/<配置>/<平台>/tests`。主程序的构建日志位于 `obj/<配置>/<平台>/build.log`。正式入口和验证源码不受 [.gitignore](.gitignore) 的生成文件规则影响。

已完成的验证：

- TLS 解析统一编译自 `directoryDetails.cpp`，通过 Debug / Release × Win32 / x64 四组主程序构建及模型 / 页面回归，另覆盖交替解析独立上下文及失败后重读；Debug/x64 默认程序运行时，使用 `bin/Debug/x64/verification` 独立输出完成验证。覆盖双格式字段、VA 转换、空 / 截断数组、终止项、地址溢出、零填充索引和 100000 项上限。Win32 / x64 验证程序交叉读取系统两种格式的 `ActivationClient.dll`（各 2 个回调）和 `aadauthhelper.dll`（各 0 个回调），字段及回调地址与 `dumpbin /TLS` 一致。Win32 通过 `Sysnative` 读取 64 位系统文件，避免 System32 重定向。

- Resource 资源树通过 Debug / Release × Win32 / x64 四组主程序构建和模型 / 页面回归，覆盖 Unicode / 数字名称、多语言、目录相对偏移与数据 RVA、异常范围、循环 / 共享子树和遍历限制；页面检查选择切换、独立上下文重载及旧状态清理。x64 页面使用 4 GB 以上缓冲区。Win32 / x64 验证程序均交叉读取两种格式的 notepad.exe（各 31 条资源 / 69 个节点）和 kernel32.dll（各 2 条资源 / 6 个节点），通过独立副本的 Windows 资源 API 映射核对条目大小和原始字节。
- 转发导出通过 Debug / Release × Win32 / x64 四组主程序构建及模型 / 页面回归，覆盖名称 / 序号目标、无名导出、空 EAT 槽、目录边界、字符串终止及 RVA 溢出。Win32 和 x64 验证程序交叉读取系统两种格式的 kernel32.dll：PE32 为 208 个转发、PE32+ 为 211 个转发，与 `dumpbin /exports` 一致；两种格式的 ntdll.dll 均为 0 个转发。
- 目录分离新增直接调用解析模型的双格式用例：缺少 IAT Directory 的导入 / Hint、共享名称与序号解码、导出数组 / 序号溢出、绑定导入 forwarder refs、调试记录截断和缺失目录。页面回归验证显式上下文、导入行排序后的描述符身份、错误状态及旧列表 / 头部字段清理。
- 本轮目录分离通过 Debug / Release × Win32 / x64 四组主程序构建和验证程序回归。Debug/x64 的默认程序正在运行，使用独立的 `bin/Debug/x64/directory-separation-check` / `obj/Debug/x64/directory-separation-check` 完成构建与验证。Win32 和 x64 验证程序均交叉读取系统两种格式的 `notepad.exe`、`kernel32.dll`，检查已实现目录；kernel32 的 PE32 / PE32+ 导出分别为 1670 / 1698 项。
- IAT / 重定位新增用例覆盖两种格式的槽位与 INT 对照、绑定和未绑定回退、多 DLL、HIGHLOW / DIR64、ABSOLUTE / HIGHADJ、异常目录和块切换。系统记事本交叉核对：PE32 的 IAT 为 361 槽、重定位为 39 块 / 4650 条；PE32+ 为 365 槽、5 块 / 366 条，与 `dumpbin /headers /imports /relocations` 的目录长度、块数和条目数一致。
- 本轮页面 / 导入适配通过 Debug / Release × Win32 / x64 四组构建及回归：两种磁盘格式、两种格式的 12 个页面、64 位输入与实际字体宽度、按钮边界、名称 / 序号导入、未绑定 IAT 回退和异常 thunk。x64 继续覆盖 4 GB 以上的文件缓冲区。
- 主程序和验证工程默认工具集已统一为 Visual Studio 2022 的 v143，SDK 保持 10.0.22621.0；`PEContext` 改造通过 Debug / Release × Win32 / x64 四组构建及回归。构建入口见 [tests/README.md](tests/README.md)。
- x86、x64 的 Debug 和 Release 四组构建通过，`C4311/C4312/C4302` 指针转换相关警告为零；其他既有警告仍需按后续任务处理。
- 四组配置均调用实际文件读取函数，使用合成 PE32 数据创建 12 个隐藏文件查看对话框，检查头部、节表、导入函数、导出、绑定导入和调试目录的列表及预期输出，并验证位图资源可加载。
- x64 验证中的缓冲区位于 4 GB 以上，头部 / 节表指针、RVA 转换、目录列表名称和条目数正确；修改后的源码中文注释、编码与差异空白检查通过。

上述验证覆盖已完成的指针和回调适配。完整 PE32+ 支持以及真实文件、损坏输入的完整矩阵仍按前面的验收计划推进。

## 规范参考

- [Microsoft PE / COFF 格式](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format)：文件布局、字段宽度、导入 / 导出与数据目录。
- [Windows 指针使用规则](https://learn.microsoft.com/en-us/windows/win32/winprog64/rules-for-using-pointers)：本进程地址和消息参数的双平台类型选择。
- [DLGPROC](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-dlgproc)、[MODULEINFO](https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-moduleinfo)：回调签名与实际加载映像信息。
- [EnumProcessModules](https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-enumprocessmodules)、[EnumProcessModulesEx](https://learn.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-enumprocessmodulesex)：进程模块枚举的跨位数限制。
