#ifndef StatusBarProgressService_h
#define StatusBarProgressService_h
#pragma once

#include "utl/IProgressService.h"


class CMFCStatusBar;


class CStatusBarProgressService : public utl::IStatusProgressService
	, private utl::noncopyable
{
public:
	CStatusBarProgressService( size_t maxPos, size_t pos = 0 );
	~CStatusBarProgressService();

	static void InitStatusBarInfo( CMFCStatusBar* pStatusBar, int progPaneIndex, int labelPaneIndex = -1 );		// called after status bar creation in main frame creation

	// display options
	void SetAutoHide( bool autoHide = true ) { m_autoHide = autoHide; }
	void SetAutoWrap( bool autoWrap = true ) { m_autoWrap = autoWrap; }
	void SetDisplayText( bool displayText = true );		// inside the progress bar

	// utl::IStatusProgressService interface
	virtual bool SetLabelText( const std::tstring& text, COLORREF labelTextColor = CLR_DEFAULT ) implement;
	virtual size_t GetPos( void ) const implement;
	virtual void SetPos( size_t pos ) implement;

	// progress
	bool IsOpen( void ) const { return m_isOpen; }
	bool Close( void );

	void GotoEnd( void ) { SetPos( m_maxPos ); }
private:
	bool Open( void );

	static bool IsInit( void );
	static bool IsPaneProgressOn( void );
	static void HideProgressPanes( void );
private:
	size_t m_maxPos;

	bool m_autoHide;		// label + progress bar
	bool m_autoWrap;		// reset to 0 if it overflows
	bool m_isOpen;			// progress bar displayed?

	static CMFCStatusBar* s_pStatusBar;
	static int s_progressPaneIndex;
	static int s_labelPaneIndex;
	static int s_progressPaneWidth;
};



#endif // StatusBarProgressService_h
