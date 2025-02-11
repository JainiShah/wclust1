#if !defined(AFX_DIALLIST_H__35EE4D5D_2171_4DEC_8308_46D62606D4C0__INCLUDED_)
#define AFX_DIALLIST_H__35EE4D5D_2171_4DEC_8308_46D62606D4C0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DialList.h : header file
//
#include "cdxCSizingDialog.h"
#include "Cluster.h"


/////////////////////////////////////////////////////////////////////////////
// CDialList dialog
class CDialList : public cdxCSizingDialog
{
public:
	MY_INT_STACK AxStackX,AxStackY,ClStack;
	CCluster *m_MainClusters;
	CVerxStack *m_MainDataStack;
	int Cl, AxX, AxY, ListNum;
// Construction
public:
	CDialList(CWnd* pParent = NULL);   // standard constructor

	//iAxX = x axis, iAxY = y axis, iCl = cluster, str = listbox string
	void AddItem(int iAxX,int iAxY,int iCl,char* str);
	//clear all data structures
	void Clear();

// Dialog Data
	//{{AFX_DATA(CDialList)
	enum { IDD = IDD_DIALLIST };
	CButton	m_wndCancel;
	CButton	m_wndOK;
	CListBox	m_wndList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialList)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDialList)
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSelchangeList1();
	afx_msg void OnDblclkList1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALLIST_H__35EE4D5D_2171_4DEC_8308_46D62606D4C0__INCLUDED_)
