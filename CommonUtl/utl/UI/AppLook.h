#ifndef AppLook_h
#define AppLook_h
#pragma once


namespace app
{
	enum AppLook { Windows_2000, Office_XP, Windows_XP, Office_2003, VS_2005, VS_2008, Office_2007_Blue, Office_2007_Black, Office_2007_Silver, Office_2007_Aqua, Windows_7 };
}


class CAppLook : public CCmdTarget
{
public:
	CAppLook( app::AppLook appLook );
	virtual ~CAppLook();

	void SetAppLook( app::AppLook appLook );
	void Save( void );

	static app::AppLook FromId( UINT cmdId );
private:
	persist app::AppLook m_appLook;

	// generated stuff
protected:
	afx_msg void OnApplicationLook( UINT cmdId );
	afx_msg void OnUpdateApplicationLook( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // AppLook_h
