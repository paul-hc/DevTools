
#include "pch.h"
#include "EditLinkItem.h"
#include "utl/FlagTags.h"
#include "utl/UI/Shortcut.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CEditLinkItem implementation

CEditLinkItem::CEditLinkItem( const fs::CPath& linkPath, IShellLink* pShellLink )
	: CShortcutItem( linkPath, pShellLink )
	, m_destShortcut( GetShortcut() )
{
}

CEditLinkItem::CEditLinkItem( const fs::CPath& linkPath, const shell::CShortcut& srcShortcut )
	: CShortcutItem( linkPath, srcShortcut )
	, m_destShortcut( srcShortcut )
{
}

CEditLinkItem::~CEditLinkItem()
{
}

CEditLinkItem* CEditLinkItem::LoadLinkItem( const fs::CPath& linkPath )
{
	CComPtr<IShellLink> pShellLink = shell::LoadLinkFromFile( linkPath.GetPtr() );

	return new CEditLinkItem( linkPath, pShellLink );
}


namespace fmt
{
	std::tstring FormatEditLinkEntry( const fs::CPath& linkPath, const shell::CShortcut& srcShortcut, const shell::CShortcut& destShortcut )
	{
		return linkPath.Get()
			+ _T(": {")
			+ shell::CShortcut::GetTags_Fields().FormatKey( srcShortcut.GetDiffFields( destShortcut ) )
			+ _T("}");
	}
}
