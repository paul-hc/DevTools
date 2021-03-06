#ifndef BaseMainDialog_h
#define BaseMainDialog_h
#pragma once

#include "LayoutDialog.h"


abstract class CBaseMainDialog : public CLayoutDialog
{
protected:
	CBaseMainDialog( UINT templateId, CWnd* pParent = NULL );
public:
	virtual ~CBaseMainDialog();

	static void ParseCommandLine( int argc, TCHAR* argv[] );

	bool UseSysTrayMinimize( void ) const { return m_pSystemTrayInfo.get() != NULL; }
	bool NotifyTrayIcon( int notifyCode );

	void ShowAll( bool show );
protected:
	// base overrides
	virtual void PostRestorePlacement( int showCmd );
private:
	void _Minimize( void );
protected:
	struct CSysTrayInfo
	{
		CMenu m_popupMenu;
	};

	std::auto_ptr< CSysTrayInfo > m_pSystemTrayInfo;
public:
	enum { ShellIconId = 100 };

	static const UINT WM_TASKBARCREATED;
	static const UINT WM_TRAYICONNOTIFY;
public:
	// generated stuff
protected:
	virtual BOOL OnInitDialog( void );
	afx_msg void OnDestroy( void );
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg void OnSysCommand( UINT cmdId, LPARAM lParam );
	afx_msg void OnPaint( void );
	afx_msg HCURSOR OnQueryDragIcon( void );
	afx_msg LRESULT OnTrayIconNotify( WPARAM wParam, LPARAM lParam );
	LRESULT OnExplorerRestart( WPARAM, LPARAM );

	DECLARE_MESSAGE_MAP()
};


#endif // BaseMainDialog_h
