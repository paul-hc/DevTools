#ifndef IncludeOptionsDialog_h
#define IncludeOptionsDialog_h
#pragma once

#include "utl/ItemContentEdit.h"
#include "utl/LayoutDialog.h"
#include "utl/SpinEdit.h"


struct CIncludeOptions;


class CIncludeOptionsDialog : public CLayoutDialog
{
public:
	CIncludeOptionsDialog( CIncludeOptions* pOptions, CWnd* pParent );
public:
	CIncludeOptions* m_pOptions;
private:
	// enum { IDD = IDD_INCLUDE_OPTIONS_DIALOG };

	CComboBox m_depthLevelCombo;
	CSpinEdit m_maxParseLinesEdit;
	CItemListEdit m_excludedEdit;
	CItemListEdit m_includedEdit;
	CItemListEdit m_additionalIncPathEdit;

	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void CkDelayedParsing( void );

	DECLARE_MESSAGE_MAP()
};


#endif // IncludeOptionsDialog_h
