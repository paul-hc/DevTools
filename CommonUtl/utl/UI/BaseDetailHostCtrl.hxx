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
	, m_pDetailToolbar( new CDialogToolBar() )
	, m_buddyLayout( H_AlignRight | V_AlignCenter, Spacing )
	, m_ignoreResize( false )
{
}

template< typename BaseCtrl >
inline void CBaseDetailHostCtrl< BaseCtrl >::SetDetailToolbar( CDialogToolBar* pDetailToolbar )
{
	m_pDetailToolbar.reset( pDetailToolbar );
}

template< typename BaseCtrl >
inline const std::vector< UINT >& CBaseDetailHostCtrl< BaseCtrl >::GetDetailCommands( void ) const
{
	ASSERT( HasDetailToolbar() );
	return m_pDetailToolbar->GetStrip().m_buttonIds;
}

template< typename BaseCtrl >
inline bool CBaseDetailHostCtrl< BaseCtrl >::ContainsDetailCommand( UINT cmdId ) const
{
	return m_pDetailToolbar.get() != NULL && m_pDetailToolbar->GetStrip().ContainsButton( cmdId );
}

template< typename BaseCtrl >
void CBaseDetailHostCtrl<BaseCtrl>::LayoutDetails( void )
{
	if ( HasDetailToolbar() )
		m_buddyLayout.LayoutCtrl( m_pDetailToolbar.get(), this );	// tile decorations toolbar
}

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseDetailHostCtrl, BaseCtrl, BaseCtrl )
	ON_WM_SIZE()
	ON_COMMAND_RANGE( MinCmdId, MaxCmdId, OnDetailCommand )
	ON_UPDATE_COMMAND_UI_RANGE( MinCmdId, MaxCmdId, OnUpdateDetailCommand )
END_MESSAGE_MAP()

template< typename BaseCtrl >
void CBaseDetailHostCtrl<BaseCtrl>::PreSubclassWindow( void )
{
	BaseCtrl::PreSubclassWindow();
	m_pParentWnd = GetParent();

	m_ignoreResize = true;

	if ( m_pDetailToolbar.get() != NULL )
	{
		m_pDetailToolbar->CreateShrinkBuddy( this, m_buddyLayout );
		m_pDetailToolbar->SetOwner( this );		// host control handles WM_COMMAND for editing, and redirects WM_NOTIFY to parent dialog (for tooltips)
	}

	m_ignoreResize = false;
}

template< typename BaseCtrl >
void CBaseDetailHostCtrl<BaseCtrl>::OnSize( UINT sizeType, int cx, int cy )
{
	BaseCtrl::OnSize( sizeType, cx, cy );

	if ( !m_ignoreResize )
		if ( SIZE_MAXIMIZED == sizeType || SIZE_RESTORED == sizeType )
			LayoutDetails();
}

template< typename BaseCtrl >
void CBaseDetailHostCtrl<BaseCtrl>::OnDetailCommand( UINT cmdId )
{
	OnBuddyCommand( cmdId );
}

template< typename BaseCtrl >
void CBaseDetailHostCtrl<BaseCtrl>::OnUpdateDetailCommand( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( ContainsDetailCommand( pCmdUI->m_nID ) );
}

template< typename BaseCtrl >
BOOL CBaseDetailHostCtrl<BaseCtrl>::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	return
		m_pParentWnd->OnCmdMsg( id, code, pExtra, pHandlerInfo ) ||		// give parent dialog a chance to handle TTN_NEEDTEXTA/TTN_NEEDTEXTW notification messages (since this is the owner of the toolbar)
		BaseCtrl::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}


// CBaseItemContentCtrl template code

template< typename BaseCtrl >
inline void CBaseItemContentCtrl<BaseCtrl>::OnBuddyCommand( UINT cmdId )
{
	cmdId;
	ui::SendCommandToParent( m_hWnd, CN_EDITDETAILS );		// let the parent handle editing details
}

template< typename BaseCtrl >
void CBaseItemContentCtrl<BaseCtrl>::SetContentType( ui::ContentType type )
{
	m_content.m_type = type;

	if ( CDialogToolBar* pDetailToolbar = GetDetailToolbar() )
		if ( GetDetailCommands().empty() )
		{
			switch ( m_content.m_type )
			{
				default: ASSERT( false );
				case ui::String:
					pDetailToolbar->GetStrip().AddButton( ID_EDIT_DETAILS );
					break;
				case ui::DirPath:
					pDetailToolbar->GetStrip().AddButton( ID_BROWSE_FOLDER );
					break;
				case ui::FilePath:
					pDetailToolbar->GetStrip().AddButton( ID_BROWSE_FILE );
					break;
				case ui::MixedPath:
					pDetailToolbar->GetStrip()
						.AddButton( ID_BROWSE_FILE )
						.AddButton( ID_BROWSE_FOLDER );
					break;
			}
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
void CBaseItemContentCtrl<BaseCtrl>::SetStringContent( bool allowEmptyItem /*= true*/, bool noDetailsButton /*= true*/ )
{
	REQUIRE( NULL == m_hWnd );			// call before creation
	SetFlag( m_content.m_itemsFlags, ui::CItemContent::RemoveEmpty, !allowEmptyItem );

	if ( noDetailsButton )
		SetDetailToolbar( NULL );
}


#endif // BaseDetailHostCtrl_hxx
