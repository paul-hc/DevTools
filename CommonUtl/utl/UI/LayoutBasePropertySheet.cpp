
#include "stdafx.h"
#include "LayoutBasePropertySheet.h"
#include "LayoutPropertyPage.h"
#include "CmdInfoStore.h"
#include "Command.h"
#include "CommandModel.h"
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


class CScopedApplyMacroCmd
{
public:
	CScopedApplyMacroCmd( CLayoutBasePropertySheet* pSheet ) : m_pSheet( pSheet ) { m_pSheet->m_pApplyMacroCmd.reset( new CMacroCommand() ); }
	~CScopedApplyMacroCmd() { m_pSheet->m_pApplyMacroCmd.reset(); }
private:
	CLayoutBasePropertySheet* m_pSheet;
};


namespace reg
{
	static const TCHAR entry_activePage[] = _T("ActivePage");
}


CLayoutBasePropertySheet::CLayoutBasePropertySheet( const TCHAR* pTitle, CWnd* pParent, UINT selPageIndex )
	: CPropertySheet( pTitle, pParent, selPageIndex )
	, m_initialPageIndex( UINT_MAX )
	, m_pCommandExecutor( nullptr )
	, m_manageOkButtonState( false )
{
}

CLayoutBasePropertySheet::~CLayoutBasePropertySheet()
{
}

CLayoutPropertyPage* CLayoutBasePropertySheet::GetPage( int pageIndex ) const
{
	return checked_static_cast<CLayoutPropertyPage*>( __super::GetPage( pageIndex ) );
}

CLayoutPropertyPage* CLayoutBasePropertySheet::GetActivePage( void ) const
{
	if ( 0 == GetPageCount() )
		return nullptr;

	return checked_static_cast<CLayoutPropertyPage*>( __super::GetActivePage() );
}

std::tstring CLayoutBasePropertySheet::GetPageTitle( int pageIndex ) const
{
	CTabCtrl* pTabCtrl = GetTabControl();
	ASSERT_PTR( pTabCtrl );
	ASSERT( pageIndex < GetPageCount() );

	std::tstring pageTitle;
	TCHAR text[ 256 ] = _T("");
	TCITEM tcItem;
	tcItem.mask = TCIF_TEXT;
	tcItem.pszText = text;
	tcItem.cchTextMax = COUNT_OF( text );

	if ( pTabCtrl->GetItem( pageIndex, &tcItem ) )
		pageTitle = tcItem.pszText;

	return pageTitle;
}

void CLayoutBasePropertySheet::SetPageTitle( int pageIndex, const std::tstring& pageTitle )
{
	CTabCtrl* pTabCtrl = GetTabControl();
	ASSERT_PTR( pTabCtrl );
	ASSERT( pageIndex < GetPageCount() );

	TCHAR textBuffer[ 256 ];
	TCITEM tabItem;
	tabItem.mask = TCIF_TEXT;
	tabItem.pszText = textBuffer;
	tabItem.cchTextMax = COUNT_OF( textBuffer );
	VERIFY( pTabCtrl->GetItem( pageIndex, &tabItem ) );

	if ( pageTitle != textBuffer )
	{
		tabItem.pszText = const_cast<TCHAR*>( pageTitle.c_str() );
		VERIFY( pTabCtrl->SetItem( pageIndex, &tabItem ) );				// update tab item caption

		if ( HasFlag( pTabCtrl->GetStyle(), TCS_MULTILINE ) )
			LayoutSheet();
	}

	GetPage( pageIndex )->SetTitle( pageTitle );	// update dialog caption
}

bool CLayoutBasePropertySheet::AnyPageResizable( void ) const
{
	for ( size_t i = 0, count = m_pages.GetSize(); i != count; ++i )
		if ( IsPageResizable( GetPage( (int)i ) ) )
			return true;

	return false;
}

bool CLayoutBasePropertySheet::IsPageResizable( const CLayoutPropertyPage* pPage ) const
{
	return pPage->HasControlLayout();
}

void CLayoutBasePropertySheet::RemoveAllPages( void )
{
	for ( int i = GetPageCount(); i-- != 0; )
	{
		delete GetPage( i );
		RemovePage( i );
	}
}

void CLayoutBasePropertySheet::DeleteAllPages( void )
{
	for ( int i = GetPageCount(); i-- != 0; )
		delete GetPage( i );
}

void CLayoutBasePropertySheet::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	if ( pTooltip != nullptr && m_pTooltipCtrl.get() == pTooltip )
	{
		UINT pageIndex = cmdId - TabItem_BaseToolId;				// tool id: subtract TabItem_BaseToolId

		if ( pageIndex < (UINT)GetPageCount() )
		{
			CLayoutPropertyPage* pPage = GetPage( pageIndex );
			pPage->QueryTooltipText( rText, pPage->GetTemplateId(), pTooltip );

			if ( rText.empty() )
			{
				ui::CCmdInfo cmdInfo( pPage->GetTemplateId() );		// use the page template resource string
				if ( cmdInfo.IsValid() )
					rText = cmdInfo.m_tooltipText;
			}
		}
	}
}

