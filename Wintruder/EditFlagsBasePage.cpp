
#include "pch.h"
#include "EditFlagsBasePage.h"
#include "FlagsListPage.h"
#include "PromptDialog.h"
#include "AppService.h"
#include "Application.h"
#include "resource.h"
#include "wnd/FlagRepository.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDC_TOTAL_FLAGS_EDIT, pctSizeX( 50 ) },
		{ IDC_UNKNOWN_FLAGS_LABEL, pctMoveX( 50 ) },
		{ IDC_UNKNOWN_FLAGS_EDIT, pctMoveX( 50 ) | pctSizeX( 50 ) },
		{ IDC_DETAILS_SHEET, Size }
	};
}


CEditFlagsBasePage::CEditFlagsBasePage( const std::tstring& section, UINT templateId /*= 0*/ )
	: CDetailBasePage( templateId != 0 ? templateId : IDD_EDIT_FLAGS_PAGE )
	, m_pGeneralStore( nullptr )
	, m_pSpecificStore( nullptr )
	, m_flags( 0 )
	, m_hWndLastTarget( nullptr )
	, m_pGeneralList( nullptr )
	, m_pSpecificList( nullptr )
{
	if ( 0 == templateId )
		RegisterCtrlLayout( ARRAY_SPAN( layout::styles ) );

	m_listSheet.m_regSection = section;
	m_listSheet.AddPage( new CFlagsListPage( this, _T("General") ) );
	m_listSheet.AddPage( new CFlagsListPage( this, _T("n/a") ) );
}

CEditFlagsBasePage::~CEditFlagsBasePage()
{
}

void CEditFlagsBasePage::LoadPageInfo( UINT id )
{
	m_pageInfo.Setup( str::Load( id ) );
	ASSERT( m_pageInfo.IsValid() );
	SetTitle( m_pageInfo.m_statusText );
}

void CEditFlagsBasePage::QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	if ( GetTemplateId() == cmdId )
		rText = m_pageInfo.m_tooltipText;
	else if ( IDC_TOTAL_FLAGS_EDIT == cmdId )
		rText = m_totalEdit.FormatTooltip();
	else if ( IDC_UNKNOWN_FLAGS_EDIT == cmdId )
		rText = m_unknownEdit.FormatTooltip();
	else
		CDetailBasePage::QueryTooltipText( rText, cmdId, pTooltip );
}

void CEditFlagsBasePage::SetSpecificFlags( DWORD specificFlags )
{
	if ( UseSpecificFlags() )
		*m_pSpecificFlags = specificFlags;
}

bool CEditFlagsBasePage::IsDirty( void ) const
{
	if ( HWND hTargetWnd = app::GetValidTargetWnd()->GetSafeHwnd() )
	{
		DWORD flags = m_pGeneralStore->GetWindowField( hTargetWnd );
		if ( flags != m_flags )
			return true;

		if ( UseSpecificFlags() && m_pSpecificStore != nullptr )
			if ( *m_pSpecificFlags != m_pSpecificStore->GetWindowField( hTargetWnd ) )
				return true;
	}
	return false;
}

