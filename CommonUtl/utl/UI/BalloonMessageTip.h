#ifndef BalloonMessageTip_h
#define BalloonMessageTip_h
#pragma once

//
// Inspired from Paul Roberts' article: https://www.codeproject.com/Articles/24155/CBalloonMsg-An-Easy-to-use-Non-modal-Balloon-Alter
//
// Uses a small, transparent window to launch the tooltip in a separate thread.
// This enables the tooltip to be used in almost any situation, without any big changes to the
// calling code (i.e. no need to route msgs via RelayEvent etc).
//
// Similar in functionality with CEdit::ShowBalloonTip(), but works for any control or window.
//

#include "ui_fwd.h"
#include "WindowTimer.h"


namespace ui
{
	bool BalloonsEnabled( void );				// balloons have been suppressed?

	CPoint GetDisplayScreenPos( const CWnd* pCtrl );
}


class CEnumTags;
struct CGuiThreadInfo;


class CBalloonHostWnd : public CWnd
{	// a small transparent window that sits above the initial mouse position and is the parent for the tooltip
	friend class CBalloonUiThread;

	CBalloonHostWnd( void );

	bool Create( CWnd* pParentWnd );
public:
	virtual ~CBalloonHostWnd();

	static CBalloonHostWnd* Display( const TCHAR* pTitle, const std::tstring& message, HICON hToolIcon = TTI_NONE, const CPoint& screenPos = ui::GetNullPos() );

	bool IsTooltipDisplayed( void ) const { return ::IsWindow( m_toolTipCtrl.GetSafeHwnd() ) && m_toolTipCtrl.IsWindowVisible(); }
	void SetAutoPopTimeout( UINT autoPopTimeout ) { m_autoPopTimeout = autoPopTimeout; }
private:
	bool CreateToolTip( void );
	bool CheckMainThreadChanges( void );
private:
	const DWORD m_mainThreadId;

	std::tstring m_title;
	std::tstring m_message;
	HICON m_hToolIcon;
	CPoint m_screenPos;
	UINT m_autoPopTimeout;		// time before self-close (millisecs), set to zero for unlimited

	CToolTipCtrl m_toolTipCtrl;
	CWindowTimer m_timer;
	UINT m_eventCount;
	std::auto_ptr< CGuiThreadInfo > m_pThreadInfo;			// to check for changes in the main thread

	enum { TimerEventId = 100, EllapseMs = 50, ToolId = 10 };
public:
	// to customize: modify before displaying the baloon tip
	static UINT s_autoPopTimeout;		// time before self-close (millisecs), set to zero for unlimited
	static UINT s_maxTipWidth;			// in pixels, set to force linebreaks in tooltip content
	static UINT s_toolBorder;			// used to judge when the mouse has moved enough to pop the balloon

	enum TipMetrics
	{
		AutoPopTimeout = 10000,			// stay up for 10 secs unless RequestCloseAll() is used
		MaxTipWidth = 260,				// in pixels, set to force linebreaks in tooltip content
		ToolBorder = 10					// used to judge when the mouse has moved enough to pop the balloon
	};

	// generated stuff
public:
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	virtual BOOL OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult );
public:
	afx_msg int OnCreate( CREATESTRUCT* pCreateStruct );
	afx_msg void OnDestroy( void );
	afx_msg void OnTimer( UINT_PTR eventId );
	afx_msg BOOL OnSetCursor( CWnd* pWnd, UINT hitTest, UINT message );

	DECLARE_MESSAGE_MAP()
};


class CBalloonUiThread : public CWinThread
{	// a separate UI thread that provides a dedicated message queue for our tooltip
	DECLARE_DYNCREATE( CBalloonUiThread )

	CBalloonUiThread( void );
public:
	virtual ~CBalloonUiThread();

	CBalloonHostWnd* GetHostWnd( void ) { return &m_hostWnd; }
	void RequestClose( void ) { m_exiting = true; }

	static void ResetCloseAll( void ) { s_exitingAll = false; }
	static void RequestCloseAll( void ) { s_exitingAll = true; }
private:
	CBalloonHostWnd m_hostWnd;
	bool m_exiting;
	static bool s_exitingAll;			// to close all current balloons

	// generated stuff
public:
	virtual BOOL InitInstance( void );
	virtual int ExitInstance( void );
	virtual BOOL OnIdle( LONG count );
};


struct CGuiThreadInfo : public tagGUITHREADINFO
{
	CGuiThreadInfo( DWORD threadId = 0 );

	void ReadInfo( DWORD threadId );

	enum Change { None, FocusChanged, CaptureChanged, FlagsChanged };

	Change Compare( const GUITHREADINFO& right ) const;
	static const CEnumTags& GetTags_Change( void );
};


#endif // BalloonMessageTip_h
