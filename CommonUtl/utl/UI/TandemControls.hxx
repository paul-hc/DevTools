#ifndef TandemControls_hxx
#define TandemControls_hxx

#include "BaseTrackMenuWnd.hxx"
#include "CmdUpdate.h"
#include "Dialog_fwd.h"
#include "DialogToolBar.h"
#include "ShellUtilities.h"
#include "resource.h"


// CBaseHostToolbarCtrl template code

template< typename BaseCtrlT >
CBaseHostToolbarCtrl<BaseCtrlT>::CBaseHostToolbarCtrl( void )
	: TBaseClass()
	, m_pParentWnd( nullptr )
	, m_pMateToolbar( new CDialogToolBar() )
	, m_tandemLayout( ui::EditShrinkHost_MateOnRight, Spacing )
	, m_ignoreResize( false )
{
	m_pMateToolbar->SetEnableUnhandledCmds();		// enable detail buttons by default
}

template< typename BaseCtrlT >
CBaseHostToolbarCtrl<BaseCtrlT>::~CBaseHostToolbarCtrl()
{
}

template< typename BaseCtrlT >
void CBaseHostToolbarCtrl<BaseCtrlT>::DDX_Tandem( CDataExchange* pDX, int ctrlId, CWnd* pWndTarget /*= nullptr*/ )
{
	if ( nullptr == this->m_hWnd )
	{
		ASSERT( DialogOutput == pDX->m_bSaveAndValidate );

		::DDX_Control( pDX, ctrlId, *this );

		if ( pWndTarget != nullptr )
			GetMateToolbar()->SetOwner( pWndTarget );		// host control handles WM_COMMAND for editing, and redirects WM_NOTIFY to parent dialog (for tooltips)
	}
}

template< typename BaseCtrlT >
inline void CBaseHostToolbarCtrl<BaseCtrlT>::ResetMateToolbar( void )
{
	m_pMateToolbar.reset();
}

template< typename BaseCtrlT >
inline const std::vector<UINT>& CBaseHostToolbarCtrl<BaseCtrlT>::GetMateCommands( void ) const
{
	REQUIRE( HasMateToolbar() );
	return m_pMateToolbar->GetStrip().GetButtonIds();
}

template< typename BaseCtrlT >
inline bool CBaseHostToolbarCtrl<BaseCtrlT>::ContainsMateCommand( UINT cmdId ) const
{
	return m_pMateToolbar.get() != nullptr && m_pMateToolbar->GetStrip().ContainsButton( cmdId );
}

template< typename BaseCtrlT >
void CBaseHostToolbarCtrl<BaseCtrlT>::LayoutMates( void )
{
	if ( HasMateToolbar() )
		m_tandemLayout.LayoutMate( m_pMateToolbar.get(), this );	// tile decorations toolbar
}

template< typename BaseCtrlT >
BOOL CBaseHostToolbarCtrl<BaseCtrlT>::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
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

template< typename BaseCtrlT >
BOOL CBaseHostToolbarCtrl<BaseCtrlT>::OnCommand( WPARAM wParam, LPARAM lParam )
{
	UINT cmdId = LOWORD( wParam );
	int notifCode = HIWORD( wParam );

	// Note: detail editing commands are "soft" (toolstrip buttons) and can't be handled via message map (for range IDs) without disrupting all other mapped command handlers in derived classes
	if ( CN_COMMAND == notifCode && OnMateCommand( cmdId ) )
		return TRUE;	// handled

	return __super::OnCommand( wParam, lParam );
}

template< typename BaseCtrlT >
void CBaseHostToolbarCtrl<BaseCtrlT>::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();
	m_pParentWnd = this->GetParent();

	m_ignoreResize = true;

	if ( m_pMateToolbar.get() != nullptr )
	{
		m_pMateToolbar->CreateTandem( this, m_tandemLayout );
		m_pMateToolbar->SetOwner( this );		// host control handles WM_COMMAND for editing, and redirects WM_NOTIFY to parent dialog (for tooltips)
	}

	m_ignoreResize = false;
}


// message handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseHostToolbarCtrl, BaseCtrlT, TBaseClass )
	//ON_WM_SIZE()		// read the OnSize() note below
	ON_WM_WINDOWPOSCHANGED()
END_MESSAGE_MAP()

