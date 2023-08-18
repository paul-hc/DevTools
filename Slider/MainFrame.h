#ifndef MainFrame_h
#define MainFrame_h
#pragma once

#include "utl/UI/InternalChange.h"
#include "utl/UI/WindowTimer.h"
#include "utl/UI/BaseMainFrameWndEx.h"


interface IImageView;
class CBaseZoomView;
class CMainToolbar;

typedef CBaseMainFrameWndEx<CMDIFrameWndEx> TMDIFrameWndEx;

enum StatusBarFields { Status_Info, Status_ProgressLabel, Status_Progress };


class CMainFrame : public TMDIFrameWndEx
{
	DECLARE_DYNAMIC( CMainFrame )
public:
	CMainFrame( void );
	virtual ~CMainFrame();

	size_t GetMdiChildCount( void ) const;
	IImageView* GetActiveImageView( void ) const;
	bool IsViewActive( IImageView* pImageView ) const { return GetActiveImageView() == pImageView; }
	bool IsMdiRestored( void ) const;			// current document not maximized?

	CMainToolbar* GetToolbar( void ) { return m_pToolbar.get(); }
	CMFCStatusBar* GetStatusBar( void ) { return &m_statusBar; }

	// status bar temporary messages
	bool IsStatusBarAutoClear( void ) const { return m_messageClearTimer.IsStarted(); }
	bool CancelStatusBarAutoClear( UINT idleMessageID = AFX_IDS_IDLEMESSAGE );
	void SetStatusBarMessage( const TCHAR* pMessage, UINT elapseMs = UINT_MAX );
	void SetIdleStatusBarMessage( UINT idleMessageID = AFX_IDS_IDLEMESSAGE );

	void StartEnqueuedAlbumTimer( UINT timerDelay = 750 );

	bool ResizeViewToFit( CBaseZoomView* pZoomScrollView );
protected:
	void CleanupWindow( void );
private:
	CMFCMenuBar	m_menuBar;
	CMFCToolBar m_standardToolBar;
	CMFCToolBar m_albumToolBar;
	CMFCStatusBar m_statusBar;

		// obsolete	
		std::auto_ptr<CMainToolbar> m_pToolbar;

	CWindowTimer m_messageClearTimer;
	CWindowTimer m_ddeEnqueuedTimer;				// monitors enqueued image paths

	enum Metrics { ProgressBarWidth = 100 };
	enum TimerIds { MessageTimerId = 2000, QueueTimerId };

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT cmdId, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	afx_msg int OnCreate( CREATESTRUCT* pCS );
	afx_msg void OnDestroy( void );
	afx_msg void OnClose( void );
	afx_msg void OnShowWindow( BOOL bShow, UINT nStatus );
	afx_msg void OnDropFiles( HDROP hDropInfo );
	afx_msg void OnTimer( UINT_PTR eventId );
	afx_msg void OnWindowPosChanging( WINDOWPOS* wndPos );
	afx_msg void OnGetMinMaxInfo( MINMAXINFO* mmi );
	afx_msg void On_MdiClose( void );
	afx_msg void On_MdiCloseAll( void );
	afx_msg void OnUpdateAnyMDIChild( CCmdUI* pCmdUI );
	afx_msg void OnToggleMaximize( void );
	afx_msg void OnUpdateMaximize( CCmdUI* pCmdUI );
	afx_msg void CmLoggerOptions( void );
	afx_msg void CmRefreshContent( void );
	afx_msg void CmClearImageCache( void );
	afx_msg void On_RegisterImageAssoc( UINT cmdId );
	afx_msg void OnUpdate_RegisterImageAssoc( CCmdUI* pCmdUI );
	afx_msg void OnUpdateAlwaysEnabled( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // MainFrame_h
