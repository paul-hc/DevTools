#ifndef TandemControls_hxx
#define TandemControls_hxx

#include "CmdInfoStore.h"
#include "CmdUpdate.h"
#include "Dialog_fwd.h"
#include "DialogToolBar.h"
#include "ShellUtilities.h"
#include "resource.h"


// CBaseHostToolbarCtrl template code

template< typename BaseCtrl >
CBaseHostToolbarCtrl<BaseCtrl>::CBaseHostToolbarCtrl( void )
	: BaseCtrl()
	, m_pParentWnd( NULL )
	, m_pMateToolbar( new CDialogToolBar() )
	, m_tandemLayout( ui::EditShinkHost_MateOnRight, Spacing )
	, m_ignoreResize( false )
{
	m_pMateToolbar->SetEnableUnhandledCmds();		// enable detail buttons by default
}

template< typename BaseCtrl >
CBaseHostToolbarCtrl< BaseCtrl >::~CBaseHostToolbarCtrl()
{
}

template< typename BaseCtrl >
void CBaseHostToolbarCtrl<BaseCtrl>::DDX_Tandem( CDataExchange* pDX, int ctrlId, CWnd* pWndTarget /*= NULL*/ )
{
	if ( NULL == m_hWnd )
	{
		ASSERT( DialogOutput == pDX->m_bSaveAndValidate );

		::DDX_Control( pDX, ctrlId, *this );

		if ( pWndTarget != NULL )
			GetMateToolbar()->SetOwner( pWndTarget );		// host control handles WM_COMMAND for editing, and redirects WM_NOTIFY to parent dialog (for tooltips)
	}
}

template< typename BaseCtrl >
inline void CBaseHostToolbarCtrl<BaseCtrl>::ResetMateToolbar( void )
{
	m_pMateToolbar.reset();
}

template< typename BaseCtrl >
inline const std::vector< UINT >& CBaseHostToolbarCtrl<BaseCtrl>::GetMateCommands( void ) const
{
	REQUIRE( HasMateToolbar() );
	return m_pMateToolbar->GetStrip().GetButtonIds();
}

template< typename BaseCtrl >
inline bool CBaseHostToolbarCtrl<BaseCtrl>::ContainsMateCommand( UINT cmdId ) const
{
	return m_pMateToolbar.get() != NULL && m_pMateToolbar->GetStrip().ContainsButton( cmdId );
}

template< typename BaseCtrl >
void CBaseHostToolbarCtrl<BaseCtrl>::LayoutMates( void )
{
	if ( HasMateToolbar() )
		m_tandemLayout.LayoutMate( m_pMateToolbar.get(), this );	// tile decorations toolbar
}

template< typename BaseCtrl >
BOOL CBaseHostToolbarCtrl<BaseCtrl>::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( WM_NOTIFY == HIWORD( code ) )
	{	// give parent dialog a chance to handle tooltip notification messages (since this is the owner of the toolbar)
		static UINT s_tooltipId = 0;		// MFC: the tooltip is a popup window with ID of 0 shared by all windows in a UI thread

		if ( s_tooltipId == id || (UINT)m_pMateToolbar->GetDlgCtrlID() == id )
			if ( m_pParentWnd->OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
				return TRUE;
	}

	return __super::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

template< typename BaseCtrl >
BOOL CBaseHostToolbarCtrl<BaseCtrl>::OnCommand( WPARAM wParam, LPARAM lParam )
{
	UINT cmdId = LOWORD( wParam );
	int notifCode = HIWORD( wParam );

	// Note: detail editing commands are "soft" (toolstrip buttons) and can't be handled via message map (for range IDs) without disrupting all other mapped command handlers in derived classes
	if ( CN_COMMAND == notifCode && OnMateCommand( cmdId ) )
		return TRUE;	// handled

	return __super::OnCommand( wParam, lParam );
}

template< typename BaseCtrl >
void CBaseHostToolbarCtrl<BaseCtrl>::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();
	m_pParentWnd = GetParent();

	m_ignoreResize = true;

	if ( m_pMateToolbar.get() != NULL )
	{
		m_pMateToolbar->CreateTandem( this, m_tandemLayout );
		m_pMateToolbar->SetOwner( this );		// host control handles WM_COMMAND for editing, and redirects WM_NOTIFY to parent dialog (for tooltips)
	}

	m_ignoreResize = false;
}


// message handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseHostToolbarCtrl, BaseCtrl, TBaseClass )
	ON_WM_SIZE()
	ON_WM_INITMENUPOPUP()
END_MESSAGE_MAP()

