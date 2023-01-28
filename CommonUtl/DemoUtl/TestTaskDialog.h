#pragma once

#include "utl/UI/LayoutDialog.h"
#include "utl/UI/EnumSplitButton.h"


namespace ui { class CTaskDialog; }
class CEnumTags;


class CTestTaskDialog : public CLayoutDialog
{
public:
	CTestTaskDialog( CWnd* pParent );
	virtual ~CTestTaskDialog();
private:
	enum TaskDialogUsage { TD_Basic, TD_CommandLinks, TD_MessageBox, TD_ProgressBar, TD_MarqueeProgressBar, TD_Navigation, TD_Complete };

	static const CEnumTags& GetTags_TaskDialogUsage( void );

	void DisplayOutcome( void );
	void ClearOutcome( void );

	ui::CTaskDialog* MakeTaskDialog_Basic( void ) const;
	ui::CTaskDialog* MakeTaskDialog_CommandLinks( void ) const;
	ui::CTaskDialog* MakeTaskDialog_MessageBox( void ) const;
	ui::CTaskDialog* MakeTaskDialog_ProgressBar( void ) const;
	ui::CTaskDialog* MakeTaskDialog_MarqueeProgressBar( void ) const;
	ui::CTaskDialog* MakeTaskDialog_Navigation( void ) const;
	ui::CTaskDialog* MakeTaskDialog_Complete( void ) const;
private:
	INT_PTR m_outcome;
	int m_radioButtonId;
	bool m_verificationChecked;

	static const std::tstring s_mainInstruction;
	static const std::tstring s_footer;
	static const std::tstring s_verificationText;
private:
	// enum { IDD = IDD_TEST_TASK_DIALOG };

	CEnumSplitButton m_usageButton;
public:
	// generated overrides
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	// message map functions
	afx_msg void OnDestroy( void );
	afx_msg void OnBnClicked_TaskUsage( void );

	DECLARE_MESSAGE_MAP()
};
