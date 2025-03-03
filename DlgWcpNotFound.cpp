// DlgWcpNotFound.cpp : implementation file
//

#include "stdafx.h"
#include "wclust.h"
#include "DlgWcpNotFound.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgWcpNotFound dialog


CDlgWcpNotFound::CDlgWcpNotFound(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgWcpNotFound::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgWcpNotFound)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgWcpNotFound::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgWcpNotFound)
	DDX_Control(pDX, IDC_LIST, m_wndList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgWcpNotFound, CDialog)
	//{{AFX_MSG_MAP(CDlgWcpNotFound)
	ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgWcpNotFound message handlers

bool CDlgWcpNotFound::CheckWcp()
{
	bool state = true;
	
	MY_WCP_FILES_STACK::iterator indWantWcp;	// from *.CL
	MY_WCP_FILES_STACK::iterator indExistsWcp;	// on disk
	
	notFoundStack.clear();

	for (indWantWcp = m_MainClusters->usedExternWCP.begin(); indWantWcp != m_MainClusters->usedExternWCP.end(); indWantWcp++)
	{
		indExistsWcp = m_MainDataStack->wcpFilesStack.begin();

		char foundIt = 0;
		while ( !foundIt && indExistsWcp != m_MainDataStack->wcpFilesStack.end() )
		{
			if ( (*indWantWcp).paramName.Compare((*indExistsWcp).paramName) == 0 )
			{
				foundIt = 1;
				(*indWantWcp).fileName = (*indExistsWcp).fileName;
				m_MainDataStack->wcpFilesStack.erase( indExistsWcp );			
			}
			else
				indExistsWcp++;
		}
		if ( !foundIt )
		{
			state = false;
			(*indWantWcp).fileName = CString("-");
			notFoundStack.push_back( (*indWantWcp) );
		}
	}
	return state;
}

void CDlgWcpNotFound::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void CDlgWcpNotFound::OnRefresh() 
{
	m_MainDataStack->CheckWcpFiles();
	bool result = CheckWcp();
	if ( result == true )
	{
		CDialog::OnOK();
	}
	else
	{
		FillList();
		Invalidate(false);
	}
}

void CDlgWcpNotFound::FillList()
{
	m_wndList.ResetContent();

	MY_WCP_FILES_STACK::iterator indNotFound;
	for ( indNotFound = notFoundStack.begin(); indNotFound != notFoundStack.end(); indNotFound++ )
	{
		m_wndList.AddString((LPCTSTR) (*indNotFound).paramName);
	}
}

void CDlgWcpNotFound::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);
	
	FillList();	
}
