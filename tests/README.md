# PEViewer 验证入口

在仓库根目录运行（PowerShell 7、Visual Studio C++ 工具集及工程指定的 Windows SDK）：

```powershell
# 当前配置的主程序和独立验证程序
pwsh -File .\scripts\build.ps1 -Configuration Debug -Platform x64 -Test

# Debug/Release × Win32/x64 全部构建并验证
pwsh -File .\scripts\build.ps1 -All -Test
```

脚本从自身目录定位工程，支持从其他工作目录调用。`x86` 是 `Win32` 的别名；`-Rebuild` 强制重新构建，`-MSBuildPath` 可指定 MSBuild。直接用 VS 打开根目录 `PEViewer.sln` 仍可编译主程序。

`portability-smoke.vcxproj` 是独立控制台工程，不加入主程序的编译列表。脚本为它传入主程序当前配置的工具集和 SDK；也可以在 VS 中单独打开此工程（默认 v145 / SDK 10.0.22621.0）。

验证程序使用 `fixtures/pe32.h` 中的确定性 PE32 样本生成代码和预期值，调用实际文件读取函数和 10 个页面的消息回调，检查头部、RVA 转换、节表、名称导入、名称导出、绑定导入、调试目录及位图资源。页面在隐藏窗口中初始化；x64 验证要求文件缓冲区位于 4 GB 以上。

主程序输出到 `bin/<配置>/<平台>/PEViewer.exe`，验证程序输出到其 `tests` 子目录。中间文件、构建日志及生成样本位于 `obj/<配置>/<平台>` 和其 `tests` 子目录，均由 Git 忽略。可直接运行验证程序，传入一个父目录已存在的样本输出路径：

```powershell
.\bin\Debug\x64\tests\portability-smoke.exe .\obj\Debug\x64\tests\synthetic-pe32.exe
```

此入口用于迁移和已有指针适配的回归验证。PE32+、损坏输入、序号导入、无名及转发导出仍属于 `architecture.md` 的后续计划。
