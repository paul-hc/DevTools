
#include "stdafx.h"
#include "FlagsListPage.h"
#include "FlagsListCtrl.h"
#include "AppService.h"
#include "Application.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDC_FLAGS_EDITOR_LIST, Stretch }
	};
}


CFlagsListPage::CFlagsListPage( IEmbeddedPageCallback* pParentCallback, const TCHAR* pTitle /*= NULL*/ )
	: CLayoutPropertyPage( IDD_FLAG_LIST_PAGE )
	, m_pParentCallback( pParentCallback )
	, m_pFlagsListCtrl( new CFlagsListCtrl )
{
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );

	if ( pTitle != NULL )
		SetTitle( pTitle );
}

CFlagsListPage::~CFlagsListPage()
{
}

CBaseFlagsCtrl* CFlagsListPage::GetFlagsCtrl( void )
{
	return m_pFlagsListCtrl.get();
}

void CFlagsListPage::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* /*pTooltip*/ ) const
{
	if ( IDC_FLAGS_EDITOR_LIST == cmdId )
		rText = m_pFlagsListCtrl->FormatTooltip();
}

void CFlagsListPage::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_FLAGS_EDITOR_LIST, *m_pFlagsListCtrl );

	CLayoutPropertyPage::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CFlagsListPage, CLayoutPropertyPage )
	ON_FN_FLAGSCHANGED( IDC_FLAGS_EDITOR_LIST, OnFnFlagsChanged_FlagsList )
END_MESSAGE_MAP()

void CFlagsListPage::OnFnFlagsChanged_FlagsList( void )
{
	if ( m_pParentCallback != NULL )
		m_pParentCallback->OnChildPageNotify( this, m_pFlagsListCtrl.get(), CBaseFlagsCtrl::FN_FLAGSCHANGED );
}