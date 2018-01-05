
#include "stdafx.h"
#include "MenuUtilitiesOLD.h"
#include "IdeUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace menu
{


void addSubPopups( std::vector< HMENU >& rOutPopups, HMENU hMenu )
{
	ASSERT( hMenu != NULL );

	rOutPopups.push_back( hMenu );

	for ( int i = 0, count = ::GetMenuItemCount( hMenu ); i != count; ++i )
	{
		HMENU hSubMenu =::GetSubMenu( hMenu, i );

		if ( hSubMenu != NULL )
			addSubPopups( rOutPopups, hSubMenu );
	}
}

UINT getItemFromPosID( MenuItem& rOutFoundItem, const CPoint& screenPos, const std::vector< HMENU >& rPopups )
{
	HWND hWndMenu = getMenuWindowFromPoint( screenPos );

	if ( hWndMenu != NULL )
		for ( unsigned int i = 0; i != rPopups.size(); ++i )
		{
			int itemIndex =::MenuItemFromPoint( hWndMenu, rPopups[ i ], screenPos );

			if ( itemIndex != -1 )
				if ( rOutFoundItem.build( rPopups[ i ], itemIndex ) )
					return rOutFoundItem.ID;
		}

	return 0;
}

HWND getMenuWindowFromPoint( CPoint screenPos /*= CPoint( -1, -1 )*/ )
{
	if ( screenPos.x == -1 || screenPos.y == -1 )
		::GetCursorPos( &screenPos );

	HWND hWndMenu = WindowFromPoint( screenPos );

	if ( hWndMenu == NULL )
		return NULL;

	if ( ide::getWindowClassName( hWndMenu ) != _T("#32768") )
		return NULL;

	return hWndMenu;
}


} // namespace menu


/**
	MenuItem implementation
*/

#define BAD_STATE 0xFFFFFFFF


MenuItem::MenuItem( void )
	: hPopup( NULL )
	, index( -1 )
	, ID( 0 )
	, state( BAD_STATE )
{
}

MenuItem::~MenuItem()
{
}

bool MenuItem::build( HMENU _hPopup, int _index )
{
	hPopup = _hPopup;
	index = _index;
	ID = ::GetMenuItemID( hPopup, index );
	state = ::GetMenuState( hPopup, index, MF_BYPOSITION );
	return isValidCommand();
}

bool MenuItem::isValidCommand( void ) const
{
	if ( index == -1 || ID == 0 || ID == BAD_STATE || state == BAD_STATE )
		return false;

	if ( state & ( MF_GRAYED | MF_DISABLED | MF_POPUP | MF_SEPARATOR ) )
		return false;

	if ( !( state & MF_HILITE ) )
		return false;

	return true;
}

void MenuItem::dump( void )
{
	static struct { UINT flag; LPCTSTR label; } menuFlags[] =
	{
		{ MF_SEPARATOR        , _T("MF_SEPARATOR") },
		{ MF_GRAYED           , _T("MF_GRAYED") },
		{ MF_DISABLED         , _T("MF_DISABLED") },
		{ MF_CHECKED          , _T("MF_CHECKED") },
		{ MF_USECHECKBITMAPS  , _T("MF_USECHECKBITMAPS") },
		{ MF_BITMAP           , _T("MF_BITMAP") },
		{ MF_OWNERDRAW        , _T("MF_OWNERDRAW") },
		{ MF_POPUP            , _T("MF_POPUP") },
		{ MF_MENUBARBREAK     , _T("MF_MENUBARBREAK") },
		{ MF_MENUBREAK        , _T("MF_MENUBREAK") },
		{ MF_HILITE           , _T("MF_HILITE") }
	};

	TRACE( _T(" >> hPopup=0x%08X index=%d ID=%d state=0x%08X: "), hPopup, index, ID, state );
	for ( int i = 0; i < COUNT_OF( menuFlags ); ++i )
		if ( state & menuFlags[ i ].flag )
			TRACE( _T(" %s"), menuFlags[ i ].label );
	TRACE( _T("\n") );
}
