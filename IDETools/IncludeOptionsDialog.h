#ifndef IncludeOptionsDialog_h
#define IncludeOptionsDialog_h
#pragma once

#include "utl/UI/EnumComboBox.h"
#include "utl/UI/ItemContentEdit.h"
#include "utl/UI/LayoutDialog.h"
#include "utl/UI/SpinEdit.h"


struct CIncludeOptions;


class CIncludeOptionsDialog : public CLayoutDialog
{
public:
	CIncludeOptionsDialog( CIncludeOptions* pOptions, CWnd* pParent );
public:
	CIncludeOptions* m_pOptions;
private:
	// enum { IDD = IDD_INCLUDE_OPTIONS_DIALOG };

	CEnumComboBox m_depthLevelCombo;
	CSpinEdit m_maxParseLinesEdit;
	CItemListEdit m_ignoredEdit;				// CIncludeOptions::m_fnIgnored
	CItemListEdit m_addedEdit;					// CIncludeOptions::m_fnAdded
	CItemListEdit m_additionalIncPathEdit;		// CIncludeOptions::m_additionalIncludePath

	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual void OnOK( void );
protected:
	afx_msg void OnToggle_DelayedParsing( void );

	DECLARE_MESSAGE_MAP()
};


#endif // IncludeOptionsDialog_h