template< typename BaseCtrlT >
void CBaseHostToolbarCtrl<BaseCtrlT>::OnSize( UINT sizeType, int cx, int cy )
{
	// Avoid using ON_WM_SIZE(), since in layout dialogs can be laggy and leave trails of the mate control.
	// Better handle the ON_WM_WINDOWPOSCHANGED() event, as the mate layout works best!
	//
	__super::OnSize( sizeType, cx, cy );

	if ( !m_ignoreResize )
		if ( SIZE_MAXIMIZED == sizeType || SIZE_RESTORED == sizeType )
			LayoutMates();
}

template< typename BaseCtrlT >
void CBaseHostToolbarCtrl<BaseCtrlT>::OnWindowPosChanged( WINDOWPOS* pWndPos )
{
	__super::OnWindowPosChanged( pWndPos );

	if ( !m_ignoreResize )
		LayoutMates();
}

template< typename BaseCtrlT >
bool CBaseHostToolbarCtrl<BaseCtrlT>::OnMateCommand( UINT cmdId )
{
	if ( ContainsMateCommand( cmdId ) )
		return OnBuddyCommand( cmdId );		// true if handled

	return false;
}


// CHostToolbarCtrl template code

template< typename BaseCtrlT >
CHostToolbarCtrl<BaseCtrlT>::CHostToolbarCtrl( ui::TTandemAlign tandemAlign /*= ui::EditShrinkHost_MateOnRight*/ )
	: CBaseHostToolbarCtrl<BaseCtrlT>()
{
	this->RefTandemLayout().SetTandemAlign( tandemAlign );
}

template< typename BaseCtrlT >
bool CHostToolbarCtrl<BaseCtrlT>::OnBuddyCommand( UINT cmdId )
{
	cmdId;
	return false;		// continue routing
}

template< typename BaseCtrlT >
BOOL CHostToolbarCtrl<BaseCtrlT>::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( __super::OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return true;

	if ( this->ContainsMateCommand( id ) )		// unhandled buddy command?
		if ( this->m_pParentWnd->OnCmdMsg( id, code, pExtra, pHandlerInfo ) )
			return TRUE;					// command handled by parent

	return FALSE;
}


// CBaseItemContentCtrl template code

template< typename BaseCtrlT >
bool CBaseItemContentCtrl<BaseCtrlT>::OnBuddyCommand( UINT cmdId )
{
	cmdId;
	ui::SendCommandToParent( this->m_hWnd, CN_EDITDETAILS );		// let the parent handle editing details
	return true;		// handled
}

template< typename BaseCtrlT >
void CBaseItemContentCtrl<BaseCtrlT>::SetContentType( ui::ContentType type )
{
	m_content.m_type = type;

	if ( CDialogToolBar* pMateToolbar = this->GetMateToolbar() )
		if ( this->GetMateCommands().empty() )
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

template< typename BaseCtrlT >
void CBaseItemContentCtrl<BaseCtrlT>::SetFileFilter( const TCHAR* pFileFilter )
{
	m_content.m_pFileFilter = pFileFilter;

	if ( m_content.m_pFileFilter != nullptr )
		SetContentType( ui::FilePath );
}

template< typename BaseCtrlT >
void CBaseItemContentCtrl<BaseCtrlT>::SetStringContent( bool allowEmptyItem /*= true*/, bool noMateButton /*= true*/ )
{
	REQUIRE( nullptr == this->m_hWnd );			// call before creation
	SetFlag( m_content.m_itemsFlags, ui::CItemContent::RemoveEmpty, !allowEmptyItem );

	if ( noMateButton )
		this->ResetMateToolbar();
}

template< typename BaseCtrlT >
void CBaseItemContentCtrl<BaseCtrlT>::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();

	this->DragAcceptFiles( m_content.IsPathContent() );
}


// message handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseItemContentCtrl, BaseCtrlT, TBaseClass )
	ON_WM_DROPFILES()
END_MESSAGE_MAP()

template< typename BaseCtrlT >
void CBaseItemContentCtrl<BaseCtrlT>::OnDropFiles( HDROP hDropInfo )
{
	if ( m_content.IsPathContent() )
	{
		std::vector<fs::CPath> filePaths;
		shell::QueryDroppedFiles( filePaths, hDropInfo );

		if ( !filePaths.empty() )
			OnDroppedFiles( filePaths );
	}
	else
		__super::OnDropFiles( hDropInfo );
}


#endif // TandemControls_hxx
