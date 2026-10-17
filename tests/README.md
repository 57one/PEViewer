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

验证程序使用 `fixtures/pe32.h` 中的确定性 PE32 样本生成代码和预期值，调用实际文件读取函数和 10 个页面的消息回调，检查头部、RVA 转换、节表、名称导入、名称导出、绑定导入、调试目录及位图资源。页面在隐藏窗口中初始化；x64 验证要求文件缓冲区位于 4 GB 以上。

主程序输出到 `bin/<配置>/<平台>/PEViewer.exe`，验证程序输出到其 `tests` 子目录。中间文件、构建日志及生成样本位于 `obj/<配置>/<平台>` 和其 `tests` 子目录，均由 Git 忽略。可直接运行验证程序，传入一个父目录已存在的样本输出路径：

```powershell
.\bin\Debug\x64\tests\portability-smoke.exe .\obj\Debug\x64\tests\synthetic-pe32.exe
```

`pe-context-tests.cpp` 使用独立于 UI 的 `PEContext`，覆盖 PE32 / PE32+ 头部、64 位字段、上下文隔离、所有权、重载与失败清理、目录数量、非法 / 截断头部、RVA 越界、虚拟零填充和无终止符字符串。PE32+ 头部样本由 `fixtures/pe64.h` 生成。

页面回归另检查显式上下文传递、PE32 序号导入及 `FirstThunk` 回退、无名导出的 `Base`、未知调试类型和非法目录 RVA。新增 PE32+ 可选头与汇总页面回归，检查高于 4 GB 的 `ImageBase` 与栈 / 堆大小、完整 64 位数值、仍为 32 位的公共字段，以及 PE32 / PE32+ 切换时 `BaseOfData` 的文本和启用状态；x64 中继续使用位于 4 GB 以上的文件缓冲区。

PE32+ 页面测试直接初始化实际对话框并调用读取函数；正常文件选择入口仍拦截 PE32+，字段调整按钮及目录 thunk 的适配继续按 `architecture.md` 推进。`fixtures/pe64.h` 用于头部测试，目录内容仍继承 PE32 样本，不作为完整 PE32+ 目录样本。

主程序与独立验证工程默认工具集均为 Visual Studio 2022 的 v143，SDK 为 10.0.22621.0。通过 Visual Studio 或上面的构建脚本可直接使用工程默认配置；脚本继续将主程序的工具集和 SDK 同步给验证工程。
