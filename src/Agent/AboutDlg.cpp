// About.cpp: 구현 파일
//

#include "pch.h"
#include "Agent.h"
#include "AboutDlg.h"
#include "afxdialogex.h"


// About 대화 상자

IMPLEMENT_DYNAMIC(CAboutDlg, CDialogEx)

CAboutDlg::CAboutDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ABOUT_DIALOG, pParent)
{

}

CAboutDlg::~CAboutDlg()
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PICTURE, m_cPicture);
}


BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// About 메시지 처리기

BOOL CAboutDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// 이미지 띄우기
	m_cPicture.ModifyStyle(0xF, SS_BITMAP | SS_CENTERIMAGE);
	HBITMAP hBmp = ::LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_BITMAP1));
	m_cPicture.SetBitmap(hBmp);
	BITMAP size;
	::GetObject(hBmp, sizeof(BITMAP), &size);
	m_cPicture.MoveWindow(0, 0, size.bmWidth, size.bmHeight);

	// dialog 크기 조절
	::SetWindowPos(GetSafeHwnd(), NULL, 0, 0, size.bmWidth+15, size.bmHeight+40, SWP_NOMOVE | SWP_NOZORDER);
	return TRUE;
}
