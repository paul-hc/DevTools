#ifndef AboutBox_h
#define AboutBox_h
#pragma once

#include "LayoutDialog.h"


class CLinkStatic;
class CReportListControl;


// dialog template stored in utl/UI/utl_ui.rc, project specific info in VS_VERSION_INFO resource

class CAboutBox : public CLayoutDialog
{
public:
	CAboutBox( CWnd* pParent );
	virtual ~CAboutBox();
private:
	void SetupBuildInfoList( void );
public:
	static UINT m_appIconId;
private:
	std::tstring m_executablePath;

	// enum { IDD = IDD_ABOUT_BOX };
	CStatic m_appIconStatic;
	std::auto_ptr< CLinkStatic > m_pEmailStatic;
	std::auto_ptr< CReportListControl > m_pBuildInfoList;

	// generated overrides
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnExploreExecutable( void );

	DECLARE_MESSAGE_MAP()
};


#endif // AboutBox_h
