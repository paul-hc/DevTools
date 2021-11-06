#ifndef FlagsListPage_h
#define FlagsListPage_h
#pragma once

#include "utl/UI/LayoutPropertyPage.h"
#include "BaseFlagsCtrl.h"


class CBaseFlagsCtrl;
class CFlagsListCtrl;


class CFlagsListPage : public CLayoutPropertyPage
{
public:
	CFlagsListPage( ui::IEmbeddedPageCallback* pParentCallback, const TCHAR* pTitle = NULL );
	virtual ~CFlagsListPage();

	CBaseFlagsCtrl* GetFlagsCtrl( void );

	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;
private:
	ui::IEmbeddedPageCallback* m_pParentCallback;
private:
	// enum { IDD = IDD_FLAG_LIST_PAGE };

	std::auto_ptr<CFlagsListCtrl> m_pFlagsListCtrl;
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	// message map functions
	afx_msg void OnFnFlagsChanged_FlagsList( void );

	DECLARE_MESSAGE_MAP()
};


#endif // FlagsListPage_h
