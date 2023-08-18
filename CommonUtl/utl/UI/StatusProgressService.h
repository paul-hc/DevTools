#ifndef StatusProgressService_h
#define StatusProgressService_h
#pragma once

#include "utl/IProgressService.h"


class CMFCStatusBar;
class CDelayedHideImpl;


class CStatusProgressService : public utl::IStatusProgressService
	, private utl::noncopyable
{
	friend class CDelayedHideImpl;
public:
	CStatusProgressService( size_t maxPos = 0, size_t pos = 0 );	// if maxPos=0, progress is not activated
	~CStatusProgressService();

	void StartProgress( size_t maxPos, size_t pos = 0 );			// lazy initialization, when using the default constructor

	enum { DefaultHideDelayMs = 3000 };

	// display options
	void SetAutoHide( bool autoHide = true ) { m_autoHide = autoHide; }
	void SetAutoWrap( bool autoWrap = true ) { m_autoWrap = autoWrap; }
	void SetHideElapseMs( UINT hideDelayMs = DefaultHideDelayMs ) { m_hideDelayMs = hideDelayMs; }
	void SetDisplayText( bool displayText = true );			// inside the progress bar

	// utl::IStatusProgressService interface
	virtual bool SetLabelText( const std::tstring& text, COLORREF labelTextColor = CLR_DEFAULT, COLORREF labelBkColor = CLR_DEFAULT ) implement;
	virtual size_t GetMaxPos( void ) const implement;
	virtual size_t GetPos( void ) const implement;
	virtual void SetPos( size_t pos ) implement;

	// progress
	bool IsActive( void ) const { return m_isActive; }
	bool Deactivate( void );	// stop progress

	static void InitStatusBarInfo( CMFCStatusBar* pStatusBar, int progPaneIndex, int labelPaneIndex = -1 );		// called after status bar creation in main frame creation
private:
	bool Activate( void );		// start progress

	static bool IsInit( void );
	static bool IsPaneProgressOn( void );
	static bool HideProgressPanes( void );
	static void HideProgressPanesImpl( void );
private:
	size_t m_maxPos;

	bool m_autoHide;		// label + progress bar
	bool m_autoWrap;		// reset to 0 if it overflows
	bool m_isActive;		// progress bar displayed?
	UINT m_hideDelayMs;		// timeout for hiding the progress panes: 0 for instant closing, UINT_MAX to keep it on indefinitely

	static CMFCStatusBar* s_pStatusBar;
	static int s_progressPaneIndex;
	static int s_labelPaneIndex;
	static int s_progressPaneWidth;
};


#endif // StatusProgressService_h