void CEditFlagsBasePage::OnTargetWndChanged( const CWndSpot& targetWnd )
{
	m_wndClass.clear();
	m_pGeneralStore = m_pSpecificStore = nullptr;
	m_flags = 0;
	SetSpecificFlags( 0 );

	bool valid = targetWnd.IsValid();
	ui::EnableWindow( *this, valid );

	if ( valid )
	{
		m_wndClass = wc::GetClassName( targetWnd );
		StoreFlagStores( targetWnd );

		if ( m_pGeneralStore != nullptr )
			m_flags = m_pGeneralStore->GetWindowField( targetWnd );

		if ( m_pSpecificStore != nullptr )
			SetSpecificFlags( m_pSpecificStore->GetWindowField( targetWnd ) );
	}

	UINT unknownMask = 0;
	if ( m_pGeneralStore != nullptr )
		unknownMask = UINT_MAX & ~m_pGeneralStore->GetMask();
	if ( m_pSpecificStore != nullptr && !UseSpecificFlags() )			// single m_flags
		unknownMask &= ~m_pSpecificStore->GetMask();

	std::vector< const CFlagStore* > flagStores;
	QueryAllFlagStores( flagStores );

	m_totalEdit.SetMultiFlagStores( flagStores, UINT_MAX );
	m_unknownEdit.SetFlagsMask( unknownMask );
	m_pGeneralList->SetFlagStore( m_pGeneralStore );
	m_pSpecificList->SetFlagStore( m_pSpecificStore );

	m_totalEdit.SetFlags( m_flags );
	m_unknownEdit.SetFlags( EvalUnknownFlags( m_flags ) );
	m_pGeneralList->SetFlags( m_flags );
	m_pSpecificList->SetFlags( UseSpecificFlags() ? *m_pSpecificFlags : m_flags );

	if ( m_pSpecificStore != nullptr )
		m_listSheet.SetPageTitle( SpecificPage, wc::FormatClassName( targetWnd ) );
	else
		m_listSheet.SetPageTitle( SpecificPage, _T("N/A") );

	m_hWndLastTarget = targetWnd.GetSafeHwnd();		// remember last target window

	SetModified( false );
}

void CEditFlagsBasePage::QueryAllFlagStores( std::vector< const CFlagStore* >& rFlagStores ) const
{
	if ( m_pGeneralStore != nullptr )
		rFlagStores.push_back( m_pGeneralStore );
	if ( m_pSpecificStore != nullptr )
		rFlagStores.push_back( m_pSpecificStore );
}

DWORD CEditFlagsBasePage::EvalUnknownFlags( DWORD flags ) const
{
	DWORD unknownFlags = flags;

	if ( m_pGeneralStore != nullptr )
		m_pGeneralList->StripUsedFlags( &unknownFlags );

	if ( m_pSpecificStore != nullptr && !UseSpecificFlags() )
		m_pSpecificList->StripUsedFlags( &unknownFlags );

	return unknownFlags;
}

void CEditFlagsBasePage::InputFlags( DWORD* pFlags, DWORD* pSpecificFlags /*= nullptr*/ ) const
{
	DWORD flags = m_pGeneralList->GetFlags() | m_unknownEdit.GetFlags(), specificFlags = m_pSpecificList->GetFlags();
	if ( !UseSpecificFlags() )
		flags |= specificFlags;

	if ( pFlags != nullptr )
		*pFlags = flags;
	if ( pSpecificFlags != nullptr )
		*pSpecificFlags = specificFlags;
}

void CEditFlagsBasePage::OnChildPageNotify( CLayoutPropertyPage* pEmbeddedPage, CWnd* pCtrl, int notifCode )
{
	pEmbeddedPage;
	if ( CBaseFlagsCtrl::FN_FLAGSCHANGED == notifCode )
	{
		CBaseFlagsCtrl* pFlagsCtrl = dynamic_cast<CBaseFlagsCtrl*>( pCtrl );
		if ( m_pGeneralList == pFlagsCtrl )
			OnFnFlagsChanged_GeneralList();
		else if ( m_pSpecificList == pFlagsCtrl )
			OnFnFlagsChanged_SpecificList();
	}
}

void CEditFlagsBasePage::ApplyPageChanges( void ) throws_( CRuntimeException )
{
	InputFlags( &m_flags, m_pSpecificFlags.get() );

	if ( HWND hTargetWnd = app::GetValidTargetWnd()->GetSafeHwnd() )
	{
		ASSERT_PTR( m_pGeneralStore );
		if ( m_flags != m_pGeneralStore->GetWindowField( hTargetWnd ) )
			m_pGeneralStore->SetWindowField( hTargetWnd, m_flags );

		if ( m_pSpecificStore != nullptr && UseSpecificFlags() )
			if ( *m_pSpecificFlags != m_pSpecificStore->GetWindowField( hTargetWnd ) )
				m_pSpecificStore->SetWindowField( hTargetWnd, *m_pSpecificFlags );
	}
}

