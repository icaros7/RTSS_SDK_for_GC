#include "stdafx.h"
#include "RTSSSharedMemory.h"
#include "RTSSSharedMemorySample.h"
#include "RTSSSharedMemorySampleDlg.h"
#include "GroupedString.h"
#include <shlwapi.h>
#include <float.h>
#include <io.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	enum { IDD = IDD_ABOUTBOX };
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
protected:
	DECLARE_MESSAGE_MAP()
};
CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}
void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}
BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()
CRTSSSharedMemorySampleDlg::CRTSSSharedMemorySampleDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRTSSSharedMemorySampleDlg::IDD, pParent)
{

	m_hIcon						= AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_dwNumberOfProcessors		= 0;
	m_pNtQuerySystemInformation	= NULL;
	m_strStatus					= "";
	m_strInstallPath			= "";

	m_bMultiLineOutput			= TRUE;
	m_bFormatTags				= TRUE;
	m_bFillGraphs				= FALSE;
	m_bConnected				= FALSE;

	m_dwHistoryPos				= 0;
}
void CRTSSSharedMemorySampleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}
BEGIN_MESSAGE_MAP(CRTSSSharedMemorySampleDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()
BOOL CRTSSSharedMemorySampleDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	SetIcon(m_hIcon, TRUE);			// Set big icon


	CWnd* pPlaceholder = GetDlgItem(IDC_PLACEHOLDER);

	if (pPlaceholder)
	{
		CRect rect; 
		pPlaceholder->GetClientRect(&rect);

		if (!m_richEditCtrl.Create(WS_VISIBLE|ES_READONLY|ES_MULTILINE|ES_AUTOHSCROLL|WS_HSCROLL|ES_AUTOVSCROLL|WS_VSCROLL, rect, this, 0))
			return FALSE;

		m_font.CreateFont(-11, 0, 0, 0, FW_REGULAR, 0, 0, 0, BALTIC_CHARSET, 0, 0, 0, 0, "Courier New");
		m_richEditCtrl.SetFont(&m_font);
	}

	m_nTimerID = SetTimer(0x1234, 1000, NULL);

	Refresh();
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}
void CRTSSSharedMemorySampleDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}
void CRTSSSharedMemorySampleDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}
HCURSOR CRTSSSharedMemorySampleDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}
void CRTSSSharedMemorySampleDlg::OnTimer(UINT nIDEvent) 
{
	Refresh();
	
	CDialog::OnTimer(nIDEvent);
}
void CRTSSSharedMemorySampleDlg::OnDestroy() 
{
	if (m_nTimerID)
		KillTimer(m_nTimerID);

	m_nTimerID = NULL;

	MSG msg; 
	while (PeekMessage(&msg, m_hWnd, WM_TIMER, WM_TIMER, PM_REMOVE));

	ReleaseOSD();

	CDialog::OnDestroy();	
}
DWORD CRTSSSharedMemorySampleDlg::GetSharedMemoryVersion()
{
	DWORD dwResult	= 0;

	HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "RTSSSharedMemoryV2");

	if (hMapFile)
	{
		LPVOID pMapAddr				= MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		LPRTSS_SHARED_MEMORY pMem	= (LPRTSS_SHARED_MEMORY)pMapAddr;

		if (pMem)
		{
			if ((pMem->dwSignature == 'RTSS') && 
				(pMem->dwVersion >= 0x00020000))
				dwResult = pMem->dwVersion;

			UnmapViewOfFile(pMapAddr);
		}

		CloseHandle(hMapFile);
	}

	return dwResult;
}
DWORD CRTSSSharedMemorySampleDlg::EmbedGraph(DWORD dwOffset, FLOAT* lpBuffer, DWORD dwBufferPos, DWORD dwBufferSize, LONG dwWidth, LONG dwHeight, LONG dwMargin, FLOAT fltMin, FLOAT fltMax, DWORD dwFlags)
{
	DWORD dwResult	= 0;

	HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "RTSSSharedMemoryV2");

	if (hMapFile)
	{
		LPVOID pMapAddr				= MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		LPRTSS_SHARED_MEMORY pMem	= (LPRTSS_SHARED_MEMORY)pMapAddr;

		if (pMem)
		{
			if ((pMem->dwSignature == 'RTSS') && 
				(pMem->dwVersion >= 0x00020000))
			{
				for (DWORD dwPass=0; dwPass<2; dwPass++)
				{
					for (DWORD dwEntry=1; dwEntry<pMem->dwOSDArrSize; dwEntry++)
					{
						RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY pEntry = (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY)((LPBYTE)pMem + pMem->dwOSDArrOffset + dwEntry * pMem->dwOSDEntrySize);

						if (dwPass)
						{
							if (!strlen(pEntry->szOSDOwner))
								strcpy_s(pEntry->szOSDOwner, sizeof(pEntry->szOSDOwner), "RTSSSharedMemorySample");
						}

						if (!strcmp(pEntry->szOSDOwner, "RTSSSharedMemorySample"))
						{
							if (pMem->dwVersion >= 0x0002000c)
							{
								if (dwOffset + sizeof(RTSS_EMBEDDED_OBJECT_GRAPH) + dwBufferSize * sizeof(FLOAT) > sizeof(pEntry->buffer))
								{
									UnmapViewOfFile(pMapAddr);

									CloseHandle(hMapFile);

									return 0;
								}

								LPRTSS_EMBEDDED_OBJECT_GRAPH lpGraph = (LPRTSS_EMBEDDED_OBJECT_GRAPH)(pEntry->buffer + dwOffset);

								lpGraph->header.dwSignature	= RTSS_EMBEDDED_OBJECT_GRAPH_SIGNATURE;
								lpGraph->header.dwSize		= sizeof(RTSS_EMBEDDED_OBJECT_GRAPH) + dwBufferSize * sizeof(FLOAT);
								lpGraph->header.dwWidth		= dwWidth;
								lpGraph->header.dwHeight	= dwHeight;
								lpGraph->header.dwMargin	= dwMargin;
								lpGraph->dwFlags			= dwFlags;
								lpGraph->fltMin				= fltMin;
								lpGraph->fltMax				= fltMax;
								lpGraph->dwDataCount		= dwBufferSize;

								if (lpBuffer && dwBufferSize)
								{
									for (DWORD dwPos=0; dwPos<dwBufferSize; dwPos++)
									{
										FLOAT fltData = lpBuffer[dwBufferPos];

										lpGraph->fltData[dwPos] = (fltData == FLT_MAX) ? 0 : fltData;

										dwBufferPos = (dwBufferPos + 1) & (dwBufferSize - 1);
									}
								}

								dwResult = lpGraph->header.dwSize;
							}

							break;
						}
					}

					if (dwResult)
						break;
				}
			}

			UnmapViewOfFile(pMapAddr);
		}

		CloseHandle(hMapFile);
	}

	return dwResult;
}
BOOL CRTSSSharedMemorySampleDlg::UpdateOSD(LPCSTR lpText)
{
	BOOL bResult	= FALSE;

	HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "RTSSSharedMemoryV2");

	if (hMapFile)
	{
		LPVOID pMapAddr				= MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		LPRTSS_SHARED_MEMORY pMem	= (LPRTSS_SHARED_MEMORY)pMapAddr;

		if (pMem)
		{
			if ((pMem->dwSignature == 'RTSS') && 
				(pMem->dwVersion >= 0x00020000))
			{
				for (DWORD dwPass=0; dwPass<2; dwPass++)
				{
					for (DWORD dwEntry=1; dwEntry<pMem->dwOSDArrSize; dwEntry++)
					{
						RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY pEntry = (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY)((LPBYTE)pMem + pMem->dwOSDArrOffset + dwEntry * pMem->dwOSDEntrySize);

						if (dwPass)
						{
							if (!strlen(pEntry->szOSDOwner))
								strcpy_s(pEntry->szOSDOwner, sizeof(pEntry->szOSDOwner), "RTSSSharedMemorySample");
						}

						if (!strcmp(pEntry->szOSDOwner, "RTSSSharedMemorySample"))
						{
							if (pMem->dwVersion >= 0x00020007)
								strncpy_s(pEntry->szOSDEx, sizeof(pEntry->szOSDEx), lpText, sizeof(pEntry->szOSDEx) - 1);	
							else
								strncpy_s(pEntry->szOSD, sizeof(pEntry->szOSD), lpText, sizeof(pEntry->szOSD) - 1);

							pMem->dwOSDFrame++;

							bResult = TRUE;

							break;
						}
					}

					if (bResult)
						break;
				}
			}

			UnmapViewOfFile(pMapAddr);
		}

		CloseHandle(hMapFile);
	}

	return bResult;
}
void CRTSSSharedMemorySampleDlg::ReleaseOSD()
{
	HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "RTSSSharedMemoryV2");

	if (hMapFile)
	{
		LPVOID pMapAddr = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);

		LPRTSS_SHARED_MEMORY pMem = (LPRTSS_SHARED_MEMORY)pMapAddr;

		if (pMem)
		{
			if ((pMem->dwSignature == 'RTSS') && 
				(pMem->dwVersion >= 0x00020000))
			{
				for (DWORD dwEntry=1; dwEntry<pMem->dwOSDArrSize; dwEntry++)
				{
					RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY pEntry = (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY)((LPBYTE)pMem + pMem->dwOSDArrOffset + dwEntry * pMem->dwOSDEntrySize);

					if (!strcmp(pEntry->szOSDOwner, "RTSSSharedMemorySample"))
					{
						memset(pEntry, 0, pMem->dwOSDEntrySize);
						pMem->dwOSDFrame++;
					}
				}
			}

			UnmapViewOfFile(pMapAddr);
		}

		CloseHandle(hMapFile);
	}
}

