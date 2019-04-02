#ifndef ShellContextMenuBuilder_h
#define ShellContextMenuBuilder_h
#pragma once


class CShellContextMenuBuilder;


class CBaseMenuBuilder
{
protected:
	CBaseMenuBuilder( CMenu* pPopupMenu, UINT indexMenu );
public:
	CMenu* GetPopupMenu( void ) const { return m_pPopupMenu; }

	void AddCmdItem( UINT cmdOffset, const std::tstring& itemText, CBitmap* pItemBitmap );
	void AddPopupItem( HMENU hSubMenu, const std::tstring& itemText, CBitmap* pItemBitmap );
	void AddSeparator( void );
protected:
	// composite overridables
	virtual CBaseMenuBuilder* GetParentBuilder( void ) const = 0;

	bool IsShellMenu( void ) const { return NULL == GetParentBuilder(); }
	bool IsSubMenuMenu( void ) const { return !IsShellMenu(); }
	CShellContextMenuBuilder* GetShellBuilder( void );
private:
	CMenu* m_pPopupMenu;
	UINT m_indexMenu;

	CShellContextMenuBuilder* m_pShellBuilder;		// self-encapsulated, lazy
};


class CSubMenuBuilder : public CBaseMenuBuilder
{
public:
	CSubMenuBuilder( CBaseMenuBuilder* pParentBuilder );
	~CSubMenuBuilder();
protected:
	// composite overrides
	virtual CBaseMenuBuilder* GetParentBuilder( void ) const;

	static CMenu* CreateEmptyPopupMenu( void );
private:
	CBaseMenuBuilder* m_pParentBuilder;
};


class CShellContextMenuBuilder : public CBaseMenuBuilder
{
public:
	CShellContextMenuBuilder( HMENU hShellMenu, UINT indexMenu, UINT idBaseCmd );

	UINT GetAddedCmdCount( void ) const { return m_cmdCount; }
	UINT MakeCmdId( UINT cmdOffset ) const { return m_idBaseCmd + cmdOffset; }

	void OnAddCmd( void ) { ++m_cmdCount; }
protected:
	// composite overrides
	virtual CBaseMenuBuilder* GetParentBuilder( void ) const;
private:
	const UINT m_idBaseCmd;
	UINT m_cmdCount;			// total count of commands ADDED (excluding separators, sub-menus)
};


#endif // ShellContextMenuBuilder_h
