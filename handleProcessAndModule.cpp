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

	//��ʼ��						
	memset(&vitem, 0, sizeof(LV_ITEM));
	vitem.mask = LVIF_TEXT;

	//vitem.pszText = const_cast<wchar_t*>(TEXT("csrss.exe"));
	vitem.pszText = szProcessName;
	vitem.iItem = 0;		//�ڼ���				
	vitem.iSubItem = 0;		//�ڼ���
	// ����һ���� �� sendMessage��������һ��
	//ListView_InsertItem(hListProcess, &vitem);
	//���½���Ϣ
	SendMessage(hListProcess, LVM_INSERTITEM, 0, (DWORD)&vitem);

	//��һ��д��֮�� ����ȫ�� SETITEM
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

	//��ʼ��
	memset(&vitem, 0, sizeof(LV_ITEM));
	vitem.mask = LVIF_TEXT;

	vitem.pszText = const_cast <wchar_t*>(TEXT("kernel.dll"));
	vitem.iItem = 0;		//�ڼ���				
	vitem.iSubItem = 0;		//�ڼ���
	SendMessage(hListModule, LVM_INSERTITEM, 0, (DWORD)&vitem);

	//��һ��д��֮�� ����ȫ�� SETITEM
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