DWORD CRTSSSharedMemorySampleDlg::GetClientsNum()
{
	DWORD dwClients = 0;	

	HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "RTSSSharedMemoryV2");

	if (hMapFile)
	{
		LPVOID pMapAddr = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);

		LPRTSS_SHARED_MEMORY pMem = (LPRTSS_SHARED_MEMORY)pMapAddr;

		if (pMem)
		{
			if ((pMem->dwSignature == 'RTSS') && 
				(pMem->dwVersion >= 0x00020000))
			{
				for (DWORD dwEntry=0; dwEntry<pMem->dwOSDArrSize; dwEntry++)
				{
					RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY pEntry = (RTSS_SHARED_MEMORY::LPRTSS_SHARED_MEMORY_OSD_ENTRY)((LPBYTE)pMem + pMem->dwOSDArrOffset + dwEntry * pMem->dwOSDEntrySize);

					if (strlen(pEntry->szOSDOwner))
						dwClients++;
				}
			}

			UnmapViewOfFile(pMapAddr);
		}

		CloseHandle(hMapFile);
	}

	return dwClients;
}
void CRTSSSharedMemorySampleDlg::Refresh()
{

	if (m_strInstallPath.IsEmpty())
	{
		HKEY hKey;

		if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\Unwinder\\RTSS", &hKey))
		{
			char buf[MAX_PATH];

			DWORD dwSize = MAX_PATH;
			DWORD dwType;

			if (ERROR_SUCCESS == RegQueryValueEx(hKey, "InstallPath", 0, &dwType, (LPBYTE)buf, &dwSize))
			{
				if (dwType == REG_SZ)
					m_strInstallPath = buf;
			}

			RegCloseKey(hKey);
		}
	}

	if (_taccess(m_strInstallPath, 0))
		m_strInstallPath = "";
	if (!m_strInstallPath.IsEmpty())
	{
		if (!m_profileInterface.IsInitialized())
			m_profileInterface.Init(m_strInstallPath);
	}
	DWORD dwSharedMemoryVersion = GetSharedMemoryVersion();
	DWORD dwMaxTextSize = (dwSharedMemoryVersion >= 0x00020007) ? sizeof(RTSS_SHARED_MEMORY::RTSS_SHARED_MEMORY_OSD_ENTRY().szOSDEx) : sizeof(RTSS_SHARED_MEMORY::RTSS_SHARED_MEMORY_OSD_ENTRY().szOSD);

	CGroupedString groupedString(dwMaxTextSize - 1);
	BOOL bFormatTagsSupported	= (dwSharedMemoryVersion >= 0x0002000b);
	BOOL bObjTagsSupported		= (dwSharedMemoryVersion >= 0x0002000c);

	CString strOSD;

	if (bFormatTagsSupported && m_bFormatTags)
	{
		if (GetClientsNum() == 1)
		strOSD += "<P=0,10>";
		strOSD += "<A0=-20>";
		strOSD += "<A1=3>";
		strOSD += "<A2=4>";
		strOSD += "<C0=00AAFF>";

		strOSD += "<C1=FFA200>";
		strOSD += "<C2=FF0000>";
		strOSD += "\r";
	}
	else
		strOSD = "";

	CString strGroup;
	if (bFormatTagsSupported && m_bFormatTags) 
		strGroup = "<C0>Gamer Clock<C>";
	else
		strGroup = "Gamer Clock";

	CString strValue;
	if (bFormatTagsSupported && m_bFormatTags) 
		strValue.Format("<A1><C0> for OSD<C><A>");
	else
		strValue.Format(" for OSD");

	groupedString.Add(strValue, strGroup, "\n", m_bFormatTags ? " " : ", ");
	m_dwHistoryPos = (m_dwHistoryPos + 1) & (MAX_HISTORY - 1);

	///////////////////////////////////

	TCHAR path[_MAX_PATH];
	GetModuleFileName(NULL, path, sizeof(path));
	CString strPath = path;
	int i = strPath.ReverseFind('\\');
	strPath = strPath.Left(i);

	TCHAR szBuf[MAX_PATH] = { 0, };
	CString temp = "\\OSD.ini";
	CString temp2 = strPath + temp;
	CString strINIPath(_T(temp2));
	CString strSection, strKey;

	strSection = _T("GamerClock");

	////////////////////////////////////


	if (bFormatTagsSupported && m_bFormatTags)
	{

		strKey = _T("Task");
		GetPrivateProfileString(strSection, strKey, _T(""), szBuf, MAX_PATH, strINIPath);
		groupedString.Add(szBuf, "<C2>Task<C>", "\n", m_bFormatTags ? " " : ", ");


		strKey = _T("Set");
		GetPrivateProfileString(strSection, strKey, _T(""), szBuf, MAX_PATH, strINIPath);
		groupedString.Add(szBuf, "<C1>Set Time<C>", "\n", m_bFormatTags ? " " : ", ");

		strKey = _T("Msg");
		GetPrivateProfileString(strSection, strKey, _T(""), szBuf, MAX_PATH, strINIPath);
		groupedString.Add(szBuf, "<C1>Message<C>", "\n", m_bFormatTags ? " " : ", ");
		//CString LeftTime = "13 : 13 : 13";
		//groupedString.Add(LeftTime, "<C2>Left Time<C>", "\n", m_bFormatTags ? " " : ", ");
	}
	else
	{
		strKey = _T("Task");
		GetPrivateProfileString(strSection, strKey, _T(""), szBuf, MAX_PATH, strINIPath);
		groupedString.Add(szBuf, "", "\n");

		strKey = _T("Set");
		GetPrivateProfileString(strSection, strKey, _T(""), szBuf, MAX_PATH, strINIPath);
		groupedString.Add(szBuf, "", "\n");

		strKey = _T("Msg");
		GetPrivateProfileString(strSection, strKey, _T(""), szBuf, MAX_PATH, strINIPath);
		groupedString.Add(szBuf, "", "\n");

		//groupedString.Add("13:13:13", "", "\n");
	}
																									
	BOOL bTruncated	= FALSE;

	strOSD += groupedString.Get(bTruncated, FALSE, m_bFormatTags ? "\t" : " \t: ");

	if (!strOSD.IsEmpty())
	{
		BOOL bResult = UpdateOSD(strOSD);

		m_bConnected = bResult;
		m_strStatus = "Gamer Clock for OSD 가 실행 중입니다!\n아래 출력을 확인 해 주세요.\n\n";

		if (bResult)
		{
			m_strStatus += "이 창을 닫지 말아주세요!\n";
			m_strStatus += "모든게 정상입니다.\n\n";
			strKey = _T("Set");
			GetPrivateProfileString(strSection, strKey, _T(""), szBuf, MAX_PATH, strINIPath);
			m_strStatus += "설정 시간 : ";
			m_strStatus += szBuf;

			m_strStatus += "\n예정 작업 : ";
			strKey = _T("Task");
			GetPrivateProfileString(strSection, strKey, _T(""), szBuf, MAX_PATH, strINIPath);
			m_strStatus += szBuf;

			m_strStatus += "\n메시지 : ";
			strKey = _T("Msg");
			GetPrivateProfileString(strSection, strKey, _T(""), szBuf, MAX_PATH, strINIPath);
			m_strStatus += szBuf;

		}
		else
		{

			if (m_strInstallPath.IsEmpty())
				m_strStatus += "Riva Tuner Statistics Server가 설치 되어있지 않습니다!!\nRTSS를 설치 후 사용해 주세요.\n";
			else
				m_strStatus += "Riva Tuner Statistics Server가 설치는 되어 있지만 응답하지 않습니다.\n<스페이스바>를 누르시면 RTSS를 수동 실행 시킵니다.";
		}

		if (m_profileInterface.IsInitialized())
		{
			m_strStatus += "\n\nRTSS_Gamer_Clock v1.0.0.0\n";
			m_strStatus += "hominlab@gmail.com";
		}

		m_richEditCtrl.SetWindowText(m_strStatus);
	}
}
BOOL CRTSSSharedMemorySampleDlg::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case 'F':
			if (m_bConnected)
			{
				m_bFormatTags = !m_bFormatTags;
				Refresh();
			}
			break;
		case 'I':
			if (m_bConnected)
			{
				m_bFillGraphs = !m_bFillGraphs;
				Refresh();
			}
			break;
		case ' ':
			if (m_bConnected)
			{
				m_bMultiLineOutput = !m_bMultiLineOutput;
				Refresh();
			}
			else
			{
				if (!m_strInstallPath.IsEmpty())
					ShellExecute(GetSafeHwnd(), "open", m_strInstallPath, NULL, NULL, SW_SHOWNORMAL);
			}
			return TRUE;
		case VK_UP:
			IncProfileProperty("", "PositionY", -1);
			return TRUE;
		case VK_DOWN:
			IncProfileProperty("", "PositionY", +1);
			return TRUE;
		case VK_LEFT:
			IncProfileProperty("", "PositionX", -1);
			return TRUE;
		case VK_RIGHT:
			IncProfileProperty("", "PositionX", +1);
			return TRUE;
		case 'R':
			SetProfileProperty("", "BaseColor", 0xFF0000);
			return TRUE;
		case 'G':
			SetProfileProperty("", "BaseColor", 0x00FF00);
			return TRUE;
		case 'B':
			SetProfileProperty("", "BaseColor", 0x0000FF);
			return TRUE;
		}
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}
/////////////////////////////////////////////////////////////////////////////
void CRTSSSharedMemorySampleDlg::IncProfileProperty(LPCSTR lpProfile, LPCSTR lpProfileProperty, LONG dwIncrement)
{
	if (m_profileInterface.IsInitialized())
	{
		m_profileInterface.LoadProfile(lpProfile);

		LONG dwProperty = 0;

		if (m_profileInterface.GetProfileProperty(lpProfileProperty, (LPBYTE)&dwProperty, sizeof(dwProperty)))
		{
			dwProperty += dwIncrement;

			m_profileInterface.SetProfileProperty(lpProfileProperty, (LPBYTE)&dwProperty, sizeof(dwProperty));
			m_profileInterface.SaveProfile(lpProfile);
			m_profileInterface.UpdateProfiles();
		}
	}
}
/////////////////////////////////////////////////////////////////////////////
void CRTSSSharedMemorySampleDlg::SetProfileProperty(LPCSTR lpProfile, LPCSTR lpProfileProperty, DWORD dwProperty)
{
	if (m_profileInterface.IsInitialized())
	{
		m_profileInterface.LoadProfile(lpProfile);
		m_profileInterface.SetProfileProperty(lpProfileProperty, (LPBYTE)&dwProperty, sizeof(dwProperty));
		m_profileInterface.SaveProfile(lpProfile);
		m_profileInterface.UpdateProfiles();
	}
}
/////////////////////////////////////////////////////////////////////////////
