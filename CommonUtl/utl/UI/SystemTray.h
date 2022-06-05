/////////////////////////////////////////////////////////////////////////////
//
// Written by Chris Maunder (https://www.codeproject.com/Articles/74/Adding-Icons-to-the-System-Tray)
// Copyright (c) 1998.
//
#ifndef SystemTray_h
#define SystemTray_h
#pragma once

#include <vector>
#include "Timer.h"


class CTrayIconAnimation;
namespace ui { interface ISystemTrayCallback; }


abstract class CSystemTray		// wrapper class for interfacing with the shell system tray icons
{
protected:
	CSystemTray( void );

	virtual CWnd* EnsurePopupWnd( void ) = 0;		// creates or hooks the popup window
public:
	virtual ~CSystemTray();

	virtual CWnd* GetPopupWnd( void ) = 0;

	void SetOwnerCallback( ui::ISystemTrayCallback* pOwnerCallback ) { m_pOwnerCallback = pOwnerCallback; }

	// create the tray icon
	bool CreateTrayIcon( HICON hIcon, UINT trayIconId, const TCHAR* pIconTipText, bool iconHidden = false );
	bool CreateTrayIcon( UINT trayIconResId, const TCHAR* pIconTipText, bool iconHidden = false );		// load tray icon resource

	static CSystemTray* Instance( void ) { return s_pInstance; }

	static bool IsMinimizedToTray( CWnd* pPopupWnd );
	static void MinimizeToTray( CWnd* pPopupWnd );
	static void RestoreFromTray( CWnd* pPopupWnd );

	void MinimizePopupWnd( void );
	void RestorePopupWnd( void );

	CWnd* GetNotifyWnd( void ) const { return CWnd::FromHandle( m_niData.hWnd ); }

	// tray icon tooltip
	bool IsTooltipVisible( void ) const { return m_tooltipVisible; }
	std::tstring GetTooltipText( void ) const { return m_niData.szTip; }
	bool SetTooltipText( const TCHAR* pIconTipText );

	bool IsBalloonTipVisible( void ) const { return m_baloonVisible; }
	bool ShowBalloonTip( const TCHAR text[], const TCHAR* pTitle = NULL, DWORD infoFlag = NIIF_NONE, UINT timeoutSecs = 10 );	// Win 2K+
	bool HideBalloonTip( void ) { return ShowBalloonTip( NULL ); }

	// icon displayed
	UINT GetTrayIconId( void ) const { return m_niData.uID; }

	bool HasIcon( void ) const { return m_iconAdded; }
	HICON GetIcon( void ) const { return m_niData.hIcon; }
	bool SetIcon( HICON hIcon );

	bool LoadResIcon( UINT iconResId );
	bool SetStandardIcon( UINT iconResId ) { SetIcon( ::LoadIcon( NULL, MAKEINTRESOURCE( iconResId ) ) ); }

	bool AddIcon( void );
	bool RemoveIcon( void );

	bool IsIconVisible( void ) const { return HasIcon() && !m_iconHidden; }
	bool SetIconVisible( bool visible = true );

	void SetTrayFocus( void );

	// icon animation
	CImageList& GetAnimImageList( void ) const { return const_cast<CImageList&>( m_animImageList ); }
	bool CanAnimate( void ) const { return m_animImageList.GetSafeHandle() != NULL && m_animImageList.GetImageCount() != 0 && IsIconVisible(); }
	bool IsAnimating( void ) const { return m_pAnimation.get() != NULL; }
	void Animate( UINT stepDelayMiliSecs, double durationSecs );
	bool StopAnimation( void );
protected:
	bool NotifyTrayIcon( int notifyCode );
	bool Notify_AddIcon( void );
	bool Notify_DeleteIcon( void );
	bool Notify_ModifyState( DWORD stateFlag, bool on );

	void InstallIconPending( void );
	void InstallIconPending( bool showIconPending ) { m_showIconPending = showIconPending; InstallIconPending(); }

	ui::ISystemTrayCallback* GetOwnerCallback( void ) const { return m_pOwnerCallback; }
	NOTIFYICONDATA& RefIconData( void ) { return m_niData; }
private:
	ui::ISystemTrayCallback* m_pOwnerCallback;		// receives tray icon notifications, and handles special events, e.g. NIN_SELECT, NINF_KEY, NIN_KEYSELECT, NIN_BALLOONSHOW, etc
	NOTIFYICONDATA m_niData;
	bool m_iconAdded;			// has the icon been added?
	bool m_iconHidden;
	bool m_showIconPending;		// show the icon once the taskbar has been created
	bool m_tooltipVisible;		// is icon tooltip currently displayed?
	bool m_baloonVisible;		// is icon balloon tip currently displayed?
	bool m_ignoreNextLDblClc;
	UINT m_autoHideFlags;		// auto-hides the tray icon when tip/balloon hides, if tooltips or balloons are displayed if icon was not visible previously

	// main icon: the first one created via CreateTrayIcon(); an application can have additional tray icons
	UINT m_mainTrayIconId;
	UINT m_mainFlags;

	// icon animation
	CImageList m_animImageList;
	std::auto_ptr<CTrayIconAnimation> m_pAnimation;
private:
	enum { BalloonTextMaxLength = 256, BalloonTitleMaxLength = 64 };

	static CSystemTray* s_pInstance;			// shared singleton instance for global baloons
	static const UINT s_tooltipMaxLength;
public:
	static const UINT WM_TASKBARCREATED;		// send by explorer.exe when process is restarted
	static const UINT WM_TRAYICONNOTIFY;		// icon notify message sent to parent
protected:
	// message handlers
	void HandleDestroy( void );
	bool HandleTimer( UINT_PTR eventId );
	void HandleSettingChange( UINT flags );
	void HandleExplorerRestart( void );
	bool HandleTrayIconNotify( WPARAM wParam, LPARAM lParam );			// handler for tray notifications
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

	bool SetNotifyWnd( CWnd* pNotifyWnd );				// window that will receieve tray notifications; normally should not be used
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


#include "ISystemTrayCallback.h"


// singleton instance for displaying on-demand application global baloons for the main window (lazy created, for occasional tray notifications)
//
class CSystemTraySingleton : public CSystemTrayWndHook
	, public ui::ISystemTrayCallback
{
public:
	CSystemTraySingleton( CWnd* pMainWnd, UINT appIconId = s_appIconId, const TCHAR* pAppTipText = s_appTipText.c_str() );

	static void StoreAppInfo( UINT appIconId, const std::tstring& appTipText );
private:
	void Construct( UINT appIconId, const TCHAR* pAppTipText );

	virtual CWnd* GetOwnerWnd( void ) override { return m_pMainWnd; }
	virtual CMenu* GetTrayIconContextMenu( void ) override { return NULL; }
	virtual bool OnTrayIconNotify( UINT msgNotifyCode, UINT iconId, const CPoint& screenPos ) override;
private:
	CWnd* m_pMainWnd;

	static UINT s_appIconId;
	static std::tstring s_appTipText;
};


#endif // SystemTray_h
