
#include "stdafx.h"
#include "LayoutPropertyPage.h"
#include "LayoutBasePropertySheet.h"
#include "LayoutEngine.h"
#include "CmdInfoStore.h"
#include "CmdUpdate.h"
#include "PostCall.h"
#include "WndUtils.h"
#include <afxpriv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CLayoutPropertyPage::CLayoutPropertyPage( UINT templateId, UINT titleId /*= 0*/ )
	: CPropertyPage( templateId, titleId )
	, m_templateId( templateId )
	, m_pLayoutEngine( new CLayoutEngine() )
	, m_useLazyUpdateData( false )				// don't output on each page activation - contrary to CPropertyPage default behaviour
	, m_hCtrlFocus( NULL )
	, m_idleUpdateDeep( false )
{
	m_psp.dwFlags &= ~PSP_HASHELP;				// don't show the help button by default
}

CLayoutPropertyPage::~CLayoutPropertyPage()
{
}

CLayoutEngine& CLayoutPropertyPage::GetLayoutEngine( void )
{
	return *m_pLayoutEngine;
}

void CLayoutPropertyPage::RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count )
{
	m_pLayoutEngine->RegisterCtrlLayout( layoutStyles, count );
}

bool CLayoutPropertyPage::HasControlLayout( void ) const
{
	return m_pLayoutEngine->HasCtrlLayout();
}

void CLayoutPropertyPage::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	rText, cmdId, pTooltip;
}

CLayoutBasePropertySheet* CLayoutPropertyPage::GetParentSheet( void ) const
{
	CLayoutBasePropertySheet* pParentSheet = checked_static_cast<CLayoutBasePropertySheet*>( GetParent() );
	ASSERT_PTR( pParentSheet );
	return pParentSheet;
}

CWnd* CLayoutPropertyPage::GetParentOwner( void ) const
{
	CLayoutBasePropertySheet* pSheet = GetParentSheet();
	return HasFlag( pSheet->GetStyle(), WS_CHILD ) ? pSheet->GetParent() : pSheet;
}

CMacroCommand* CLayoutPropertyPage::GetApplyMacroCmd( void ) const
{
	return GetParentSheet()->GetApplyMacroCmd();
}

void CLayoutPropertyPage::SetTitle( const std::tstring& pageTitle )
{
	if ( m_hWnd != NULL )
		ui::SetWindowText( m_hWnd, pageTitle );
	else
	{
		m_strCaption = pageTitle.c_str();
		m_psp.pszTitle = m_strCaption;
		m_psp.dwFlags |= PSP_USETITLE;
	}
}

void CLayoutPropertyPage::ApplyPageChanges( void ) throws_( CRuntimeException )
{	// nothing to do at this level, must be overriden by concrete page classes
}

bool CLayoutPropertyPage::LayoutPage( void )
{
	return m_pLayoutEngine->IsInitialized() && m_pLayoutEngine->LayoutControls();
}

void CLayoutPropertyPage::SetModified( bool changed )
{
	__super::SetModified( changed );
}

bool CLayoutPropertyPage::StoreFocusControl( void )
{
	HWND hCtrlFocus = ::GetFocus();
	if ( hCtrlFocus != NULL && ::IsChild( m_hWnd, hCtrlFocus ) )
		m_hCtrlFocus = hCtrlFocus;
	else
		m_hCtrlFocus = NULL;

	return m_hCtrlFocus != NULL;
}

bool CLayoutPropertyPage::RestoreFocusControl( void )
{
	if ( NULL == m_hCtrlFocus || !::IsWindow( m_hCtrlFocus ) )
		return false;

	::SetFocus( m_hCtrlFocus );
	return true;
}

void CLayoutPropertyPage::OnIdleUpdateControls( void )
{
	SendMessageToDescendants( WM_IDLEUPDATECMDUI, (WPARAM)TRUE, 0, m_idleUpdateDeep, TRUE );		// update dialog toolbars
}

void CLayoutPropertyPage::DoDataExchange( CDataExchange* pDX )
{
	__super::DoDataExchange( pDX );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( !m_pLayoutEngine->IsInitialized() )
		{
			m_pLayoutEngine->Initialize( this );
			m_pLayoutEngine->LayoutControls();			// initial layout
			EnableToolTips( TRUE );
		}
	}
}

BOOL CLayoutPropertyPage::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return __super::OnCmdMsg( id, code, pExtra, pHandlerInfo );
//	return CWnd::OnCmdMsg( id, code, pExtra, pHandlerInfo );		// skip base CDialog::OnCmdMsg() to avoid infinite recursion
}

BOOL CLayoutPropertyPage::OnSetActive( void )
{
	ui::PostCall( this, &CLayoutPropertyPage::RestoreFocusControl );			// delayed focus so that it overrides page default behaviour: resetting the focus to first control in the page

	if ( m_useLazyUpdateData )
		return __super::OnSetActive();

	return TRUE;
}

