#ifndef BaseMainDialog_h
#define BaseMainDialog_h
#pragma once

#include "LayoutDialog.h"
#include "ISystemTrayCallback.h"


class CSystemTray;


abstract class CBaseMainDialog : public CLayoutDialog
	, protected ui::ISystemTrayCallback
{
protected:
	CBaseMainDialog( UINT templateId, CWnd* pParent = NULL );
public:
	virtual ~CBaseMainDialog();

	static void ParseCommandLine( int argc, TCHAR* argv[] );

	bool UseSysTrayMinimize( void ) const { return m_trayPopupMenu.GetSafeHmenu() != NULL; }

	void ShowAll( bool show );
protected:
	// base overrides
	virtual void PostRestorePlacement( int showCmd, bool restoreToMaximized ) override;
private:
	// ui::ISystemTrayCallback interface
	virtual CWnd* GetOwnerWnd( void ) override;
	virtual CMenu* GetTrayIconContextMenu( void ) override;
	virtual bool OnTrayIconNotify( UINT msgNotifyCode, UINT trayIconId, const CPoint& screenPos ) override;

	void _Minimize( bool restoreToMaximized );
protected:
	CMenu m_trayPopupMenu;
	std::auto_ptr<CSystemTray> m_pSystemTray;
public:
	enum { ShellIconId = 100 };

	// generated stuff
protected:
	virtual BOOL OnInitDialog( void ) override;
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg void OnPaint( void );
	afx_msg HCURSOR OnQueryDragIcon( void );
	afx_msg void OnAppRestore( void );
	afx_msg void OnUpdateAppRestore( CCmdUI* pCmdUI );
	afx_msg void OnAppMinimize( void );
	afx_msg void OnUpdateAppMinimize( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // BaseMainDialog_h