template< typename BaseCtrl >
void CBaseHostToolbarCtrl<BaseCtrl>::OnSize( UINT sizeType, int cx, int cy )
{
	__super::OnSize( sizeType, cx, cy );

	if ( !m_ignoreResize )
		if ( SIZE_MAXIMIZED == sizeType || SIZE_RESTORED == sizeType )
			LayoutMates();
}

template< typename BaseCtrl >
void CBaseHostToolbarCtrl<BaseCtrl>::OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu )
{
	index;
	if ( !isSysMenu )
		ui::UpdateMenuUI( this, pPopupMenu );		// update tracking menu targeting this control
}

template< typename BaseCtrl >
bool CBaseHostToolbarCtrl<BaseCtrl>::OnMateCommand( UINT cmdId )
{
	if ( ContainsMateCommand( cmdId ) )
		return OnBuddyCommand( cmdId );		// true if handled

	return false;
}


// CHostToolbarCtrl template code

template< typename BaseCtrl >
CHostToolbarCtrl<BaseCtrl>::CHostToolbarCtrl( ui::TTandemAlign tandemAlign /*= ui::EditShinkHost_MateOnRight*/ )
	: CBaseHostToolbarCtrl<BaseCtrl>()
{
	RefTandemLayout().SetTandemAlign( tandemAlign );
}

template< typename BaseCtrl >
bool CHostToolbarCtrl<BaseCtrl>::OnBuddyCommand( UINT cmdId )
{
	cmdId;
	return false;		// continue routing
}

template< typename BaseCtrl >
BOOL CHostToolbarCtrl<BaseCtrl>::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( __super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return true;

	if ( ContainsMateCommand( id ) )		// unhandled buddy command?
		if ( m_pParentWnd->OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
			return TRUE;					// command handled by parent

	return FALSE;
}


// CBaseItemContentCtrl template code

template< typename BaseCtrl >
bool CBaseItemContentCtrl<BaseCtrl>::OnBuddyCommand( UINT cmdId )
{
	cmdId;
	ui::SendCommandToParent( m_hWnd, CN_EDITDETAILS );		// let the parent handle editing details
	return true;		// handled
}

template< typename BaseCtrl >
void CBaseItemContentCtrl<BaseCtrl>::SetContentType( ui::ContentType type )
{
	m_content.m_type = type;

	if ( CDialogToolBar* pMateToolbar = GetMateToolbar() )
		if ( GetMateCommands().empty() )
			switch ( m_content.m_type )
			{
				default: ASSERT( false );
				case ui::String:
					pMateToolbar->GetStrip().AddButton( ID_EDIT_DETAILS );
					break;
				case ui::FilePath:
					pMateToolbar->GetStrip().AddButton( ID_BROWSE_FILE );
					break;
				case ui::DirPath:
					pMateToolbar->GetStrip().AddButton( ID_BROWSE_FOLDER );
					break;
				case ui::MixedPath:
					pMateToolbar->GetStrip()
						.AddButton( ID_BROWSE_FOLDER )
						.AddButton( ID_BROWSE_FILE );
					break;
			}
}

template< typename BaseCtrl >
void CBaseItemContentCtrl<BaseCtrl>::SetFileFilter( const TCHAR* pFileFilter )
{
	m_content.m_pFileFilter = pFileFilter;

	if ( m_content.m_pFileFilter != NULL )
		SetContentType( ui::FilePath );
}

template< typename BaseCtrl >
void CBaseItemContentCtrl<BaseCtrl>::SetStringContent( bool allowEmptyItem /*= true*/, bool noMateButton /*= true*/ )
{
	REQUIRE( NULL == m_hWnd );			// call before creation
	SetFlag( m_content.m_itemsFlags, ui::CItemContent::RemoveEmpty, !allowEmptyItem );

	if ( noMateButton )
		ResetMateToolbar();
}

template< typename BaseCtrl >
void CBaseItemContentCtrl<BaseCtrl>::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();

	DragAcceptFiles( m_content.IsPathContent() );
}


// message handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseItemContentCtrl, BaseCtrl, TBaseClass )
	ON_WM_DROPFILES()
END_MESSAGE_MAP()

template< typename BaseCtrl >
void CBaseItemContentCtrl<BaseCtrl>::OnDropFiles( HDROP hDropInfo )
{
	if ( m_content.IsPathContent() )
	{
		std::vector< fs::CPath > filePaths;
		shell::QueryDroppedFiles( filePaths, hDropInfo );

		if ( !filePaths.empty() )
			OnDroppedFiles( filePaths );
	}
	else
		__super::OnDropFiles( hDropInfo );
}


#endif // TandemControls_hxx
