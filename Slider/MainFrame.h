#ifndef MainFrame_h
#define MainFrame_h
#pragma once

#include "utl/UI/WindowTimer.h"
#include "utl/UI/BaseMainFrameWndEx.h"
#include "utl/UI/IZoomBar.h"
#include "INavigationBar.h"


interface IImageView;
class CBaseZoomView;
class CMainToolbar;

typedef CBaseMainFrameWndEx<CMDIFrameWndEx> TMDIFrameWndEx;

enum StatusBarFields { Status_Info, Status_ProgressLabel, Status_Progress };


class CMainFrame : public TMDIFrameWndEx
	, public ui::IZoomBar
	, public INavigationBar
{
	DECLARE_DYNAMIC( CMainFrame )
public:
	CMainFrame( void );
	virtual ~CMainFrame();

	size_t GetMdiChildCount( void ) const;
	IImageView* GetActiveImageView( void ) const;
	bool IsViewActive( IImageView* pImageView ) const { return GetActiveImageView() == pImageView; }
	bool IsMdiRestored( void ) const;			// current document not maximized?

	CMFCToolBar* GetStandardToolbar( void ) { return &m_standardToolBar; }
	CMFCStatusBar* GetStatusBar( void ) { return &m_statusBar; }
		CMainToolbar* GetOldToolbar( void ) { return m_pOldToolbar.get(); }

	ui::IZoomBar* GetZoomBar( void ) { return this; }
	INavigationBar* GetNavigationBar( void ) { return this; }

	// status bar temporary messages
	bool CancelStatusBarAutoClear( UINT idleMessageID = AFX_IDS_IDLEMESSAGE );
	void SetStatusBarMessage( const TCHAR* pMessage, UINT elapseMs = UINT_MAX );
	void SetIdleStatusBarMessage( UINT idleMessageID = AFX_IDS_IDLEMESSAGE );

	void StartEnqueuedAlbumTimer( UINT timerDelay = 750 );

	bool ResizeViewToFit( CBaseZoomView* pZoomScrollView );
protected:
	void CleanupWindow( void );
private:
	// ui::IZoomBar interface
	virtual bool OutputScalingMode( ui::ImageScalingMode scalingMode );
	virtual ui::ImageScalingMode InputScalingMode( void ) const;
	virtual bool OutputZoomPct( UINT zoomPct );
	virtual UINT InputZoomPct( ui::ComboField byField ) const;		// return 0 on error

	// INavigationBar interface
	virtual bool OutputNavigRange( UINT imageCount );
	virtual bool OutputNavigPos( int imagePos );
	virtual int InputNavigPos( void ) const;

	void HandleResetToolbar( UINT toolBarResId );
private:
	CMFCMenuBar	m_menuBar;
	CMFCToolBar m_standardToolBar;
	CMFCToolBar m_navigateToolBar;
	CMFCStatusBar m_statusBar;


		// obsolete
		std::auto_ptr<CMainToolbar> m_pOldToolbar;

	CWindowTimer m_messageClearTimer;
	CWindowTimer m_ddeEnqueuedTimer;				// monitors enqueued image paths

	enum Metrics { ProgressBarWidth = 100 };
	enum TimerIds { MessageTimerId = 2000, QueueTimerId };
	enum { ScalingModeComboWidth = 130, ZoomComboWidth = 90, SmoothCheckWidth = 65, NavigSliderCtrlWidth = 150 };

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
	afx_msg LRESULT OnResetToolbar( WPARAM toolBarResId, LPARAM );
	afx_msg void OnUpdateAlwaysEnabled( CCmdUI* pCmdUI );
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
	afx_msg void On_FocusOnZoomCombo( void );

	DECLARE_MESSAGE_MAP()
};


#endif // MainFrame_h
