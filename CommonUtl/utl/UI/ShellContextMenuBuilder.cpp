
#include "stdafx.h"
#include "ShellContextMenuBuilder.h"
#include "MenuUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CBaseMenuBuilder implementation

CBaseMenuBuilder::CBaseMenuBuilder( CShellContextMenuBuilder* pShellBuilder, CMenu* pPopupMenu, UINT indexMenu )
	: m_pShellBuilder( pShellBuilder )
	, m_pPopupMenu( pPopupMenu )
	, m_indexMenu( indexMenu )
{
	ASSERT_PTR( m_pShellBuilder );
	ASSERT_PTR( m_pPopupMenu->GetSafeHmenu() );
}

void CBaseMenuBuilder::AddCmdItem( UINT cmdOffset, const std::tstring& itemText, CBitmap* pItemBitmap )
{
	UINT cmdId = GetShellBuilder()->MakeCmdId( cmdOffset );

	VERIFY( m_pPopupMenu->InsertMenu( m_indexMenu, MF_STRING | MF_BYPOSITION, cmdId, itemText.c_str() ) );

	if ( pItemBitmap != nullptr )
		VERIFY( m_pPopupMenu->SetMenuItemBitmaps( m_indexMenu, MF_BYPOSITION, pItemBitmap, pItemBitmap ) );

	++m_indexMenu;
	GetShellBuilder()->OnAddCmd( cmdId );
}

void CBaseMenuBuilder::AddPopupItem( HMENU hSubMenu, const std::tstring& itemText, CBitmap* pItemBitmap )
{
	ASSERT_PTR( hSubMenu );

	VERIFY( m_pPopupMenu->InsertMenu( m_indexMenu, MF_POPUP | MF_BYPOSITION, (UINT_PTR)hSubMenu, itemText.c_str() ) );

	if ( pItemBitmap != nullptr )
		VERIFY( m_pPopupMenu->SetMenuItemBitmaps( m_indexMenu, MF_BYPOSITION, pItemBitmap, pItemBitmap ) );

	++m_indexMenu;
}

void CBaseMenuBuilder::AddSeparator( void )
{
	VERIFY( m_pPopupMenu->InsertMenu( m_indexMenu, MF_SEPARATOR | MF_BYPOSITION ) );
	++m_indexMenu;
}


// CSubMenuBuilder implementation

CSubMenuBuilder::CSubMenuBuilder( CBaseMenuBuilder* pParentBuilder )
	: CBaseMenuBuilder( pParentBuilder->GetShellBuilder(), CreateEmptyPopupMenu(), 0 )
	, m_pParentBuilder( pParentBuilder )
{
}

CSubMenuBuilder::~CSubMenuBuilder()
{
	delete GetPopupMenu();
}

CBaseMenuBuilder* CSubMenuBuilder::GetParentBuilder( void ) const
{
	return m_pParentBuilder;
}

CMenu* CSubMenuBuilder::CreateEmptyPopupMenu( void )
{
	CMenu* pPopupMenu = new CMenu();
	pPopupMenu->CreatePopupMenu();
	return pPopupMenu;
}


// CShellContextMenuBuilder implementation

CShellContextMenuBuilder::CShellContextMenuBuilder( HMENU hShellMenu, UINT indexMenu, UINT idBaseCmd )
	: CBaseMenuBuilder( this, CMenu::FromHandle( hShellMenu ), indexMenu )
	, m_idBaseCmd( idBaseCmd )
	, m_maxCmdId( 0 )
	, m_cmdCount( 0 )
{
}

CBaseMenuBuilder* CShellContextMenuBuilder::GetParentBuilder( void ) const
{
	return nullptr;
}

void CShellContextMenuBuilder::OnAddCmd( UINT cmdId )
{
	m_maxCmdId = std::max( cmdId, m_maxCmdId );
	++m_cmdCount;
}
