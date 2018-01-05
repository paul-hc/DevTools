#ifndef BasePopupDialog_h
#define BasePopupDialog_h
#pragma once


class CAccelTable;


class CAccelPool
{
public:
	virtual ~CAccelPool( void );

	void AddAccelTable( CAccelTable* pAccelTable );
	bool TranslateAccels( MSG* pMsg, HWND hDialog );
private:
	std::vector< CAccelTable* > m_accelTables;		// pool of accel tables that translate commands to this dialog
};


class CIcon;


abstract class CBasePopupDialog
{
protected:
	CBasePopupDialog( void );
	virtual ~CBasePopupDialog( void ) {}		// for dynamic casting
public:
	enum DlgIcon { DlgSmallIcon = ICON_SMALL, DlgLargeIcon = ICON_BIG };

	virtual const CIcon* GetDlgIcon( DlgIcon dlgIcon = DlgSmallIcon ) const;
	void LoadDlgIcon( UINT dlgIconId );
	void SetupDlgIcons( void );

	bool CanAddAboutMenuItem( void ) const;
	void AddAboutMenuItem( CMenu* pMenu );
public:
	// option flags
	bool m_resizable;			// allow resizing
	bool m_initCentered;		// center dialog on init, false: keep absolute position
	bool m_hideSysMenuIcon;		// hide dialog sys-menu icon
	bool m_noAboutMenuItem;		// avoid adding "About..." item to system menu icon
protected:
	CAccelPool m_accelPool;
private:
	UINT m_dlgIconId;
};


#endif // BasePopupDialog_h
