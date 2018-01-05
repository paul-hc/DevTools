#ifndef DetailBasePage_h
#define DetailBasePage_h
#pragma once

#include "utl/LayoutPropertyPage.h"
#include "Observers.h"


class CDetailBasePage : public CLayoutPropertyPage
					  , public IWndDetailObserver
{
protected:
	CDetailBasePage( UINT templateId );
public:
	virtual ~CDetailBasePage();

	// base overrides
	virtual void SetModified( bool changed = true );
private:
	std::tstring m_pageTitle;
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
};


#endif // DetailBasePage_h
