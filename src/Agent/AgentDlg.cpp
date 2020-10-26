
// AgentDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "Agent.h"
#include "AgentDlg.h"
#include "afxdialogex.h"
#include "LoadDriver.h"
#include "AboutDlg.h"
#include "ScanDlg.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAgentDlg 대화 상자



CAgentDlg::CAgentDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_AGENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_bInit = FALSE;
}

void CAgentDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST, m_cList);
}

BEGIN_MESSAGE_MAP(CAgentDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST, OnNMCustomdraw)

	ON_WM_CLOSE()
	ON_WM_SIZE()
	ON_COMMAND(ID_DRIVER_REFRESH, &CAgentDlg::OnDriverRefreshList)
	ON_COMMAND(ID_DRIVER_HIDEMYSELF, &CAgentDlg::OnDriverHideMyself)
	ON_COMMAND(ID_ABOUT, &CAgentDlg::OnAbout)
	ON_COMMAND(ID_PATTERNSCAN, &CAgentDlg::OnPatternscan)

	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_POPUP_REFRESH, &CAgentDlg::OnDriverRefreshList)
	ON_COMMAND(ID_POPUP_HIDESELECTEDDRIVER, &CAgentDlg::OnPopupHideSelectedDriver)

END_MESSAGE_MAP()

// 리스트컨트롤 row 색상 바꾸기
void CAgentDlg::OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

	*pResult = 0;

	// 숨김인 경우만
	CString hidden = m_cList.GetItemText((int)pLVCD->nmcd.dwItemSpec, 6);

	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
	{	
		COLORREF crText;

		// 숨김인 경우만
		if (hidden == "YES")
			crText = RGB(255, 0, 0);
		else
			crText = RGB(0, 0, 0);

		pLVCD->clrText = crText;
		*pResult = CDRF_DODEFAULT;
	}

}

// CAgentDlg 메시지 처리기

BOOL CAgentDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	/*
	if (AllocConsole())
	{
		FILE* stream = NULL;
		freopen_s(&stream, "CONOUT$", "w", stdout);
		SetConsoleTitle(L"Console");
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		_wsetlocale(0, L"korean");
	}
	*/

	m_cList.InsertColumn(0, L"No", LVCFMT_LEFT, 30);
	m_cList.InsertColumn(1, L"Base", LVCFMT_LEFT, 120);
	m_cList.InsertColumn(2, L"Size", LVCFMT_LEFT, 65);
	m_cList.InsertColumn(3, L"DriverObject", LVCFMT_LEFT, 125);
	m_cList.InsertColumn(4, L"Path", LVCFMT_LEFT, 400);
	m_cList.InsertColumn(5, L"Service", LVCFMT_LEFT, 120);
	m_cList.InsertColumn(6, L"Hidden", LVCFMT_LEFT, 50);

	m_cList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	// 드라이버 시작
	CLoadDriver cLoadDriver;
	BOOL bSuccess = cLoadDriver.Load();
	if (FALSE == bSuccess)
	{		
		goto EXIT_ERROR;
	}

	m_bInit = TRUE;

	bSuccess = m_cComm.Initialize();
	if (FALSE == bSuccess)
	{	
		goto EXIT_ERROR;
	}
	OnDriverRefreshList();

EXIT_ERROR:
	return TRUE; 
}

void CAgentDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); 

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

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
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CAgentDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


BOOL CAgentDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN)
			return TRUE;
		else if (pMsg->wParam == VK_ESCAPE)
			return TRUE;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}


void CAgentDlg::OnClose()
{	
	m_cComm.Finalize();
	CLoadDriver cLoadDriver;

	cLoadDriver.Unload();

	CDialogEx::OnClose();
}


void CAgentDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	
	// dialog 크기가 변하면 리스트컨트롤 크기도 조절함
	if (m_bInit)
	{
		CWnd* pCtl = GetDlgItem(IDC_LIST);
		CRect rectCtl;
		pCtl->GetWindowRect(&rectCtl);
		ScreenToClient(&rectCtl);
		pCtl->MoveWindow(rectCtl.left, rectCtl.top, cx - 2 * rectCtl.left, cy - rectCtl.top - rectCtl.left, TRUE);
	}
	
}


