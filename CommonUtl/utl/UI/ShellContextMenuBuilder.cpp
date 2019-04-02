
#include "stdafx.h"
#include "ShellContextMenuBuilder.h"
#include "MenuUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CBaseMenuBuilder implementation

CBaseMenuBuilder::CBaseMenuBuilder( CMenu* pPopupMenu, UINT indexMenu )
	: m_pPopupMenu( pPopupMenu )
	, m_indexMenu( indexMenu )
	, m_pShellBuilder( NULL )
{
	ASSERT_PTR( m_pPopupMenu->GetSafeHmenu() );
}

CShellContextMenuBuilder* CBaseMenuBuilder::GetShellBuilder( void )
{
	if ( NULL == m_pShellBuilder )
	{
		CBaseMenuBuilder* pParentBuilder = GetParentBuilder();
		if ( NULL == pParentBuilder )
			pParentBuilder = this;
		else
			while ( pParentBuilder->GetParentBuilder() != NULL )
				pParentBuilder = pParentBuilder->GetParentBuilder();

		m_pShellBuilder = checked_static_cast< CShellContextMenuBuilder* >( pParentBuilder );
	}
	ASSERT_PTR( m_pShellBuilder );
	return m_pShellBuilder;
}

void CBaseMenuBuilder::AddCmdItem( UINT cmdOffset, const std::tstring& itemText, CBitmap* pItemBitmap )
{
	VERIFY( m_pPopupMenu->InsertMenu( m_indexMenu, MF_STRING | MF_BYPOSITION, GetShellBuilder()->MakeCmdId( cmdOffset ), itemText.c_str() ) );

	if ( pItemBitmap != NULL )
		VERIFY( m_pPopupMenu->SetMenuItemBitmaps( m_indexMenu, MF_BYPOSITION, pItemBitmap, pItemBitmap ) );

	++m_indexMenu;
	GetShellBuilder()->OnAddCmd();
}

void CBaseMenuBuilder::AddPopupItem( HMENU hSubMenu, const std::tstring& itemText, CBitmap* pItemBitmap )
{
	ASSERT_PTR( hSubMenu );

	VERIFY( m_pPopupMenu->InsertMenu( m_indexMenu, MF_POPUP | MF_BYPOSITION, (UINT_PTR)hSubMenu, itemText.c_str() ) );

	if ( pItemBitmap != NULL )
		VERIFY( m_pPopupMenu->SetMenuItemBitmaps( m_indexMenu, MF_BYPOSITION, pItemBitmap, pItemBitmap ) );

	++m_indexMenu;
	GetShellBuilder()->OnAddCmd();			// for the popup entry - otherwise it skips the callback for last deep folder on IContextMenu::InvokeCommand()
}

void CBaseMenuBuilder::AddSeparator( void )
{
	VERIFY( m_pPopupMenu->InsertMenu( m_indexMenu, MF_SEPARATOR | MF_BYPOSITION ) );
	++m_indexMenu;
}


// CSubMenuBuilder implementation

CSubMenuBuilder::CSubMenuBuilder( CBaseMenuBuilder* pParentBuilder )
	: CBaseMenuBuilder( CreateEmptyPopupMenu(), 0 )
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
	CMenu* pPopupMenu = new CMenu;
	pPopupMenu->CreatePopupMenu();
	return pPopupMenu;
}


// CShellContextMenuBuilder implementation

CShellContextMenuBuilder::CShellContextMenuBuilder( HMENU hShellMenu, UINT indexMenu, UINT idBaseCmd )
	: CBaseMenuBuilder( CMenu::FromHandle( hShellMenu ), indexMenu )
	, m_idBaseCmd( idBaseCmd )
	, m_cmdCount( 0 )
{
}

CBaseMenuBuilder* CShellContextMenuBuilder::GetParentBuilder( void ) const
{
	return NULL;
}
