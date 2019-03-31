
#include "stdafx.h"
#include "ShellContextMenuBuilder.h"
#include "MenuUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CShellContextMenuBuilder::CShellContextMenuBuilder( HMENU hMenu, UINT indexMenu, UINT idBaseCmd )
	: m_pMenu( CMenu::FromHandle( hMenu ) )
	, m_indexMenu( indexMenu )
	, m_idBaseCmd( idBaseCmd )
	, m_cmdCount( 0 )
{
	ASSERT_PTR( m_pMenu->GetSafeHmenu() );
}

void CShellContextMenuBuilder::AddCmdItem( UINT cmdOffset, const std::tstring& itemText, CBitmap* pItemBitmap )
{
	VERIFY( m_pMenu->InsertMenu( m_indexMenu, MF_STRING | MF_BYPOSITION, m_idBaseCmd + cmdOffset, itemText.c_str() ) );

	if ( pItemBitmap != NULL )
		VERIFY( m_pMenu->SetMenuItemBitmaps( m_indexMenu, MF_BYPOSITION, pItemBitmap, pItemBitmap ) );

	++m_indexMenu;
	++m_cmdCount;
}

void CShellContextMenuBuilder::AddPopupItem( HMENU hSubMenu, const std::tstring& itemText, CBitmap* pItemBitmap )
{
	ASSERT_PTR( hSubMenu );

	VERIFY( m_pMenu->InsertMenu( m_indexMenu, MF_POPUP | MF_BYPOSITION, (UINT_PTR)hSubMenu, itemText.c_str() ) );

	if ( pItemBitmap != NULL )
		VERIFY( m_pMenu->SetMenuItemBitmaps( m_indexMenu, MF_BYPOSITION, pItemBitmap, pItemBitmap ) );

	++m_indexMenu;
	++m_cmdCount;			// for the popup entry (otherwise it skips the callback for last deep folder on ExecuteCommand)
	m_cmdCount += ui::GetTotalCmdCount( hSubMenu, Deep );
}

void CShellContextMenuBuilder::AddSeparator( void )
{
	VERIFY( m_pMenu->InsertMenu( m_indexMenu++, MF_SEPARATOR | MF_BYPOSITION ) );
}
