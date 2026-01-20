#ifndef ShortcutItem_h
#define ShortcutItem_h
#pragma once

#include "utl/PathItemBase.h"
#include "Shortcut.h"


class CShortcutItem : public CPathItemBase
{
public:
	CShortcutItem( const fs::CPath& linkPath, const shell::CShortcut& shortcut );
	CShortcutItem( const fs::CPath& linkPath, IShellLink* pShellLink );
	virtual ~CShortcutItem();

	const shell::CShortcut& GetShortcut( void ) const { return m_shortcut; }
	shell::CShortcut& RefShortcut( void ) { return m_shortcut; }
private:
	shell::CShortcut m_shortcut;
};


namespace func
{
	// accessors for order predicates

	struct AsTargetPath
	{
		const fs::CPath& operator()( const CShortcutItem* pItem ) const { return pItem->GetShortcut().GetTargetPath(); }
	};

	struct AsTargetPidl
	{
		const shell::CPidlAbsCp& operator()( const CShortcutItem* pItem ) const { return pItem->GetShortcut().GetTargetPidl(); }
	};

	struct AsWorkDirPath
	{
		const fs::CPath& operator()( const CShortcutItem* pItem ) const { return pItem->GetShortcut().GetWorkDirPath(); }
	};

	struct AsArguments
	{
		const std::tstring& operator()( const CShortcutItem* pItem ) const { return pItem->GetShortcut().GetArguments(); }
	};

	struct AsDescription
	{
		const std::tstring& operator()( const CShortcutItem* pItem ) const { return pItem->GetShortcut().GetDescription(); }
	};

	struct AsIconLocation
	{
		const shell::CIconLocation& operator()( const CShortcutItem* pItem ) const { return pItem->GetShortcut().GetIconLocation(); }
	};
}


#endif // ShortcutItem_h
