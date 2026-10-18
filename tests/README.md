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

验证程序使用 `fixtures/pe32.h` 中的确定性 PE32 样本生成代码和预期值，调用实际文件读取函数和 12 个页面的消息回调，检查头部、RVA 转换、节表、名称导入、名称导出、绑定导入、调试目录及位图资源。页面在隐藏窗口中初始化；x64 验证要求文件缓冲区位于 4 GB 以上。

主程序输出到 `bin/<配置>/<平台>/PEViewer.exe`，验证程序输出到其 `tests` 子目录。中间文件、构建日志及生成样本位于 `obj/<配置>/<平台>` 和其 `tests` 子目录，均由 Git 忽略。可直接运行验证程序，传入一个父目录已存在的样本输出路径：

```powershell
.\bin\Debug\x64\tests\portability-smoke.exe .\obj\Debug\x64\tests\synthetic-pe32.exe
```

`pe-context-tests.cpp` 使用独立于 UI 的 `PEContext`，覆盖 PE32 / PE32+ 头部、两种磁盘文件的加载、64 位字段、上下文隔离、所有权、重载与失败清理、目录数量、非法 / 截断头部、RVA 越界、虚拟零填充和无终止符字符串。

页面回归另检查显式上下文传递、PE32 序号导入及 `FirstThunk` 回退、无名导出的 `Base`、未知调试类型和非法目录 RVA。新增 PE32+ 可选头与汇总页面回归，检查高于 4 GB 的 `ImageBase` 与栈 / 堆大小、完整 64 位数值、仍为 32 位的公共字段，以及 PE32 / PE32+ 切换时 `BaseOfData` 的文本和启用状态；x64 中继续使用位于 4 GB 以上的文件缓冲区。

正常文件选择入口已开放 PE32+。两种格式都创建实际的 12 个页面，检查格式标题、位图和已实现的数据目录。`fixtures/pe64.h` 包含真正的 8 字节导入查找表与 IAT，按名称、序号、名称排列，防止错误的 4 字节步长提前结束遍历。额外覆盖两种格式的 `FirstThunk` 回退、完整序号标志、保留位、绑定地址、越界 / 截断条目、缺失终止项及无终止符名称。

头部编辑回归检查六个宽数值控件能完整容纳 16 位十六进制数，按文件格式设置输入长度限制。失去焦点后以 32 / 64 位范围解析，接受可选的 `0x` 前缀和首尾空白，规范化显示；非法字符、符号和溢出输入恢复为文件原值。字段编辑仍是界面预览，不写回文件。按钮回归检查 PE32+ 的 SizeOfImage / SizeOfHeaders 调整、零对齐和加法溢出；导入数值列自动适应完整的 64 位值。

主程序与独立验证工程默认工具集均为 Visual Studio 2022 的 v143，SDK 为 10.0.22621.0。通过 Visual Studio 或上面的构建脚本可直接使用工程默认配置；脚本继续将主程序的工具集和 SDK 同步给验证工程。

IAT 与重定位解析验证位于 `directory-details-tests.cpp`，直接检查独立于 UI 的结果：32 / 64 位槽位和原值、INT 名称 / 序号关联、绑定 IAT、缺少 INT 的回退、多 DLL、终止项、非法 RVA / 名称、截断槽位、重定位块长度 / 对齐、HIGHLOW / DIR64、ABSOLUTE、HIGHADJ 配对和未知类型。页面回归现覆盖两种格式的 12 个页面，包括 IAT 完整 64 位列宽、重定位块切换与错误文本；x64 的两个新页面也使用 4 GB 以上缓冲区。

验证程序可额外接收真实 PE 文件路径，检查 IAT / 重定位解析并打印数量（生成样本路径仍作为第一个参数）：

```powershell
.\bin\Debug\x64\tests\portability-smoke.exe .\obj\real-check.exe C:\Windows\System32\notepad.exe C:\Windows\SysWOW64\notepad.exe
```

结构解释参考 [Microsoft PE 格式规范](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format)，尤其是 Import Address Table 与 Base Relocation Block / Types。显示的 IAT 与重定位原值均来自磁盘文件，不应用重定位或解析进程中的运行时函数地址。
