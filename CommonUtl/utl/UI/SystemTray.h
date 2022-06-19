// Originally written by Chris Maunder (https://www.codeproject.com/Articles/74/Adding-Icons-to-the-System-Tray)
// Copyright (c) 1998.
//
#ifndef SystemTray_h
#define SystemTray_h
#pragma once

#include "SystemTray_fwd.h"


abstract class CSystemTray		// wrapper class for interfacing with the shell system tray icons
{
	friend class CTrayIcon;
protected:
	CSystemTray( void );

	virtual CWnd* EnsurePopupWnd( void ) = 0;		// creates or hooks the popup window

	ui::ISystemTrayCallback* GetOwnerCallback( void ) const { return m_pOwnerCallback; }
public:
	virtual ~CSystemTray();

	virtual CWnd* GetPopupWnd( void ) = 0;			// could be the owner top-level window of the hidden popup, depending on the concrete class
	CWnd* GetOwnerWnd( void ) const;				// optional: the top-level owner window, that implements ui::ISystemTrayCallback

	void SetOwnerCallback( ui::ISystemTrayCallback* pOwnerCallback ) { m_pOwnerCallback = pOwnerCallback; }

	bool IsEmpty( void ) const { return m_icons.empty(); }
	const std::vector< CTrayIcon* >& GetIcons( void ) const { return m_icons; }

	CTrayIcon* FindIcon( UINT trayIconId ) const;
	CTrayIcon& LookupIcon( UINT trayIconId ) const;

	CTrayIcon* FindMainIcon( void ) const { return m_mainTrayIconId != 0 ? FindIcon( m_mainTrayIconId ) : NULL; }
	CTrayIcon* FindMessageIcon( void ) const;

	// create the tray icon
	CTrayIcon* CreateTrayIcon( UINT trayIconResId, bool visible = true ) { return CreateTrayIcon( NULL, trayIconResId, NULL, visible ); }		// load tray icon resource + tooltip resource
	CTrayIcon* CreateTrayIcon( HICON hIcon, UINT trayIconId, const TCHAR* pIconTipText, bool visible = true );
	bool DeleteTrayIcon( UINT trayIconId );

	// minimize/restore top-level owner window
	void MinimizeOwnerWnd( bool restoreToMaximized = false );
	void RestoreOwnerWnd( void );
	void OnOwnerWndStatusChanged( void );

	static CSystemTray* Instance( void ) { return s_pInstance; }

	static bool IsMinimizedToTray( const CWnd* pOwnerWnd );
	static bool IsRestoreToMaximized( const CWnd* pOwnerWnd );
private:
	size_t FindIconPos( UINT trayIconId ) const;
private:
	ui::ISystemTrayCallback* m_pOwnerCallback;		// receives tray icon notifications, and handles special events, e.g. NIN_SELECT, NINF_KEY, NIN_KEYSELECT, NIN_BALLOONSHOW, etc
	std::vector< CTrayIcon* > m_icons;				// with ownership
	UINT m_mainTrayIconId;
    UINT m_restoreShowCmd;

	enum CWndFlagsEx
	{	// private flags that extend CWnd::m_nFlags, to be used for the owner window (top-level)
		WF_EX_MinimizedToTray		= 0x01000000,	// owner window is minimized to system tray, and hidden
		WF_EX_RestoreToMaximized	= 0x02000000	// owner window is minimized to system tray, and should be restored to maximized
	};
private:
	static CSystemTray* s_pInstance;			// shared singleton instance for global baloons
public:
	static const UINT WM_TASKBARCREATED;		// send by explorer.exe when process is restarted
	static const UINT WM_TRAYICONNOTIFY;		// icon notify message sent to parent
protected:
	// message handlers
	void HandleDestroy( void );
	bool HandleSysCommand( UINT sysCmdId );
	bool HandleTimer( UINT_PTR eventId );
	void HandleSettingChange( UINT flags );
	void HandleExplorerRestart( void );
	bool HandleTrayIconNotify( WPARAM wParam, LPARAM lParam );		// handler for tray notifications
};


// implementation that uses a hidden top-level popup window that interacts with the tray
//
class CSystemTrayWnd : public CWnd
	, public CSystemTray
{
public:
	CSystemTrayWnd( void );
	virtual ~CSystemTrayWnd();

	virtual CWnd* GetPopupWnd( void ) override { return this; }
protected:
	virtual CWnd* EnsurePopupWnd( void ) override;		// creates the popup window

	// generated stuff
protected:
	afx_msg void OnDestroy( void );
	afx_msg void OnTimer( UINT_PTR eventId );
	afx_msg void OnSettingChange( UINT flags, LPCTSTR pSection );
	LRESULT OnExplorerRestart( WPARAM wParam, LPARAM lParam );
	LRESULT OnTrayIconNotify( WPARAM wParam, LPARAM lParam );

	DECLARE_MESSAGE_MAP()
};


#include "WindowHook.h"


// implementation that hooks the main window's messages and interacts with the system tray
//
class CSystemTrayWndHook : public CWindowHook
	, public CSystemTray
{
public:
	CSystemTrayWndHook( bool autoDelete = false );
	virtual ~CSystemTrayWndHook();

	virtual CWnd* GetPopupWnd( void ) override;
protected:
	virtual CWnd* EnsurePopupWnd( void ) override;		// hooks the popup window

	// base overrides
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam ) override;
};


#endif // SystemTray_h