void CLayoutBasePropertySheet::RegisterTabTooltips( void )
{
	if ( nullptr == GetSheetTooltip() )
		return;

	CTabCtrl* pTabCtrl = GetTabControl();
	ASSERT( pTabCtrl );

	// register each tab item rect as a tool, using page template id

	for ( int i = 0, count = pTabCtrl->GetItemCount(); i != count; ++i )
	{
		CRect itemRect;
		pTabCtrl->GetItemRect( i, &itemRect );
		m_pTooltipCtrl->AddTool( pTabCtrl, LPSTR_TEXTCALLBACK, &itemRect, i + TabItem_BaseToolId );		// tool id: add TabItem_BaseToolId to prevent assertion failures
	}
}

void CLayoutBasePropertySheet::LoadFromRegistry( void )
{
	if ( m_initialPageIndex != UINT_MAX )
		m_psh.nStartPage = m_initialPageIndex;
	else if ( !m_regSection.empty() )
		m_psh.nStartPage = AfxGetApp()->GetProfileInt( m_regSection.c_str(), reg::entry_activePage, m_psh.nStartPage );
}

void CLayoutBasePropertySheet::SaveToRegistry( void )
{
	if ( !m_regSection.empty() )
		AfxGetApp()->WriteProfileInt( m_regSection.c_str(), reg::entry_activePage, GetActiveIndex() );
}

void CLayoutBasePropertySheet::LayoutSheet( void )
{
	if ( !IsIconic() )
		if ( CTabCtrl* pTabCtrl = GetTabControl() )
		{
			CRect tabRect;
			pTabCtrl->GetWindowRect( &tabRect );
			ScreenToClient( &tabRect );

			CRect pageRect = tabRect;
			pTabCtrl->AdjustRect( FALSE, &pageRect );
			--pageRect.left;
			++pageRect.top;

			LayoutPages( pageRect );
		}
}

void CLayoutBasePropertySheet::LayoutPages( const CRect& rPageRect )
{
	for ( int i = 0, count = GetPageCount(); i != count; ++i )
	{
		CLayoutPropertyPage* pPage = GetPage( i );

		if ( pPage->m_hWnd != nullptr )
		{
			pPage->MoveWindow( rPageRect );
			pPage->LayoutPage();
		}
	}

	if ( CTabCtrl* pTabCtrl = GetTabControl() )
		if ( HasFlag( pTabCtrl->GetStyle(), TCS_MULTILINE ) )
			RegisterTabTooltips();								// tab items may get repositioned on different lines
}

void CLayoutBasePropertySheet::BuildPropPageArray( void ) override
{
	LoadFromRegistry();				// before window creation
	__super::BuildPropPageArray();
}

bool CLayoutBasePropertySheet::IsSheetModified( void ) const
{
	if ( nullptr == m_hWnd )
		return false;				// window not yet created

	ui::TriggerInput( m_hWnd );		// trigger input if a modified edit is focused, i.e. uncommited

	// either ID_APPLY_NOW or IDOK button indicates the sheet is modified
	CWnd* pModifiedButton = GetSheetButton( ID_APPLY_NOW );
	if ( nullptr == pModifiedButton )
		pModifiedButton = GetSheetButton( IDOK );

	if ( pModifiedButton != nullptr )
		return !HasFlag( pModifiedButton->GetStyle(), WS_DISABLED );

	return true;		// assume modified since we cannot make a guess
}

void CLayoutBasePropertySheet::SetSheetModified( bool modified /*= true*/ )
{
	ASSERT_PTR( m_hWnd );

	if ( CWnd* pApplyNowButton = GetSheetButton( ID_APPLY_NOW ) )
		ui::EnableWindow( *pApplyNowButton, modified );

	if ( m_manageOkButtonState )
		if ( CWnd* pOkButton = GetSheetButton( IDOK ) )
			ui::EnableWindow( *pOkButton, modified );
}

CButton* CLayoutBasePropertySheet::GetSheetButton( UINT buttonId ) const
{
	return (CButton*)GetDlgItem( buttonId );
}

void CLayoutBasePropertySheet::SetCreateAllPages( void )
{	// create all pages when sheet is created
	ASSERT( GetPageCount() != 0 );			// pages must've been inserted

	for ( int i = 0, count = GetPageCount(); i != count; ++i )
		GetPage( i )->SetPremature();
}

void CLayoutBasePropertySheet::SetSingleLineTab( bool singleLineTab /*= true*/ )
{
	if ( singleLineTab )
		GetTabControl()->ModifyStyle( TCS_MULTILINE, TCS_SINGLELINE );
	else
		GetTabControl()->ModifyStyle( TCS_SINGLELINE, TCS_MULTILINE );
}