BOOL CLayoutPropertyPage::OnKillActive( void )
{
	StoreFocusControl();

	if ( m_useLazyUpdateData )
		return __super::OnKillActive();

	return TRUE;
}

BOOL CLayoutPropertyPage::PreTranslateMessage( MSG* pMsg )
{
	return
		m_accelPool.TranslateAccels( pMsg, m_hWnd ) ||
		__super::PreTranslateMessage( pMsg );
}


// message handlers

BEGIN_MESSAGE_MAP( CLayoutPropertyPage, CPropertyPage )
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_GETMINMAXINFO()
	ON_WM_ERASEBKGND()
	ON_WM_INITMENUPOPUP()
	ON_MESSAGE( WM_KICKIDLE, OnKickIdle )
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTW, ui::MinCmdId, ui::MaxCmdId, OnTtnNeedText )
	ON_NOTIFY_EX_RANGE( TTN_NEEDTEXTA, ui::MinCmdId, ui::MaxCmdId, OnTtnNeedText )
END_MESSAGE_MAP()

int CLayoutPropertyPage::OnCreate( CREATESTRUCT* pCreateStruct )
{
	if ( -1 == __super::OnCreate( pCreateStruct ) )
		return -1;

	ASSERT( !m_pLayoutEngine->IsInitialized() );
	m_pLayoutEngine->StoreInitialSize( this );

	if ( utl::IObserver* pObserverPage = dynamic_cast<utl::IObserver*>( this ) )
		GetParentSheet()->AddObserver( pObserverPage );

	return 0;
}

void CLayoutPropertyPage::OnDestroy( void )
{
	if ( utl::IObserver* pObserverPage = dynamic_cast<utl::IObserver*>( this ) )
		GetParentSheet()->RemoveObserver( pObserverPage );

	__super::OnDestroy();
}

void CLayoutPropertyPage::OnSize( UINT sizeType, int cx, int cy )
{
	__super::OnSize( sizeType, cx, cy );

	if ( sizeType == SIZE_MAXIMIZED || sizeType == SIZE_RESTORED )
		LayoutPage();
}

void CLayoutPropertyPage::OnGetMinMaxInfo( MINMAXINFO* pMinMaxInfo )
{
	__super::OnGetMinMaxInfo( pMinMaxInfo );
	m_pLayoutEngine->HandleGetMinMaxInfo( pMinMaxInfo );
}

BOOL CLayoutPropertyPage::OnEraseBkgnd( CDC* pDC )
{
	return
		m_pLayoutEngine->HandleEraseBkgnd( pDC ) ||
		__super::OnEraseBkgnd( pDC );
}

void CLayoutPropertyPage::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	AfxCancelModes( m_hWnd );
	if ( !isSysMenu )
		ui::UpdateMenuUI( this, pPopupMenu );

	__super::OnInitMenuPopup( pPopupMenu, index, isSysMenu );
}

LRESULT CLayoutPropertyPage::OnKickIdle( WPARAM, LPARAM )
{
	OnIdleUpdateControls();
	return Default();
}

BOOL CLayoutPropertyPage::OnTtnNeedText( UINT cmdId, NMHDR* pNmHdr, LRESULT* pResult )
{
	cmdId;
	if ( ui::CCmdInfoStore::Instance().HandleTooltipNeedText( pNmHdr, pResult, this ) )
		return TRUE;		// handled

	return Default() != 0;
}

void CLayoutPropertyPage::OnFieldModified( void )
{
	SetModified( true );
}

void CLayoutPropertyPage::OnFieldModified( UINT ctrlId )
{
	ctrlId;
	OnFieldModified();
}


// CPageValidationException implementation

CPageValidationException::CPageValidationException( const std::tstring& message, CWnd* pErrorCtrl /*= NULL*/ )
	: CRuntimeException( message )
	, m_pErrorCtrl( pErrorCtrl )
{
}

CPageValidationException::~CPageValidationException() throw()
{
}

bool CPageValidationException::FocusErrorCtrl( void ) const
{
	if ( IsWindow( m_pErrorCtrl->GetSafeHwnd() ) )
	{
		CLayoutPropertyPage* pParentPage = checked_static_cast<CLayoutPropertyPage*>( m_pErrorCtrl->GetParent() );
		ASSERT_PTR( pParentPage );

		CLayoutBasePropertySheet* pSheet = pParentPage->GetParentSheet();
		ASSERT_PTR( pSheet );

		if ( pSheet->GetActivePage() != pParentPage )
			pSheet->SetActivePage( pParentPage );

		ui::TakeFocus( *m_pErrorCtrl );
		return true;
	}

	return false;
}