void CEditFlagsBasePage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = nullptr == m_totalEdit.m_hWnd;

	DDX_Control( pDX, IDC_TOTAL_FLAGS_EDIT, m_totalEdit );
	DDX_Control( pDX, IDC_UNKNOWN_FLAGS_EDIT, m_unknownEdit );
	m_listSheet.DDX_DetailSheet( pDX, IDC_DETAILS_SHEET );

	if ( firstInit )
	{
		m_pGeneralList = m_listSheet.GetPageAs<CFlagsListPage>( GeneralPage )->GetFlagsCtrl();
		m_pSpecificList = m_listSheet.GetPageAs<CFlagsListPage>( SpecificPage )->GetFlagsCtrl();

		OutputTargetWnd();
	}

	CDetailBasePage::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CEditFlagsBasePage, CDetailBasePage )
	ON_FN_FLAGSCHANGED( IDC_TOTAL_FLAGS_EDIT, OnFnFlagsChanged_TotalEdit )
	ON_FN_FLAGSCHANGED( IDC_UNKNOWN_FLAGS_EDIT, OnFnFlagsChanged_UnknownEdit )
END_MESSAGE_MAP()

void CEditFlagsBasePage::OnFnFlagsChanged_TotalEdit( void )
{
	DWORD newFlags = m_totalEdit.GetFlags();

	int errorIndex = m_pGeneralStore->CheckFlagsTransition( newFlags, m_flags );
	if ( -1 == errorIndex )
		if ( m_pSpecificStore != nullptr && !UseSpecificFlags() )
			errorIndex = m_pSpecificStore->CheckFlagsTransition( newFlags, m_flags );

	if ( errorIndex != -1 )						// error detected -> restore previous style
	{
		ui::BeepSignal();
		m_totalEdit.SetFlags( m_flags );		// rollback user's change
		return;
	}

	m_pGeneralList->SetFlags( newFlags );

	if ( !UseSpecificFlags() )
		m_pSpecificList->SetFlags( newFlags );

	m_unknownEdit.SetFlags( EvalUnknownFlags( newFlags ) );
	InputFlags( &m_flags );

	OnFieldModified();
}

void CEditFlagsBasePage::OnFnFlagsChanged_UnknownEdit( void )
{
	InputFlags( &m_flags );
	m_totalEdit.SetFlags( m_flags );

	OnFieldModified();
}

void CEditFlagsBasePage::OnFnFlagsChanged_GeneralList( void )
{
	InputFlags( &m_flags );
	m_totalEdit.SetFlags( m_flags );

	OnFieldModified();
}

void CEditFlagsBasePage::OnFnFlagsChanged_SpecificList( void )
{
	InputFlags( &m_flags, m_pSpecificFlags.get() );
	if ( !UseSpecificFlags() )
		m_totalEdit.SetFlags( m_flags );

	OnFieldModified();
}

BOOL CEditFlagsBasePage::OnTtnNeedText( UINT cmdId, NMHDR* pNmHdr, LRESULT* pResult )
{
	TOOLTIPTEXT* pTooltipInfo = (TOOLTIPTEXT*)pNmHdr;
	if ( pTooltipInfo->hdr.hwndFrom == m_listSheet.GetTabControl()->GetToolTips()->GetSafeHwnd() )
		switch ( pTooltipInfo->hdr.idFrom )
		{
			case GeneralPage:
			case SpecificPage:
				pTooltipInfo->lpszText = const_cast<TCHAR*>( m_childPageTipText[ pTooltipInfo->hdr.idFrom ].c_str() ); break;
				break;
		}

	return CDetailBasePage::OnTtnNeedText( cmdId, pNmHdr, pResult );
}
