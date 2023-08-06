#ifndef AboutBox_h
#define AboutBox_h
#pragma once

#include "utl/MultiThreading.h"
#include "utl/Path.h"
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
	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;

	const fs::CPath* GetSelPath( void ) const;
	void SetupBuildInfoList( void );
	void AddBuildInfoPair( int pos, const TCHAR* pProperty, const std::tstring& value, const void* pItemData = nullptr );
public:
	static UINT s_appIconId;
private:
	st::CScopedInitializeOle m_scopedOle;		// enable clipboard support

	fs::CPath m_modulePath;
	fs::CPath m_exePath;
	std::tstring m_buildTime;

	// enum { IDD = IDD_ABOUT_BOX };
	CStatic m_appIconStatic;
	std::auto_ptr<CLinkStatic> m_pEmailStatic;
	std::auto_ptr<CReportListControl> m_pBuildInfoList;

	enum Column { Property, Value };

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnLvnItemChanged_ListItems( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnExploreModule( void );
	afx_msg void OnUpdateExploreModule( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // AboutBox_h