void CAgentDlg::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CMenu popup;
	CMenu* pMenu;

	popup.LoadMenuW(IDR_POPUP_MENU);
	pMenu = popup.GetSubMenu(0);

	pMenu->TrackPopupMenu(TPM_LEFTALIGN || TPM_RIGHTBUTTON, point.x, point.y, this);
}

/*
* 이하 메뉴 클릭 이벤트 처리
*/

// 팝업 메뉴와 함께 사용
void CAgentDlg::OnDriverRefreshList()
{

	BOOL bSuccess = m_cComm.GetDrivers();
	if (FALSE == bSuccess)
	{
		AfxMessageBox(L"failed to communicate with driver.");
		return;
	}
	m_cList.DeleteAllItems();
	for (int i = 0; i < m_cComm.m_nDriverListEntryCount; i++)
	{
		int nIndex = m_cList.InsertItem(m_cList.GetItemCount(), L"");
		CString str;
		str.Format(L"%d", nIndex);
		m_cList.SetItem(nIndex, 0, LVIF_TEXT, str, NULL, NULL, NULL, NULL);

		str.Format(L"%llx", m_cComm.m_pDriverEntry[i].modBaseAddress);
		str.MakeUpper();
		m_cList.SetItem(nIndex, 1, LVIF_TEXT, str, NULL, NULL, NULL, NULL);

		str.Format(L"%x", m_cComm.m_pDriverEntry[i].modSize);
		str.MakeUpper();
		m_cList.SetItem(nIndex, 2, LVIF_TEXT, str, NULL, NULL, NULL, NULL);
		if (m_cComm.m_pDriverEntry[i].DriverObject != 0)
		{
			str.Format(L"%llx", m_cComm.m_pDriverEntry[i].DriverObject);
			str.MakeUpper();
			m_cList.SetItem(nIndex, 3, LVIF_TEXT, str, NULL, NULL, NULL, NULL);
		}
		m_cList.SetItem(nIndex, 4, LVIF_TEXT, m_cComm.m_pDriverEntry[i].FilePath, NULL, NULL, NULL, NULL);
		m_cList.SetItem(nIndex, 5, LVIF_TEXT, m_cComm.m_pDriverEntry[i].ServiceName, NULL, NULL, NULL, NULL);
		if (TRUE == m_cComm.m_pDriverEntry[i].isHidden)
		{
			m_cList.SetItem(nIndex, 6, LVIF_TEXT, L"YES", NULL, NULL, NULL, NULL);
		}
		else
		{
			m_cList.SetItem(nIndex, 6, LVIF_TEXT, L"NO", NULL, NULL, NULL, NULL);
		}
	}
}

void CAgentDlg::OnDriverHideMyself()
{	
	BOOL bSuccess = m_cComm.HideMyself();
	if (FALSE == bSuccess)
	{
		AfxMessageBox(L"failed to communicate with driver.");
		return;
	}
	AfxMessageBox(L"succeeded in hiding the driver.");
	OnDriverRefreshList();
}

void CAgentDlg::OnPopupHideSelectedDriver()
{	
	int nPosition = m_cList.GetSelectionMark();
	CString strFilePath = m_cList.GetItemText(nPosition, 4);
	LPCWSTR pFilePath = strFilePath.GetBuffer();
	CString strServiceName = m_cList.GetItemText(nPosition, 5);
	LPCWSTR pServiceName = strServiceName.GetBuffer();

	BOOL bHide = FALSE;
	BOOL bSuccess = m_cComm.HideDriver(pFilePath, pServiceName, &bHide);
	if (FALSE == bSuccess)
	{
		AfxMessageBox(L"failed to communicate with driver.");
		goto EXIT;
	}
	if (bHide)
	{
		AfxMessageBox(L"succeeded in hiding the driver.");
	}
	else
	{
		AfxMessageBox(L"failed to hide the driver.");
	}
EXIT:
	strFilePath.ReleaseBuffer();
	OnDriverRefreshList();
}

void CAgentDlg::OnAbout()
{
	CAboutDlg about;
	about.DoModal();
}

void CAgentDlg::OnPatternscan()
{	
	CScan scan;
	scan.SetClass(m_cComm);
	scan.DoModal();
}
