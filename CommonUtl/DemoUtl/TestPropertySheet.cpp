
#include "pch.h"
#include "TestPropertySheet.h"
#include "DemoTemplate.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CTestPropertySheet::CTestPropertySheet( void )
	: CLayoutPropertySheet( _T("Properties - MODELESS"), NULL )
{
	Construct();
	m_styleMinMax = WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
}

CTestPropertySheet::CTestPropertySheet( CWnd* pParent )
	: CLayoutPropertySheet( _T("Properties - MODAL"), pParent )
{
	Construct();
	m_styleMinMax = WS_MAXIMIZEBOX;
}

CTestPropertySheet::~CTestPropertySheet()
{
	DeleteAllPages();
}

void CTestPropertySheet::Construct( void )
{
	m_regSection = _T("TestPropertySheet");
	LoadDlgIcon( ID_SEND_TO_CLIPBOARD );

	AddPage( new CListPage() );
	AddPage( new CEditPage() );
	AddPage( new CDetailsPage() );
	AddPage( new CDemoPage() );
}
