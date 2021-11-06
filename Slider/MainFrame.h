#ifndef MainFrame_h
#define MainFrame_h
#pragma once

#include "utl/UI/InternalChange.h"
#include "utl/UI/WindowTimer.h"


interface IImageView;
class CBaseZoomView;
class CMainToolbar;


class CMainFrame : public CMDIFrameWnd
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
	CStatusBar* GetStatusBar( void ) { return &m_statusBar; }
	CProgressCtrl* GetProgressCtrl( void ) { return &m_progressCtrl; }

	// status bar temporary messages
	bool IsStatusBarAutoClear( void ) const { return m_messageClearTimer.IsStarted(); }
	bool CancelStatusBarAutoClear( UINT idleMessageID = AFX_IDS_IDLEMESSAGE );
	void SetStatusBarMessage( const TCHAR* pMessage, UINT elapseMs = UINT_MAX );
	void SetIdleStatusBarMessage( UINT idleMessageID = AFX_IDS_IDLEMESSAGE );

	void StartEnqueuedAlbumTimer( UINT timerDelay = 750 );

	// shared progress bar
	bool InProgress( void ) const { return m_inProgress.IsInternalChange(); }
	void BeginProgress( int valueMin, int count, int stepCount, const TCHAR* pCaption = NULL );
	void EndProgress( int clearDelay );
	void SetPosProgress( int value );
	void StepItProgress( void );

	bool ResizeViewToFit( CBaseZoomView* pZoomScrollView );
protected:
	void CleanupWindow( void );
private:
	bool CreateProgressCtrl( void );
	void SetProgressCaptionText( const TCHAR* pCaption );
	bool DoClearProgressCtrl( void );
private:
	std::auto_ptr<CMainToolbar> m_pToolbar;
	CStatusBar m_statusBar;
	CProgressCtrl m_progressCtrl;

	CWindowTimer m_messageClearTimer;
	CWindowTimer m_ddeEnqueuedTimer;				// monitors enqueued image paths
	CWindowTimer m_progBarResetTimer;
	CInternalChange m_inProgress;

	enum Metrics { ProgressBarWidth = 150 };
	enum TimerIds { MessageTimerId = 2000, QueueTimerId, ProgressResetTimerId };

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
	afx_msg void OnSize( UINT sizeType, int cx, int cy );
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
	afx_msg void OnUpdateAlwaysEnabled( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // MainFrame_h
