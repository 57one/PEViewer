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

	EnumModule(hListModule);
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

	HMODULE hMod;
	DWORD cbNeeded;

	for (UINT32 i = 0; i < cProcesses; i++) {
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
			PROCESS_VM_READ,
			FALSE, iProcesses[i]);
		if (hProcess == NULL) continue;
		/*if (GetProcessImageFileName(hProcess, szProcessName, sizeof(szProcessName) / sizeof(*szProcessName)) == 0) {
			continue;
		}
		else {
			break;
		}*/
		if (EnumProcessModules(hProcess, &hMod, sizeof(hMod),
			&cbNeeded))
		{
			GetModuleBaseName(hProcess, hMod, szProcessName,
				sizeof(szProcessName) / sizeof(TCHAR));
			break;
		}

	}
	

	LV_ITEM vitem;

	//初始化						
	memset(&vitem, 0, sizeof(LV_ITEM));
	vitem.mask = LVIF_TEXT;

	//vitem.pszText = const_cast<wchar_t*>(TEXT("csrss.exe"));
	vitem.pszText = szProcessName;
	vitem.iItem = 0;		//第几行				
	vitem.iSubItem = 0;		//第几列
	// 这是一个宏 和 sendMessage做的事情一样
	//ListView_InsertItem(hListProcess, &vitem);
	//凡事皆消息
	SendMessage(hListProcess, LVM_INSERTITEM, 0, (DWORD)&vitem);

	//第一行写完之后 后面全是 SETITEM
	vitem.pszText = const_cast<wchar_t*>(TEXT("448"));
	vitem.iItem = 0;
	vitem.iSubItem = 1;
	ListView_SetItem(hListProcess, &vitem);

	vitem.pszText = const_cast<wchar_t*>(TEXT("56590000"));
	vitem.iItem = 0;
	vitem.iSubItem = 2;
	ListView_SetItem(hListProcess, &vitem);

	vitem.pszText = const_cast<wchar_t*>(TEXT("000F0000"));
	vitem.iItem = 0;
	vitem.iSubItem = 3;
	ListView_SetItem(hListProcess, &vitem);

	vitem.pszText = const_cast<wchar_t*>(TEXT("winlogon.exe"));
	vitem.iItem = 1;
	vitem.iSubItem = 0;
	//ListView_InsertItem(hListProcess, &vitem);						
	SendMessage(hListProcess, LVM_INSERTITEM, 0, (DWORD)&vitem);

	vitem.pszText = const_cast<wchar_t*>(TEXT("456"));
	vitem.iSubItem = 1;
	ListView_SetItem(hListProcess, &vitem);

	vitem.pszText = const_cast<wchar_t*>(TEXT("10000000"));
	vitem.iSubItem = 2;
	ListView_SetItem(hListProcess, &vitem);

	vitem.pszText = const_cast<wchar_t*>(TEXT("000045800"));
	vitem.iSubItem = 3;
	ListView_SetItem(hListProcess, &vitem);

}

VOID EnumModule(HWND hListModule) {
	LV_ITEM vitem;

	//初始化
	memset(&vitem, 0, sizeof(LV_ITEM));
	vitem.mask = LVIF_TEXT;

	vitem.pszText = const_cast <wchar_t*>(TEXT("kernel.dll"));
	vitem.iItem = 0;		//第几行				
	vitem.iSubItem = 0;		//第几列
	SendMessage(hListModule, LVM_INSERTITEM, 0, (DWORD)&vitem);

	//第一行写完之后 后面全是 SETITEM
	vitem.pszText = const_cast<wchar_t*>(TEXT("c:\\windows\\system32\\kernel.dll"));
	vitem.iItem = 0;
	vitem.iSubItem = 1;
	ListView_SetItem(hListModule, &vitem);


	vitem.pszText = const_cast<wchar_t*>(TEXT("user.dll"));
	vitem.iItem = 1;
	vitem.iSubItem = 0;
	//ListView_InsertItem(hListProcess, &vitem);						
	SendMessage(hListModule, LVM_INSERTITEM, 0, (DWORD)&vitem);

	vitem.pszText = const_cast<wchar_t*>(TEXT("c:\\windows\\system32\\user.dll"));
	vitem.iSubItem = 1;
	ListView_SetItem(hListModule, &vitem);

}