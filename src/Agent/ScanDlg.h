#pragma once
#include "Communication.h"

// CScan 대화 상자

class CScan : public CDialogEx
{
	DECLARE_DYNAMIC(CScan)

public:
	CScan(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CScan();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SCAN_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedButtonScan();;
	void SetClass(CCommunication cComm);

private:
	CCommunication m_cComm;
public:
	CStatic m_cStaticNumPattern;
	CEdit m_cResult;	
	CEdit m_cPattern;
};
