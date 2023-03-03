#include"handleProcessAndModule.h"
#include<string>

VOID InitProcessListView(HWND hDlg) {
	LV_COLUMN lv;
	HWND hListProcess;

	//初始化								
	memset(&lv, 0, sizeof(LV_COLUMN));
	//获取IDC_LIST_PROCESS句柄 (List的句柄)								
	hListProcess = GetDlgItem(hDlg, IDC_LIST_PROCESS);
	//设置整行选中
	//点击某行的时候 显示整行选中
	SendMessage(hListProcess, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	//第一列								
	lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lv.pszText = const_cast<wchar_t*>(TEXT("进程"));				//列标题				
	lv.cx = 100;								//列宽
	lv.iSubItem = 0;						//第几列		
	//ListView_InsertColumn(hListProcess, 0, &lv);								
	SendMessage(hListProcess, LVM_INSERTCOLUMN, 0, (DWORD)&lv);
	//第二列								
	lv.pszText = const_cast<wchar_t*>(TEXT("PID"));
	lv.cx = 60;
	lv.iSubItem = 1;
	//ListView_InsertColumn(hListProcess, 1, &lv);								
	SendMessage(hListProcess, LVM_INSERTCOLUMN, 1, (DWORD)&lv);
	//第三列								
	lv.pszText = const_cast<wchar_t*>(TEXT("镜像基址"));
	lv.cx = 85;
	lv.iSubItem = 2;
	ListView_InsertColumn(hListProcess, 2, &lv);
	//第四列								
	lv.pszText = const_cast<wchar_t*>(TEXT("镜像大小"));
	lv.cx = 90;
	lv.iSubItem = 3;
	ListView_InsertColumn(hListProcess, 3, &lv);

	EnumProcess(hListProcess);
}

VOID InitModulesListView(HWND hDlg) {
	LV_COLUMN lv;
	HWND hListModule;

	//初始化
	memset(&lv, 0, sizeof(LV_COLUMN));
	//获取IDC_LIST_MODULE的句柄 (module的句柄)
	hListModule = GetDlgItem(hDlg, IDC_LIST_MODULE);
	//设置整行选中
	SendMessage(hListModule, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	//第一列
	lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lv.pszText = const_cast<wchar_t*>(TEXT("模块名称"));				//列标题				
	lv.cx = 100;								//列宽
	lv.iSubItem = 0;						//第几列
	SendMessage(hListModule, LVM_INSERTCOLUMN, 0, (DWORD)&lv);

	//第二列
	lv.pszText = const_cast<wchar_t*>(TEXT("模块位置"));
	lv.cx = 100;
	lv.iSubItem = 1;
	ListView_InsertColumn(hListModule, 1, &lv);

	//EnumModule(hListModule);
}

VOID DrawProcess(HWND hListProcess, int Row, TCHAR* szProcessName, TCHAR* szProcessID) {
	LV_ITEM vitem;

	//初始化						
	memset(&vitem, 0, sizeof(LV_ITEM));
	vitem.mask = LVIF_TEXT;

	vitem.pszText = szProcessName;
	vitem.iItem = Row;		//第几行				
	vitem.iSubItem = 0;		//第几列
	// 这是一个宏 和 sendMessage做的事情一样
	//ListView_InsertItem(hListProcess, &vitem);
	//凡事皆消息
	SendMessage(hListProcess, LVM_INSERTITEM, 0, (DWORD)&vitem);

	//第一行写完之后 后面全是 SETITEM
	vitem.pszText = szProcessID;
	vitem.iSubItem = 1;
	ListView_SetItem(hListProcess, &vitem);

	vitem.pszText = const_cast<wchar_t*>(TEXT("56590000"));
	vitem.iSubItem = 2;
	ListView_SetItem(hListProcess, &vitem);

	vitem.pszText = const_cast<wchar_t*>(TEXT("000F0000"));
	vitem.iSubItem = 3;
	ListView_SetItem(hListProcess, &vitem);
}

VOID DrawModules(HWND hListModule, int Row, TCHAR* szModName, TCHAR* szModFullName) {
	LV_ITEM vitem;

	//初始化
	memset(&vitem, 0, sizeof(LV_ITEM));
	vitem.mask = LVIF_TEXT;

	vitem.pszText = szModName;
	vitem.iItem = Row;		//第几行				
	vitem.iSubItem = 0;		//第几列
	SendMessage(hListModule, LVM_INSERTITEM, 0, (DWORD)&vitem);

	//第一行写完之后 后面全是 SETITEM
	vitem.pszText = szModFullName;
	vitem.iSubItem = 1;
	ListView_SetItem(hListModule, &vitem);
}

VOID EnumProcess(HWND hListProcess) {
	DWORD iProcesses[1024], pcbNeeded, cProcesses;
	if (!EnumProcesses(iProcesses, sizeof(iProcesses), &pcbNeeded)) {
		return;
	}

	//number of Process;
	cProcesses = pcbNeeded / sizeof(DWORD);

	HANDLE hProcess;
	TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
	TCHAR szBuf[6] = { 0 };
	UINT32 count = 0;
	for (UINT32 i = 0; i < cProcesses; i++) {
		if (iProcesses[i] != 0) {
			hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
				PROCESS_VM_READ,
				FALSE, iProcesses[i]);
			if (hProcess == NULL) continue;
			HMODULE hMod;
			DWORD cbNeeded;
			MODULEINFO module_info;
			memset(&module_info, 0, sizeof(module_info));
			if (EnumProcessModules(hProcess, &hMod, sizeof(hMod),
				&cbNeeded))
			{
				GetModuleInformation(hProcess, hMod, &module_info, sizeof(module_info));
				//MessageBox(NULL, TEXT("请选择进程"), TEXT("出错了"), MB_OK);
				GetModuleBaseName(hProcess, hMod, szProcessName,
					sizeof(szProcessName) / sizeof(TCHAR));
				wsprintf(szBuf, TEXT("%ld"), iProcesses[i]);
				DrawProcess(hListProcess, count, szProcessName, szBuf);
				count++;
			}
		}
	}
}

VOID EnumModules(HWND hListProcess, HWND hListModule, WPARAM wParam, LPARAM lParam) {
	DWORD dwRowId;
	TCHAR szPid[0x20];
	LV_ITEM lv;

	//初始化
	memset(&lv, 0, sizeof(LV_ITEM));
	memset(szPid, 0, 0x20);
	//获取选择行
	dwRowId = SendMessage(hListProcess, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
	if (dwRowId == -1) {
		MessageBox(NULL, TEXT("请选择进程"), TEXT("出错了"), MB_OK);
		return;
	}
	//获取PID
	lv.iSubItem = 1;	//要获取的列
	lv.pszText = szPid;	//指定存储查询结果的缓冲区
	lv.cchTextMax = 0x20;	//指定缓冲区大小
	SendMessage(hListProcess, LVM_GETITEMTEXT, dwRowId, (DWORD)&lv);

	DWORD processID;
	HMODULE hMods[1024];
	HANDLE hProcess;
	DWORD cbNeeded;

	processID = wcstoul(szPid, NULL, 10);

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
		PROCESS_VM_READ,
		FALSE, processID);
 
	UINT32 count = 0;

	if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
	{
		for (UINT32 i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
		{
			TCHAR szModName[MAX_PATH], szModFullName[MAX_PATH];

			// Get the full path to the module's file.

			if (GetModuleBaseName(hProcess, hMods[i], szModName,
				sizeof(szModName) / sizeof(TCHAR))) {
			}

			if (GetModuleFileNameEx(hProcess, hMods[i], szModFullName,
				sizeof(szModName) / sizeof(TCHAR))) {
			}

			DrawModules(hListModule, count, szModName, szModFullName);
			count++;
		}
	}

}