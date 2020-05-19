#ifndef ListCtrlEditorHost_h
#define ListCtrlEditorHost_h
#pragma once

#include "AccelTable.h"


class CReportListControl;


class CListCtrlEditorHost : public CCmdTarget
						  , private utl::noncopyable
{
public:
	CListCtrlEditorHost( CReportListControl* pListCtrl );
	virtual ~CListCtrlEditorHost();
private:
	CReportListControl* m_pListCtrl;
	CAccelTable m_listAccel;

	// generated overrides
public:
	virtual bool HandleTranslateMessage( MSG* pMsg );
protected:
	DECLARE_MESSAGE_MAP()
};


#endif // ListCtrlEditorHost_h
