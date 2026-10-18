# PEViewer 验证入口

在仓库根目录运行（PowerShell 7、Visual Studio C++ 工具集及工程指定的 Windows SDK）：

```powershell
# 当前配置的主程序和独立验证程序
pwsh -File .\scripts\build.ps1 -Configuration Debug -Platform x64 -Test

# Debug/Release × Win32/x64 全部构建并验证
pwsh -File .\scripts\build.ps1 -All -Test
```

脚本从自身目录定位工程，支持从其他工作目录调用。`x86` 是 `Win32` 的别名；`-Rebuild` 强制重新构建，`-MSBuildPath` 可指定 MSBuild。直接用 VS 打开根目录 `PEViewer.sln` 仍可编译主程序。

`portability-smoke.vcxproj` 是独立控制台工程，不加入主程序的编译列表。脚本为它传入主程序当前配置的工具集和 SDK；也可以在 VS 中单独打开此工程（默认 v143 / SDK 10.0.22621.0）。

验证程序使用 `fixtures/pe32.h` 中的确定性 PE32 样本生成代码和预期值，调用实际文件读取函数和 13 个页面的消息回调，检查头部、RVA 转换、节表、名称导入、名称导出、绑定导入、调试目录及位图资源。页面在隐藏窗口中初始化；x64 验证要求文件缓冲区位于 4 GB 以上。

主程序输出到 `bin/<配置>/<平台>/PEViewer.exe`，验证程序输出到其 `tests` 子目录。中间文件、构建日志及生成样本位于 `obj/<配置>/<平台>` 和其 `tests` 子目录，均由 Git 忽略。可直接运行验证程序，传入一个父目录已存在的样本输出路径：

```powershell
.\bin\Debug\x64\tests\portability-smoke.exe .\obj\Debug\x64\tests\synthetic-pe32.exe
```

`pe-context-tests.cpp` 使用独立于 UI 的 `PEContext`，覆盖 PE32 / PE32+ 头部、两种磁盘文件的加载、64 位字段、上下文隔离、所有权、重载与失败清理、目录数量、非法 / 截断头部、RVA 越界、虚拟零填充和无终止符字符串。

页面回归另检查显式上下文传递、PE32 序号导入及 `FirstThunk` 回退、无名导出的 `Base`、未知调试类型和非法目录 RVA。新增 PE32+ 可选头与汇总页面回归，检查高于 4 GB 的 `ImageBase` 与栈 / 堆大小、完整 64 位数值、仍为 32 位的公共字段，以及 PE32 / PE32+ 切换时 `BaseOfData` 的文本和启用状态；x64 中继续使用位于 4 GB 以上的文件缓冲区。

正常文件选择入口已开放 PE32+。两种格式都创建实际的 13 个页面，检查格式标题、位图和已实现的数据目录。`fixtures/pe64.h` 包含真正的 8 字节导入查找表与 IAT，按名称、序号、名称排列，防止错误的 4 字节步长提前结束遍历。额外覆盖两种格式的 `FirstThunk` 回退、完整序号标志、保留位、绑定地址、越界 / 截断条目、缺失终止项及无终止符名称。

头部编辑回归检查六个宽数值控件能完整容纳 16 位十六进制数，按文件格式设置输入长度限制。失去焦点后以 32 / 64 位范围解析，接受可选的 `0x` 前缀和首尾空白，规范化显示；非法字符、符号和溢出输入恢复为文件原值。字段编辑仍是界面预览，不写回文件。按钮回归检查 PE32+ 的 SizeOfImage / SizeOfHeaders 调整、零对齐和加法溢出；导入数值列自动适应完整的 64 位值。

主程序与独立验证工程默认工具集均为 Visual Studio 2022 的 v143，SDK 为 10.0.22621.0。通过 Visual Studio 或上面的构建脚本可直接使用工程默认配置；脚本继续将主程序的工具集和 SDK 同步给验证工程。

IAT 与重定位解析验证位于 `directory-details-tests.cpp`，直接检查独立于 UI 的结果：32 / 64 位槽位和原值、INT 名称 / 序号关联、绑定 IAT、缺少 INT 的回退、多 DLL、终止项、非法 RVA / 名称、截断槽位、重定位块长度 / 对齐、HIGHLOW / DIR64、ABSOLUTE、HIGHADJ 配对和未知类型。页面回归现覆盖两种格式的 13 个页面，包括 IAT 完整 64 位列宽、重定位块切换与错误文本；x64 的两个新页面也使用 4 GB 以上缓冲区。

验证程序可额外接收真实 PE 文件路径，检查导入、IAT、导出、绑定导入、调试、资源及重定位解析并打印数量（生成样本路径仍作为第一个参数）：

```powershell
.\bin\Debug\x64\tests\portability-smoke.exe .\obj\real-check.exe C:\Windows\System32\notepad.exe C:\Windows\SysWOW64\notepad.exe
```

结构解释参考 [Microsoft PE 格式规范](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format)，尤其是 Import Address Table 与 Base Relocation Block / Types。显示的 IAT 与重定位原值均来自磁盘文件，不应用重定位或解析进程中的运行时函数地址。

已实现目录的解析 / 显示现统一分离：`src/pe/directoryDetails.*` 返回结构化模型，`src/ui/directoryDialogs.*` 负责显示。`directory-details-tests.cpp` 直接验证导入、导出、绑定导入和调试结果，不创建窗口；名称 / 序号解码在导入页与 IAT 页共用，返回原始 Hint。验证覆盖 IAT Directory 缺失时仍能读取 INT，以及从 FirstThunk 回退后仍保留 Hint。

新增模型用例覆盖导出数组边界、无名导出与序号溢出、绑定导入子记录 / 名称边界、调试记录截断和目录缺失。页面用例覆盖导入列表排序后仍按数值描述符 ID 选择对象、改动显示文字不影响读取、非法目录清空旧数据以及可见错误状态。调试数据载荷解析留待独立功能迭代。

转发导出用例覆盖 PE32 / PE32+ 的普通导出、`MYDLL.expfunc` / `MYDLL.#27` 转发、无名导出、空 EAT 槽、导出目录范围的两端、结尾 NULL 位于目录最后一字节、结尾 NULL 越过目录、空字符串、文件边界 / 无映射目标和 RVA 范围溢出。页面检查原始 RVA / 文件偏移、类型、目标、状态、目标列宽，以及无名条目和异常条目的保留；x64 页面继续使用 4 GB 以上的缓冲区。真实文件检查同时打印转发条目数，便于与 `dumpbin /exports` 对照。

资源解析用例位于 `resource-details-tests.cpp`，独立于窗口；`fixtures/resources.h` 为两种格式建立相同的类型 / 名称 / 语言树，含 Unicode 名称、数字 ID 和三个资源数据条目。覆盖目录起点相对偏移、数据 RVA、完整数据范围、多语言、带长度名称中的空字符、截断名称 / 表头 / 条目 / 数据描述符、非法地址、循环引用、共享子树、深度 / 节点 / 名称总量限制、空目录和零长度资源。

当前页面回归扩展为两种格式的 13 个页面，资源页检查树标签、数字模型索引、语言切换后的字段、显式上下文重载、旧树 / 详情清理和错误状态；x64 资源页同样使用 4 GB 以上的缓冲区。真实资源数据先复制到 `obj` 中的独立文件，避免系统 MUI 语言包或已加载同名模块干扰，再通过 `LoadLibraryEx` 的数据 / 资源映射与 `FindResourceEx`、`SizeofResource`、`LoadResource` / `LockResource` 交叉比较条目大小和原始字节；该映射不执行目标程序代码。
