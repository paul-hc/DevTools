#pragma once

#include "utl/UI/LayoutPropertySheet.h"
#include "ITestMarkup.h"


class CTestPropertySheet : public CLayoutPropertySheet
						 , public ITestMarkup
{
public:
	CTestPropertySheet( void );				// modeless
	CTestPropertySheet( CWnd* pParent );	// modal
	virtual ~CTestPropertySheet();
private:
	void Construct( void );
};
