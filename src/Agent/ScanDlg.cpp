// ScanDlg.cpp: 구현 파일
//

#include "pch.h"
#include "Agent.h"
#include "ScanDlg.h"
#include "afxdialogex.h"
#include <vector>

using namespace std;

// CScan 대화 상자

IMPLEMENT_DYNAMIC(CScan, CDialogEx)

CScan::CScan(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SCAN_DIALOG, pParent)
{

}

CScan::~CScan()
{
}

void CScan::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_PATTERN, m_cPattern);
	DDX_Control(pDX, IDC_STATIC_NUM_PATTERN, m_cStaticNumPattern);
	DDX_Control(pDX, IDC_EDIT_RESULT, m_cResult);
}


BEGIN_MESSAGE_MAP(CScan, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_SCAN, &CScan::OnBnClickedButtonScan)
END_MESSAGE_MAP()


// CScan 메시지 처리기

BOOL CScan::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	return TRUE;
}

BOOL CScan::PreTranslateMessage(MSG* pMsg)
{
	CEdit* edit;
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		if (GetDlgItem(IDC_EDIT_PATTERN) == GetFocus())
		{
			edit = (CEdit*)GetDlgItem(IDC_EDIT_PATTERN);
			edit->ReplaceSel(_T("\r\n"));
		}
		
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CScan::SetClass(CCommunication cComm)
{
	m_cComm = cComm;
}

void CScan::OnBnClickedButtonScan()
{
	CString strMsg; 
	CString result;

	vector<BYTE> Pattern;

	CString strEdit; // 에디트 컨트롤에서 가져온 문자열
	GetDlgItemTextW(IDC_EDIT_PATTERN, strEdit);

	// 입력 데이터가 너무 크면 뱉음
	if (strEdit.GetLength() > 4096)
	{
		AfxMessageBox(L"The maximum input string length is 4096 bytes.");
		goto EXIT_ERROR;
	}
	strEdit.Replace(L" ", NULL); // 공백 제거

	// 문자열 유효성 검사
	if (strEdit.SpanIncluding(L"0123456789abcdefABCDEF\r\n") != strEdit)
	{
		AfxMessageBox(L"It contains non-HEX strings.\r\nYou can only enter hex.");
		goto EXIT_ERROR;
	}
		
	int nPatternCount = 0;

	while (!strEdit.IsEmpty())
	{
		CString strLine = strEdit.SpanExcluding(L"\r\n");
		strEdit = strEdit.Right(strEdit.GetLength() - strLine.GetLength()).TrimLeft(L"\r\n");
		if (strLine.IsEmpty())
		{
			continue;
		}

		int nLineLen = strLine.GetLength();
		if (nLineLen < 8)
		{
			strMsg.Format(L"The pattern(No. %d) size is too short. Input at least 4 bytes.", nPatternCount + 1);
			AfxMessageBox(strMsg);
			goto EXIT_ERROR;
		}
		if (nLineLen % 2 != 0)
		{
			strMsg.Format(L"The pattern(No. %d) size is not even.\r\nEX) BFF(WRONG) 0BFF(TRUE)", nPatternCount + 1);
			AfxMessageBox(strMsg);
			goto EXIT_ERROR;
		}
		// 시작 패턴을 알리는 signature
		Pattern.push_back('S');
		Pattern.push_back('T');
		Pattern.push_back('a');
		Pattern.push_back('r');
		Pattern.push_back('t');

		// 한줄읽은거에서 바이트 형식으로 변환
		for (int j = 0; j < nLineLen; j = j + 2)
		{
			WCHAR temp[5] = { 0, };
			wcsncpy_s(temp, 5, strLine.GetBuffer() + j, 2);
			Pattern.push_back((BYTE)wcstoul(temp, NULL, 16));
		}
	
		// 끝 패턴을 알리는 signature
		Pattern.push_back('e');
		Pattern.push_back('n');
		Pattern.push_back('D');

		nPatternCount++;				
	}
	
	strMsg.Format(L"Input Pattern count : %d", nPatternCount);
	m_cStaticNumPattern.SetWindowTextW(strMsg);

	// 잘 들어갔는지 로깅	
	char* szBuffer = (char*)malloc(5000);
	memset(szBuffer, 0, 5000);
	printf("pattern size : %zd\n", Pattern.size());
	for (int i = 0; i < Pattern.size(); i++)
	{
		if (i % 16 == 0)
		{
			strcat_s(szBuffer, 5000, "\r\n");
		}
		char temp[5] = { 0, };
		sprintf_s(temp, 5, "%02x ", Pattern[i]);
		strcat_s(szBuffer, 5000, temp);
	}
	printf("%s\r\n", szBuffer);
	free(szBuffer);

	BOOL bSuccess = m_cComm.ScanPattern(&Pattern.front(), Pattern.size());
	if (FALSE == bSuccess)
	{
		AfxMessageBox(L"failed to communicate with driver.");
		goto EXIT_ERROR;
	}
	
	// 0개 검출시 1바이트가 리턴옴
	if (m_cComm.m_DetectListEntry.size() == 0)
	{
		m_cResult.SetWindowText(L"No pattern detected.");
		goto EXIT;
	}
	

	for (int i = 0; i < m_cComm.m_DetectListEntry.size(); i++)
	{
		for (int j = 0; j < m_cComm.m_DriverListEntry.size(); j++)
		{
			if (m_cComm.m_DriverListEntry[j].modBaseAddress == m_cComm.m_DetectListEntry[i].BaseAddress)
			{
				strMsg.Format(L"Pattern No (%d) Base : %llX Offset : %X %s\r\n", 
					m_cComm.m_DetectListEntry[i].PatternNo, 
					m_cComm.m_DetectListEntry[i].BaseAddress, 
					m_cComm.m_DetectListEntry[i].Offset, 
					m_cComm.m_DriverListEntry[j].FilePath);
				result = result + strMsg;
			}
		}		
	}	
	m_cComm.m_DetectListEntry.clear();
	m_cResult.SetWindowText(result);
EXIT:
EXIT_ERROR:
	return;
}
