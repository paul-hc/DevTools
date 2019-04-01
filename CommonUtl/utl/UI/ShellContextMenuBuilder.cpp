
#include "stdafx.h"
#include "ShellContextMenuBuilder.h"
#include "MenuUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CBaseMenuBuilder implementation

CBaseMenuBuilder::CBaseMenuBuilder( CBaseMenuBuilder* pParentBuilder, CMenu* pPopupMenu, UINT indexMenu )
	: m_pParentBuilder( pParentBuilder )
	, m_pPopupMenu( pPopupMenu )
	, m_indexMenu( indexMenu )
	, m_pShellBuilder( NULL )
{
	ASSERT_PTR( m_pPopupMenu->GetSafeHmenu() );
}

CBaseMenuBuilder::~CBaseMenuBuilder()
{
}

CShellContextMenuBuilder* CBaseMenuBuilder::GetShellBuilder( void )
{
	if ( NULL == m_pShellBuilder )
	{
		CBaseMenuBuilder* pParentBuilder = m_pParentBuilder;
		if ( NULL == pParentBuilder )
			pParentBuilder = this;
		else
			while ( pParentBuilder->m_pParentBuilder != NULL )
				pParentBuilder = pParentBuilder->m_pParentBuilder;

		m_pShellBuilder = checked_static_cast< CShellContextMenuBuilder* >( pParentBuilder );
	}
	ENSURE( m_pShellBuilder != NULL );
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

CMenu* CBaseMenuBuilder::CreateEmptyPopupMenu( void )
{
	CMenu* pPopupMenu = new CMenu;
	pPopupMenu->CreatePopupMenu();
	return pPopupMenu;
}


// CShellContextMenuBuilder implementation

CShellContextMenuBuilder::CShellContextMenuBuilder( HMENU hShellMenu, UINT indexMenu, UINT idBaseCmd )
	: CBaseMenuBuilder( NULL, CMenu::FromHandle( hShellMenu ), indexMenu )
	, m_idBaseCmd( idBaseCmd )
	, m_cmdCount( 0 )
{
}
