#ifndef EditLinkItem_h
#define EditLinkItem_h
#pragma once

#include "utl/UI/ShortcutItem.h"


class CEditLinkItem : public CShortcutItem
{
	CEditLinkItem( const fs::CPath& linkPath, IShellLink* pShellLink );
public:
	CEditLinkItem( const fs::CPath& linkPath, const shell::CShortcut& srcShortcut );
	virtual ~CEditLinkItem();

	static CEditLinkItem* LoadLinkItem( const fs::CPath& linkPath );

	const shell::CShortcut& GetSrcShortcut( void ) const { return GetShortcut(); }
	const shell::CShortcut& GetDestShortcut( void ) const { return m_destShortcut; }

	bool IsModified( void ) const { return m_destShortcut.IsTargetValid() && m_destShortcut != GetSrcShortcut(); }

	shell::CShortcut& RefDestShortcut( void ) { return m_destShortcut; }
	void Reset( void ) { m_destShortcut = GetSrcShortcut(); }
private:
	shell::CShortcut m_destShortcut;
};


namespace func
{
	struct AsSrcShortcut
	{
		const shell::CShortcut& operator()( const CEditLinkItem* pItem ) const { return pItem->GetSrcShortcut(); }
	};


	// accessors for order predicates

	struct Dest_AsTargetPath
	{
		const fs::CPath& operator()( const CEditLinkItem* pItem ) const { return pItem->GetDestShortcut().GetTargetPath(); }
	};

	struct Dest_AsTargetPidl
	{
		const shell::CPidlAbsolute& operator()( const CEditLinkItem* pItem ) const { return pItem->GetDestShortcut().GetTargetPidl(); }
	};

	struct Dest_AsWorkDirPath
	{
		const fs::CPath& operator()( const CEditLinkItem* pItem ) const { return pItem->GetDestShortcut().GetWorkDirPath(); }
	};

	struct Dest_AsArguments
	{
		const std::tstring& operator()( const CEditLinkItem* pItem ) const { return pItem->GetDestShortcut().GetArguments(); }
	};

	struct Dest_AsDescription
	{
		const std::tstring& operator()( const CEditLinkItem* pItem ) const { return pItem->GetDestShortcut().GetDescription(); }
	};

	struct Dest_AsIconLocation
	{
		const shell::CIconLocation& operator()( const CEditLinkItem* pItem ) const { return pItem->GetDestShortcut().GetIconLocation(); }
	};
}


namespace fmt
{
	std::tstring FormatEditLinkEntry( const fs::CPath& linkPath, const shell::CShortcut& srcShortcut, const shell::CShortcut& destShortcut );
}


#endif // EditLinkItem_h
