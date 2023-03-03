#include"handleProcessAndModule.h"
#include<string>

VOID InitProcessListView(HWND hDlg) {
	LV_COLUMN lv;
	HWND hListProcess;

	//��ʼ��								
	memset(&lv, 0, sizeof(LV_COLUMN));
	//��ȡIDC_LIST_PROCESS��� (List�ľ��)								
	hListProcess = GetDlgItem(hDlg, IDC_LIST_PROCESS);
	//��������ѡ��
	//���ĳ�е�ʱ�� ��ʾ����ѡ��
	SendMessage(hListProcess, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	//��һ��								
	lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lv.pszText = const_cast<wchar_t*>(TEXT("����"));				//�б���				
	lv.cx = 100;								//�п�
	lv.iSubItem = 0;						//�ڼ���		
	//ListView_InsertColumn(hListProcess, 0, &lv);								
	SendMessage(hListProcess, LVM_INSERTCOLUMN, 0, (DWORD)&lv);
	//�ڶ���								
	lv.pszText = const_cast<wchar_t*>(TEXT("PID"));
	lv.cx = 60;
	lv.iSubItem = 1;
	//ListView_InsertColumn(hListProcess, 1, &lv);								
	SendMessage(hListProcess, LVM_INSERTCOLUMN, 1, (DWORD)&lv);
	//������								
	lv.pszText = const_cast<wchar_t*>(TEXT("�����ַ"));
	lv.cx = 85;
	lv.iSubItem = 2;
	ListView_InsertColumn(hListProcess, 2, &lv);
	//������								
	lv.pszText = const_cast<wchar_t*>(TEXT("�����С"));
	lv.cx = 90;
	lv.iSubItem = 3;
	ListView_InsertColumn(hListProcess, 3, &lv);

	EnumProcess(hListProcess);
}

VOID InitModulesListView(HWND hDlg) {
	LV_COLUMN lv;
	HWND hListModule;

	//��ʼ��
	memset(&lv, 0, sizeof(LV_COLUMN));
	//��ȡIDC_LIST_MODULE�ľ�� (module�ľ��)
	hListModule = GetDlgItem(hDlg, IDC_LIST_MODULE);
	//��������ѡ��
	SendMessage(hListModule, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	//��һ��
	lv.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lv.pszText = const_cast<wchar_t*>(TEXT("ģ������"));				//�б���				
	lv.cx = 100;								//�п�
	lv.iSubItem = 0;						//�ڼ���
	SendMessage(hListModule, LVM_INSERTCOLUMN, 0, (DWORD)&lv);

	//�ڶ���
	lv.pszText = const_cast<wchar_t*>(TEXT("ģ��λ��"));
	lv.cx = 100;
	lv.iSubItem = 1;
	ListView_InsertColumn(hListModule, 1, &lv);

	//EnumModule(hListModule);
}

VOID DrawProcess(HWND hListProcess, int Row, TCHAR* szProcessName, TCHAR* szProcessID) {
	LV_ITEM vitem;

	//��ʼ��						
	memset(&vitem, 0, sizeof(LV_ITEM));
	vitem.mask = LVIF_TEXT;

	vitem.pszText = szProcessName;
	vitem.iItem = Row;		//�ڼ���				
	vitem.iSubItem = 0;		//�ڼ���
	// ����һ���� �� sendMessage��������һ��
	//ListView_InsertItem(hListProcess, &vitem);
	//���½���Ϣ
	SendMessage(hListProcess, LVM_INSERTITEM, 0, (DWORD)&vitem);

	//��һ��д��֮�� ����ȫ�� SETITEM
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

	//��ʼ��
	memset(&vitem, 0, sizeof(LV_ITEM));
	vitem.mask = LVIF_TEXT;

	vitem.pszText = szModName;
	vitem.iItem = Row;		//�ڼ���				
	vitem.iSubItem = 0;		//�ڼ���
	SendMessage(hListModule, LVM_INSERTITEM, 0, (DWORD)&vitem);

	//��һ��д��֮�� ����ȫ�� SETITEM
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
				//MessageBox(NULL, TEXT("��ѡ�����"), TEXT("������"), MB_OK);
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

	//��ʼ��
	memset(&lv, 0, sizeof(LV_ITEM));
	memset(szPid, 0, 0x20);
	//��ȡѡ����
	dwRowId = SendMessage(hListProcess, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
	if (dwRowId == -1) {
		MessageBox(NULL, TEXT("��ѡ�����"), TEXT("������"), MB_OK);
		return;
	}
	//��ȡPID
	lv.iSubItem = 1;	//Ҫ��ȡ����
	lv.pszText = szPid;	//ָ���洢��ѯ����Ļ�����
	lv.cchTextMax = 0x20;	//ָ����������С
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