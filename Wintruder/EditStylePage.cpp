
#include "pch.h"
#include "EditStylePage.h"
#include "resource.h"
#include "wnd/FlagRepository.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CEditStylePage::CEditStylePage( void )
	: CEditFlagsBasePage( _T("StylePage") )
{
	LoadPageInfo( IDS_EDIT_STYLE_PAGE );

	m_childPageTipText[ GeneralPage ] = _T("General Window Styles");
	m_childPageTipText[ SpecificPage ] = _T("Class Specific Window Styles");
	m_unknownTipLabel = _T("Unknown Style Mask");
}

CEditStylePage::~CEditStylePage()
{
}

void CEditStylePage::StoreFlagStores( HWND hTargetWnd )
{
	ASSERT_PTR( hTargetWnd );
	m_pGeneralStore = CStyleRepository::Instance().GetGeneralStore( HasFlag( ui::GetStyle( hTargetWnd ), WS_CHILD ) );
	m_pSpecificStore = CStyleRepository::Instance().FindSpecificStore( m_wndClass );
}

BEGIN_MESSAGE_MAP( CEditStylePage, CEditFlagsBasePage )
END_MESSAGE_MAP()
