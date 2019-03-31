#ifndef ShellContextMenuBuilder_h
#define ShellContextMenuBuilder_h
#pragma once


class CShellContextMenuBuilder
{
public:
	CShellContextMenuBuilder( HMENU hMenu, UINT indexMenu, UINT idBaseCmd );

	void AddCmdItem( UINT cmdOffset, const std::tstring& itemText, CBitmap* pItemBitmap );
	void AddPopupItem( HMENU hSubMenu, const std::tstring& itemText, CBitmap* pItemBitmap );
	void AddSeparator( void );

	UINT GetAddedCmdCount( void ) const { return m_cmdCount; }
	UINT MakeCmdId( UINT cmdOffset ) const { return m_idBaseCmd + cmdOffset; }
private:
	CMenu* m_pMenu;
	UINT m_indexMenu;
	UINT m_idBaseCmd;
	UINT m_cmdCount;		// total count of commands ADDED (excluding separators, sub-menus)
};


#endif // ShellContextMenuBuilder_h
