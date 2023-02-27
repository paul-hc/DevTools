#ifndef BaseFrameWnd_h
#define BaseFrameWnd_h
#pragma once

#include "ISystemTrayCallback.h"


class CSystemTray;


template< typename BaseWnd >
abstract class CBaseFrameWnd : public BaseWnd		// abstract base for main windows that can host a system tray icon
	, protected ui::ISystemTrayCallback
{
	typedef BaseWnd TBaseClass;
protected:
	CBaseFrameWnd( void ) : BaseWnd(), m_restoreToMaximized( false ) {}
public:
	virtual ~CBaseFrameWnd();

	bool UseSysTrayMinimize( void ) const { return m_trayPopupMenu.GetSafeHmenu() != nullptr; }
	const std::tstring& GetSection( void ) const { return m_regSection; }

	bool ShowAppWindow( int cmdShow );
protected:
	// ui::ISystemTrayCallback interface
	virtual CWnd* GetOwnerWnd( void ) override { return this; }
	virtual CMenu* GetTrayIconContextMenu( void ) override;
	virtual bool OnTrayIconNotify( UINT msgNotifyCode, UINT trayIconId, const CPoint& screenPos ) override;

	void SaveWindowPlacement( void );
	bool LoadWindowPlacement( CREATESTRUCT* pCreateStruct );
	virtual bool PersistPlacement( void ) const { return !m_regSection.empty(); }		// override to customize window placement persistence
protected:
	std::tstring m_regSection;
	std::auto_ptr<CSystemTray> m_pSystemTray;
	CMenu m_trayPopupMenu;
	persist bool m_restoreToMaximized;		// used during window placement loading

	// generated stuff
protected:
	virtual BOOL PreCreateWindow( CREATESTRUCT& rCreateStruct ) override;
protected:
	afx_msg void OnClose( void );
	afx_msg void OnAppRestore( void );
	afx_msg void OnUpdateAppRestore( CCmdUI* pCmdUI );
	afx_msg void OnAppMinimize( void );
	afx_msg void OnUpdateAppMinimize( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // BaseFrameWnd_h
