#ifndef DemoSysTray_h
#define DemoSysTray_h
#pragma once


class CSystemTray;
class CTrayIcon;
namespace app { enum MsgType; }


class CDemoSysTray : public CCmdTarget
{
public:
	CDemoSysTray( CWnd* pOwner );
	virtual ~CDemoSysTray();
private:
	struct CMsg
	{
		std::tstring m_text;
		app::MsgType m_type;
	};

	static const CMsg& GetNextMessage( void );
private:
	CWnd* m_pOwner;
	CSystemTray* m_pSysTray;

	CTrayIcon* m_pMsgIcon;
	CTrayIcon* m_pAppIcon;
private:
	// message map functions
	afx_msg void On_MsgTray_ShowNextBalloon( void );
	afx_msg void OnUpdate_MsgTray_ShowNextBalloon( CCmdUI* pCmdUI );
	afx_msg void On_MsgTray_HideBalloon( void );
	afx_msg void OnUpdate_MsgTray_HideBalloon( CCmdUI* pCmdUI );
	afx_msg void On_MsgTray_Animate( void );
	afx_msg void OnUpdate_MsgTray_Animate( CCmdUI* pCmdUI );

	afx_msg void On_AppTray_ShowBalloon( void );
	afx_msg void OnUpdate_AppTray_ShowBalloon( CCmdUI* pCmdUI );
	afx_msg void On_AppTray_ToggleTrayIcon( void );
	afx_msg void OnUpdate_AppTray_ToggleTrayIcon( CCmdUI* pCmdUI );
	afx_msg void On_AppTray_FocusTrayIcon( void );
	afx_msg void OnUpdate_AppTray_FocusTrayIcon( CCmdUI* pCmdUI );
	afx_msg void On_AppTray_Animate( void );
	afx_msg void OnUpdate_AppTray_Animate( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // DemoSysTray_h
