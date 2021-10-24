#ifndef BaseDetailHostCtrl_hxx
#define BaseDetailHostCtrl_hxx

#include "Utilities.h"
#include "resource.h"


// CBaseDetailHostCtrl template code

BEGIN_TEMPLATE_MESSAGE_MAP( CBaseDetailHostCtrl, BaseCtrl, BaseCtrl )
	ON_WM_SIZE()
END_MESSAGE_MAP()

template< typename BaseCtrl >
void CBaseDetailHostCtrl< BaseCtrl >::PreSubclassWindow( void )
{
	BaseCtrl::PreSubclassWindow();

	if ( m_pDetailButton.get() != NULL )
	{
		m_ignoreResize = true;
		m_pDetailButton->Create( this );
		m_ignoreResize = false;
	}
}

template< typename BaseCtrl >
void CBaseDetailHostCtrl< BaseCtrl >::OnSize( UINT sizeType, int cx, int cy )
{
	BaseCtrl::OnSize( sizeType, cx, cy );

	if ( !m_ignoreResize && m_pDetailButton.get() && m_pDetailButton->m_hWnd != NULL )
		if ( SIZE_MAXIMIZED == sizeType || SIZE_RESTORED == sizeType )
			m_pDetailButton->Layout();
}


// CBaseItemContentCtrl template code

template< typename BaseCtrl >
inline void CBaseItemContentCtrl< BaseCtrl >::OnBuddyCommand( UINT cmdId )
{
	cmdId;
	ui::SendCommandToParent( m_hWnd, CN_EDITDETAILS );		// let the parent handle editing details
}

template< typename BaseCtrl >
void CBaseItemContentCtrl< BaseCtrl >::SetContentType( ui::ContentType type )
{
	m_content.m_type = type;
	if ( UseStockButtonIcon() )
	{
		CDetailButton* pDetailButton = GetDetailButton();
		CIconId iconId = pDetailButton->GetIconId();
		iconId.m_id = GetStockButtonIconId();
		pDetailButton->SetIconId( iconId );
	}
}

template< typename BaseCtrl >
void CBaseItemContentCtrl< BaseCtrl >::SetFileFilter( const TCHAR* pFileFilter )
{
	m_content.m_pFileFilter = pFileFilter;
	if ( m_content.m_pFileFilter != NULL )
		SetContentType( ui::FilePath );
}

template< typename BaseCtrl >
bool CBaseItemContentCtrl< BaseCtrl >::UseStockButtonIcon( void ) const
{
	switch ( GetDetailButton()->GetIconId().m_id )
	{
		case 0:
		case ID_EDIT_DETAILS:
		case ID_EDIT_LIST_ITEMS:
		case ID_BROWSE_FILE:
		case ID_BROWSE_FOLDER:
			return true;
	}
	return false;
}

template< typename BaseCtrl >
UINT CBaseItemContentCtrl< BaseCtrl >::GetStockButtonIconId( void ) const
{
	switch ( m_content.m_type )
	{
		default: ASSERT( false );
		case ui::String:	return ID_EDIT_DETAILS;
		case ui::DirPath:	return ID_BROWSE_FOLDER;
		case ui::FilePath:	return ID_BROWSE_FILE;
		case ui::MixedPath:	return ID_BROWSE_FOLDER;	// +ID_BROWSE_FILE
	}
}


#endif // BaseDetailHostCtrl_hxx