void CLayoutBasePropertySheet::SetPagesUseLazyUpdateData( bool useLazyUpdateData /*= true*/ )
{
	ASSERT( GetPageCount() != 0 );			// pages must've been inserted

	for ( int i = 0, count = GetPageCount(); i != count; ++i )
		GetPage( i )->SetUseLazyUpdateData( useLazyUpdateData );
}

bool CLayoutBasePropertySheet::OutputPage( int pageIndex )
{
	CLayoutPropertyPage* pPage = GetPage( pageIndex );
	return pPage->GetSafeHwnd() != nullptr && pPage->UpdateData( DialogOutput ) != FALSE;
}

void CLayoutBasePropertySheet::OutputPages( void )
{
	for ( int i = 0, count = GetPageCount(); i != count; ++i )
		OutputPage( i );
}


bool CLayoutBasePropertySheet::ApplyChanges( void )
{
	if ( !IsSheetModified() )
		return true;

	CScopedApplyMacroCmd scopedApplyMacro( this );		// pages can get access through GetApplyMacroCmd()

	for ( int i = 0, count = GetPageCount(); i != count; ++i )
	{
		CLayoutPropertyPage* pPage = GetPage( i );

		if ( pPage->GetSafeHwnd() != nullptr )
		{
			if ( !pPage->UpdateData( DialogSaveChanges ) )
				return false;

			try
			{
				pPage->ApplyPageChanges();
				pPage->SetModified( false );
			}
			catch ( const CPageValidationException& pageExc )
			{
				pageExc.FocusErrorCtrl();
				ui::ReportException( pageExc );		// activates the source of error (page and control)
				return false;
			}
			catch ( const CRuntimeException& e )
			{
				if ( GetActivePage() != pPage )
					SetActivePage( pPage );
				ui::ReportException( e );
				return false;
			}
		}
	}

	if ( m_pCommandExecutor != nullptr )
		if ( m_pApplyMacroCmd.get() != nullptr && !m_pApplyMacroCmd->IsEmpty() )
			m_pCommandExecutor->Execute( m_pApplyMacroCmd.release() );

	OnChangesApplied();
	return true;
}

void CLayoutBasePropertySheet::OnChangesApplied( void )
{
}

BOOL CLayoutBasePropertySheet::PreTranslateMessage( MSG* pMsg ) override
{
	if ( GetSheetTooltip() != nullptr )
		m_pTooltipCtrl->RelayEvent( pMsg );

	return __super::PreTranslateMessage( pMsg );
}


// message handlers

BEGIN_MESSAGE_MAP( CLayoutBasePropertySheet, CPropertySheet )
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTW, ui::MinCmdId, ui::MaxCmdId, OnTtnNeedText )
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTA, ui::MinCmdId, ui::MaxCmdId, OnTtnNeedText )
END_MESSAGE_MAP()

BOOL CLayoutBasePropertySheet::OnInitDialog( void ) override
{
	BOOL result = __super::OnInitDialog();

	m_titleCode = m_strCaption.GetString();

	CTabCtrl* pTabCtrl = GetTabControl();
	ASSERT_PTR( pTabCtrl->GetSafeHwnd() );

	m_pTooltipCtrl.reset( new CToolTipCtrl() );
	if ( !m_pTooltipCtrl->Create( this ) )
		m_pTooltipCtrl.reset();

	pTabCtrl->SetToolTips( m_pTooltipCtrl.get() );
	RegisterTabTooltips();
	m_pTooltipCtrl->Activate( TRUE );

	if ( m_tabImageList.GetSafeHandle() != nullptr )
	{
		ASSERT( m_tabImageList.GetImageCount() >= GetPageCount() );
		pTabCtrl->SetImageList( &m_tabImageList );

		for ( int i = 0, count = GetPageCount(); i != count; ++i )
		{
			TC_ITEM tabItem;
			tabItem.mask = TCIF_IMAGE;
			tabItem.iImage = i;
			VERIFY( pTabCtrl->SetItem( i, &tabItem ) );
		}
	}

	return result;
}

void CLayoutBasePropertySheet::OnDestroy( void )
{
	if ( CTabCtrl* pTabCtrl = GetTabControl() )
		pTabCtrl->SetImageList( nullptr );				// release image list ownership

	SaveToRegistry();
	__super::OnDestroy();
}

void CLayoutBasePropertySheet::OnSize( UINT sizeType, int cx, int cy )
{
	__super::OnSize( sizeType, cx, cy );

	if ( sizeType != SIZE_MINIMIZED )
		LayoutSheet();
}

BOOL CLayoutBasePropertySheet::OnTtnNeedText( UINT cmdId, NMHDR* pNmHdr, LRESULT* pResult )
{
	cmdId;
	if ( ui::CCmdInfoStore::Instance().HandleTooltipNeedText( pNmHdr, pResult, this ) )
		return TRUE;		// handled

	return Default() != 0;
}
