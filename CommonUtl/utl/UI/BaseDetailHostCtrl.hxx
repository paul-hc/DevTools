#ifndef BaseDetailHostCtrl_hxx
#define BaseDetailHostCtrl_hxx

#include "CmdInfoStore.h"
#include "DialogToolBar.h"
#include "resource.h"


// CBaseDetailHostCtrl template code

template< typename BaseCtrl >
CBaseDetailHostCtrl<BaseCtrl>::CBaseDetailHostCtrl( void )
	: BaseCtrl()
	, m_pParentWnd( NULL )
	, m_pMateToolbar( new CDialogToolBar() )
	, m_tandemLayout( H_AlignRight | V_AlignCenter, Spacing )
	, m_ignoreResize( false )
{
	m_pMateToolbar->SetEnableUnhandledCmds();		// enable detail buttons by default
}

template< typename BaseCtrl >
inline void CBaseDetailHostCtrl<BaseCtrl>::ResetMateToolbar( void )
{
	m_pMateToolbar.reset();
}

template< typename BaseCtrl >
inline const std::vector< UINT >& CBaseDetailHostCtrl<BaseCtrl>::GetMateCommands( void ) const
{
	REQUIRE( HasMateToolbar() );
	return m_pMateToolbar->GetStrip().m_buttonIds;
}

template< typename BaseCtrl >
inline bool CBaseDetailHostCtrl<BaseCtrl>::ContainsMateCommand( UINT cmdId ) const
{
	return m_pMateToolbar.get() != NULL && m_pMateToolbar->GetStrip().ContainsButton( cmdId );
}

template< typename BaseCtrl >
void CBaseDetailHostCtrl<BaseCtrl>::LayoutMates( void )
{
	if ( HasMateToolbar() )
		m_tandemLayout.LayoutMate( m_pMateToolbar.get(), this );	// tile decorations toolbar
}

template< typename BaseCtrl >
BOOL CBaseDetailHostCtrl<BaseCtrl>::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( WM_NOTIFY == HIWORD( code ) )
	{
		static UINT s_tooltipId = 0;		// MFC: the tooltip is a popup window with ID of 0 shared by all windows in a UI thread

		if ( s_tooltipId == id || (UINT)m_pMateToolbar->GetDlgCtrlID() == id )
			if ( m_pParentWnd->OnCmdMsg( id, code, pExtra, pHandlerInfo ) )		// give parent dialog a chance to handle tooltip notification messages (since this is the owner of the toolbar)
				return TRUE;
	}

	return BaseCtrl::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}

template< typename BaseCtrl >
BOOL CBaseDetailHostCtrl<BaseCtrl>::OnCommand( WPARAM wParam, LPARAM lParam )
{
	UINT cmdId = LOWORD( wParam );
	int notifCode = HIWORD( wParam );

	// Note: detail editing commands are "soft" (toolstrip buttons) and can't be handled via message map without disrupting all other mapped command handlers in derived classes
	if ( CN_COMMAND == notifCode && OnMateCommand( cmdId ) )
		return TRUE;	// handled

	return BaseCtrl::OnCommand( wParam, lParam );
}

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseDetailHostCtrl, BaseCtrl, BaseCtrl )
	ON_WM_SIZE()
END_MESSAGE_MAP()

template< typename BaseCtrl >
void CBaseDetailHostCtrl<BaseCtrl>::PreSubclassWindow( void )
{
	BaseCtrl::PreSubclassWindow();
	m_pParentWnd = GetParent();

	m_ignoreResize = true;

	if ( m_pMateToolbar.get() != NULL )
	{
		m_pMateToolbar->CreateTandem( this, m_tandemLayout );
		m_pMateToolbar->SetOwner( this );		// host control handles WM_COMMAND for editing, and redirects WM_NOTIFY to parent dialog (for tooltips)
	}

	m_ignoreResize = false;
}

template< typename BaseCtrl >
void CBaseDetailHostCtrl<BaseCtrl>::OnSize( UINT sizeType, int cx, int cy )
{
	BaseCtrl::OnSize( sizeType, cx, cy );

	if ( !m_ignoreResize )
		if ( SIZE_MAXIMIZED == sizeType || SIZE_RESTORED == sizeType )
			LayoutMates();
}

template< typename BaseCtrl >
bool CBaseDetailHostCtrl<BaseCtrl>::OnMateCommand( UINT cmdId )
{
	if ( ContainsMateCommand( cmdId ) )
		return OnBuddyCommand( cmdId );		// true if handled

	return false;
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
				case ui::DirPath:
					pMateToolbar->GetStrip().AddButton( ID_BROWSE_FOLDER );
					break;
				case ui::FilePath:
					pMateToolbar->GetStrip().AddButton( ID_BROWSE_FILE );
					break;
				case ui::MixedPath:
					pMateToolbar->GetStrip()
						.AddButton( ID_BROWSE_FILE )
						.AddButton( ID_BROWSE_FOLDER );
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


#endif // BaseDetailHostCtrl_hxx
