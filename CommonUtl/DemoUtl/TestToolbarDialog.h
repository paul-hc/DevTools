#pragma once

#include "utl/UI/LayoutDialog.h"
#include "utl/UI/DialogToolBar.h"


class CTestToolbarDialog : public CLayoutDialog
{
public:
	CTestToolbarDialog( CWnd* pParent );
	virtual ~CTestToolbarDialog();
private:
	void RegisterOwnCmds( void );
private:
	// enum { IDD = IDD_TEST_TOOLBAR_DIALOG };
	enum OwnID { IdFileNew = 60, IdFileOpen, IdFileSave, IdEditCut, IdEditCopy, IdEditPaste, IdFilePrint, IdAppAbout };

	CDialogToolBar m_toolbarStdEnabled;
	CDialogToolBar m_toolbarStdDisabled;

	CDialogToolBar m_toolbarDisabledGrayScale;
	CDialogToolBar m_toolbarDisabledGray;
	CDialogToolBar m_toolbarDisabledEffect;
	CDialogToolBar m_toolbarDisabledBlendColor;

	// generated overrides
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	// message map functions
	afx_msg void OnDestroy( void );
	afx_msg void OnUpdateButton( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};
