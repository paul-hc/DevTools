
#include "pch.h"
#include "ShortcutItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CShortcutItem implementation

CShortcutItem::CShortcutItem( const fs::CPath& linkPath, const shell::CShortcut& shortcut )
	: CPathItemBase( linkPath )
	, m_shortcut( shortcut )
{
}

CShortcutItem::CShortcutItem( const fs::CPath& linkPath, IShellLink* pShellLink )
	: CPathItemBase( linkPath )
	, m_shortcut( pShellLink )
{
}

CShortcutItem::~CShortcutItem()
{
}
