
#ifndef PromptDialog_h
#define PromptDialog_h
#pragma once

#include "utl/LayoutDialog.h"


class CPromptDialog : public CLayoutDialog
{
public:
	CPromptDialog( HWND hWndTarget, const std::tstring& changedFields, CWnd* pParent = NULL );
	virtual ~CPromptDialog();

	bool CallSetWindowPos( void );
public:
	HWND m_hWndTarget;
	std::tstring m_changedFields;
private:
	static bool m_useSetWindowPos;
	static UINT m_swpFlags;
public:
	// enum { IDD = IDD_APPLY_CHANGES_DIALOG };
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlType );
	virtual void OnOK( void );
	afx_msg void CkUseSetWindowPos( void );

	DECLARE_MESSAGE_MAP()
};


#endif // PromptDialog_h
