
#include "stdafx.h"
#include "EditStyleExPage.h"
#include "resource.h"
#include "wnd/FlagRepository.h"
#include "utl/UI/CmdInfoStore.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CEditStyleExPage::CEditStyleExPage( void )
	: CEditFlagsBasePage( _T("StylePage") )
{
	LoadPageInfo( IDS_EDIT_STYLE_EX_PAGE );
	m_pSpecificFlags.reset( new DWORD( 0 ) );

	m_childPageTipText[ GeneralPage ] = _T("General Window Extended Styles");
	m_childPageTipText[ SpecificPage ] = _T("Class Specific Window Extended Styles");
	m_unknownTipLabel = _T("Unknown General Mask");
}

CEditStyleExPage::~CEditStyleExPage()
{
}

void CEditStyleExPage::StoreFlagStores( HWND hTargetWnd )
{
	hTargetWnd;
	m_pGeneralStore = CStyleExRepository::Instance().FindStore();
	m_pSpecificStore = CStyleExRepository::Instance().FindStore( m_wndClass );
}

BEGIN_MESSAGE_MAP( CEditStyleExPage, CEditFlagsBasePage )
END_MESSAGE_MAP()
