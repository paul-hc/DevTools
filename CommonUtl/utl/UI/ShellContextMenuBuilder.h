#ifndef ShellContextMenuBuilder_h
#define ShellContextMenuBuilder_h
#pragma once


class CShellContextMenuBuilder;


abstract class CBaseMenuBuilder
{
protected:
	CBaseMenuBuilder( CShellContextMenuBuilder* pShellBuilder, CMenu* pPopupMenu, UINT indexMenu );
public:
	CShellContextMenuBuilder* GetShellBuilder( void ) const { return m_pShellBuilder; }
	CMenu* GetPopupMenu( void ) const { return m_pPopupMenu; }

	void AddCmdItem( UINT cmdOffset, const std::tstring& itemText, CBitmap* pItemBitmap );
	void AddPopupItem( HMENU hSubMenu, const std::tstring& itemText, CBitmap* pItemBitmap );
	void AddSeparator( void );
protected:
	// composite overridables
	virtual CBaseMenuBuilder* GetParentBuilder( void ) const = 0;

	bool IsShellMenu( void ) const { return nullptr == GetParentBuilder(); }
	bool IsSubMenuMenu( void ) const { return !IsShellMenu(); }
private:
	CShellContextMenuBuilder* m_pShellBuilder;
	CMenu* m_pPopupMenu;
	UINT m_indexMenu;
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

	UINT MakeCmdId( UINT cmdOffset ) const { return m_idBaseCmd + cmdOffset; }

	void OnAddCmd( UINT cmdId );

	// query context menu results
	UINT GetNextCmdId( void ) const { return m_maxCmdId - m_idBaseCmd + 1; }		// increment, otherwise it skips the callback for last command on IContextMenu::InvokeCommand()
	UINT GetCmdCount( void ) const { return m_cmdCount; }
protected:
	// composite overrides
	virtual CBaseMenuBuilder* GetParentBuilder( void ) const;
private:
	const UINT m_idBaseCmd;
	UINT m_maxCmdId;			// maximum inserted command id, deep
	UINT m_cmdCount;			// total count of commands ADDED (excluding separators, sub-menus)
};


#endif // ShellContextMenuBuilder_h
