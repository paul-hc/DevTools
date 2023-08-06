#ifndef MainDialog_h
#define MainDialog_h
#pragma once

#include "utl/UI/BaseMainDialog.h"
#include "utl/UI/DialogToolBar.h"
#include "utl/UI/IconButton.h"
#include "utl/UI/InternalChange.h"
#include "utl/UI/LayoutChildPropertySheet.h"
#include "utl/UI/PopupSplitButton.h"
#include "utl/UI/SplitPushButton.h"
#include "utl/UI/WindowTimer.h"
#include "wnd/WndSearchPattern.h"
#include "Observers.h"
#include "TrackWndPickerStatic.h"
#include "WndInfoEdit.h"


class CWndHighlighter;
enum DetailPage;


class CMainDialog : public CBaseMainDialog
	, public IWndObserver
	, public IEventObserver
	, private CInternalChange
{
public:
	CMainDialog( void );
	virtual ~CMainDialog();

	CWindowTimer* GetAutoUpdateTimer( void ) { return &m_autoUpdateTimer; }

	void DrillDownDetail( DetailPage detailPage );
private:
	// IWndObserver interface
	virtual void OnTargetWndChanged( const CWndSpot& targetWnd );

	// IEventObserver interface
	virtual void OnAppEvent( app::Event appEvent );

	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;
protected:
	// base overrides
	virtual void LoadFromRegistry( void );
	virtual void SaveToRegistry( void );
	virtual void OnCollapseChanged( bool collapsed );
public:
	virtual const CIcon* GetDlgIcon( DlgIcon dlgIcon = DlgSmallIcon ) const;
private:
	void FlashTargetWnd( int flashCount );
	void AutoUpdateRefresh( void );

	void SearchUpdateTarget( void );
	void SearchWindow( void );
	void SearchNextWindow( bool forward = true );

	bool ApplyDetailChanges( bool promptOptions );
	std::tstring MakeDirtyString( void ) const;
private:
	enum { TimerAutoUpdate = 100, TimerResetRefreshButton };

	CWndSearchPattern m_searchPattern;
	CWindowTimer m_autoUpdateTimer, m_refreshTimer;
	std::auto_ptr<CWndHighlighter> m_pFlashHighlighter;
private:
	// enum { IDD = IDD_MAIN_DIALOG };

	CLayoutChildPropertySheet m_mainSheet;

	CDialogToolBar m_optionsToolbar;
	CTrackWndPickerStatic m_trackWndPicker;
	CWndInfoEdit m_briefInfoEdit;
	CButton m_highlightButton;
	CIconButton m_refreshButton;
	CPopupSplitButton m_findButton;
	CPopupSplitButton m_detailsButton;
	CSplitPushButton m_applyButton;
	CLayoutChildPropertySheet m_detailsSheet;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	virtual BOOL OnInitDialog( void );
	afx_msg void OnDestroy( void );
	afx_msg HBRUSH OnCtlColor( CDC* dc, CWnd* pWnd, UINT ctlColor );
	afx_msg void OnTimer( UINT_PTR eventId );
	afx_msg void OnToggleCollapseDetails( void );
	afx_msg void OnTsnBeginTracking_WndPicker( void );
	afx_msg void OnTsnEndTracking_WndPicker( void );
	afx_msg void OnTsnTrack_WndPicker( void );
	afx_msg void OnHighlight( void );
	afx_msg LRESULT OnEndTimerSequence( WPARAM eventId, LPARAM );
	afx_msg void CmUpdateTick( void );
	afx_msg void OnFindWindow( void );
	afx_msg void OnFindNextWindow( void );
	afx_msg void OnFindPrevWindow( void );
	afx_msg void OnApplyNow( void );
	afx_msg void OnSbnRightClicked_ApplyNow( void );
	afx_msg void CmViewMainPage( UINT cmdId );
	afx_msg void CmEditDetails( UINT cmdId );

	DECLARE_MESSAGE_MAP()
};


#endif // MainDialog_h
