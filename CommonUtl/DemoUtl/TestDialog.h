#pragma once

#include "utl/UI/LayoutDialog.h"
#include "ITestMarkup.h"


class CDemoTemplate;


class CTestDialog : public CLayoutDialog
				  , public ITestMarkup
{
public:
	CTestDialog( CWnd* pParent );
	virtual ~CTestDialog();
private:
	// enum { IDD = IDD_DEMO_DIALOG };

	std::auto_ptr<CDemoTemplate> m_pDemo;
protected:
	virtual void OnIdleUpdateControls( void );

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	DECLARE_MESSAGE_MAP()
};
