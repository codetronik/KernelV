
// AgentDlg.h: 헤더 파일
//

#pragma once
#include "Communication.h"

// CAgentDlg 대화 상자
class CAgentDlg : public CDialogEx
{
// 생성입니다.
public:
	CAgentDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_AGENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnNMCustomdraw(NMHDR*, LRESULT*);
	DECLARE_MESSAGE_MAP()

public:
	
	CListCtrl m_cList;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnClose();

private: 
	CCommunication m_cComm;
	BOOL m_bInit;
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDriverRefreshList();
	afx_msg void OnDriverHideMyself();

	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);

	afx_msg void OnPopupHideSelectedDriver();
	afx_msg void OnAbout();
	afx_msg void OnPatternscan();
};
