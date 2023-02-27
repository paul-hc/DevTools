#ifndef TrayIcon_h
#define TrayIcon_h
#pragma once

#include "Image_fwd.h"
#include "Timer.h"
#include "SystemTray_fwd.h"


class CTrayIcon
{
	friend class CSystemTray;

	// managed only by CSystemTray
	CTrayIcon( CSystemTray* pTray, UINT trayIconId, bool isMainIcon );
	~CTrayIcon();

	bool Add( CWnd* pPopupWnd, HICON hIcon, const TCHAR* pIconTipText, bool visible );
	bool Delete( void );

	bool AddPending( void );
	bool InstallPending( void ) { m_addPending = true; return AddPending(); }
public:
	CSystemTray* GetTray( void ) const { return m_pTray; }

	bool IsMainIcon( void ) const { return m_isMainIcon; }		// main icon: the first visible one created via CreateTrayIcon()
	bool IsOnDemand( void ) const { return m_autoHide; }		// display icon on-demand, auto-hide when balloon or animation finishes
	UINT GetID( void ) const { return m_trayIconId; }
	CWnd* GetNotifyWnd( void ) const { return CWnd::FromHandle( m_niData.hWnd ); }

	NOTIFYICONDATA& RefIconData( void ) { return m_niData; }

	bool IsVisible( void ) const { return m_visible; }
	bool SetVisible( bool visible = true );

	HICON GetIcon( void ) const { return m_niData.hIcon; }
	bool SetIcon( HICON hIcon );
	bool SetIcon( const CIconId& iconResId );
	bool LoadStandardIcon( UINT iconResId ) { SetIcon( ::LoadIcon( nullptr, MAKEINTRESOURCE( iconResId ) ) ); }

	void SetTrayFocus( void );

	// tray icon tooltip
	bool IsTooltipVisible( void ) const { return m_tooltipVisible; }
	std::tstring GetTooltipText( void ) const { return m_niData.szTip; }
	bool SetTooltipText( const TCHAR* pIconTipText );

	// tray balloon tip
	bool IsBalloonTipVisible( void ) const { return m_baloonVisible; }
	bool ShowBalloonTip( const std::tstring& text, const TCHAR* pTitle = nullptr, app::MsgType msgType = app::Info, UINT timeoutSecs = 0 );
	bool HideBalloonTip( void ) { return ShowBalloonTip( str::GetEmpty() ); }
	static DWORD ToInfoFlag( app::MsgType msgType, UINT* pOutTimeoutSecs = nullptr );

	// icon animation
	const CImageList& GetAnimImageList( void ) const { return m_animImageList; }
	CImageList& RefAnimImageList( void ) { return m_animImageList; }
	bool CanAnimate( void ) const { return m_animImageList.GetSafeHandle() != nullptr && m_animImageList.GetImageCount() != 0; }
	bool IsAnimating( void ) const { return m_pAnimation.get() != nullptr; }
	void Animate( double durationSecs, UINT stepDelayMiliSecs = 100 );
	bool StopAnimation( void );

	// diagnostics
	std::tstring FormatState( void ) const;
protected:
	bool NotifyTrayIcon( int notifyCode );
	bool Notify_AddIcon( void );
	bool Notify_DeleteIcon( void );
	bool Notify_ModifyState( DWORD stateFlag, bool on );

	// message handlers
	bool HandleTimer( UINT_PTR eventId );
	bool HandleTrayIconNotify( UINT msgNotifyCode, const CPoint& screenPos );

	bool DoShowBalloonTip( const std::tstring& text, const TCHAR* pTitle, DWORD infoFlag, UINT timeoutSecs );
	bool DoHideBalloonTip( void ) { return DoShowBalloonTip( str::GetEmpty(), nullptr, NIIF_NONE, 10 ); }
private:
	struct CBaloonData;
	class CAnimation;

	enum PrivateNotify { _NIN_BalloonPendingShow = NIN_BALLOONUSERCLICK + 33 };

	void PostNotify( PrivateNotify notifyCode, const CPoint& screenPos );
	bool AutoHideIcon( void );
private:
	UINT m_trayIconId;
	CSystemTray* m_pTray;
	ui::ISystemTrayCallback* m_pOwnerCallback;
	UINT m_createFlags;			// stored at creation time
	const bool m_isMainIcon;	// main icon: the first visible one created via CreateTrayIcon(); an application can have additional tray icons
	bool m_autoHide;			// display icon on-demand, auto-hide at the end (balloon, showing animation, etc)
	bool m_addPending;			// failed to add the icon to tray (taskbar window does not exist or is unresponsive)

	bool m_visible;				// not NIS_HIDDEN?
	bool m_tooltipVisible;		// is icon tooltip currently displayed?
	bool m_baloonVisible;		// is icon balloon tip currently displayed?
	bool m_ignoreNextLDblClc;
	UINT m_autoHideFlags;		// auto-hides the tray icon when tip/balloon hides, if tooltips or balloons are displayed if icon was not visible previously

	NOTIFYICONDATA m_niData;

	std::auto_ptr<CBaloonData> m_pBaloonPending;		// used in the delayed transaction to replace a displayed ballon to another with different content (sys-tray notifications have an async sequence)

	// icon animation
	CImageList m_animImageList;
	std::auto_ptr<CAnimation> m_pAnimation;

	static const UINT s_tooltipMaxLength;
	enum { BalloonTextMaxLength = 256, BalloonTitleMaxLength = 64 };
private:
	struct CBaloonData
	{
		CBaloonData( const std::tstring& text, const TCHAR* pTitle, app::MsgType msgType, UINT timeoutSecs )
			: m_text( text ), m_msgType( msgType ), m_timeoutSecs( timeoutSecs )
		{
			if ( pTitle != nullptr )
				m_title = pTitle;
		}

		bool ShowBalloon( CTrayIcon* pIcon ) { ASSERT_PTR( pIcon ); return pIcon->ShowBalloonTip( m_text, m_title.c_str(), m_msgType, m_timeoutSecs ); }
	public:
		std::tstring m_text;
		std::tstring m_title;
		app::MsgType m_msgType;
		UINT m_timeoutSecs;
	};


	class CAnimation
	{
	public:
		CAnimation( CTrayIcon* pTrayIcon, double durationSecs, UINT stepDelayMiliSecs );
		~CAnimation();

		bool HandleTimerEvent( UINT_PTR eventId );
		bool IsTimeout( void ) const { return m_timer.ElapsedSeconds() > m_durationSecs; }
	private:
		enum { AnimationEventId = 7654 };

		CTrayIcon* m_pTrayIcon;
		CWnd* m_pWnd;
		const HICON m_hIconOrig;
		const double m_durationSecs;
		const UINT_PTR m_eventId;
		const size_t m_imageCount;
		size_t m_imagePos;
		CTimer m_timer;
	};
};


#endif // TrayIcon_h
