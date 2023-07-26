#ifndef MenuPickerButton_h
#define MenuPickerButton_h
#pragma once

#include <afxmenubutton.h>


class CMenuPickerButton : public CMFCMenuButton
{
public:
	CMenuPickerButton( CWnd* pTargetWnd = nullptr );
	virtual ~CMenuPickerButton();

	void SetTargetWnd( CWnd* pTargetWnd ) { m_pTargetWnd = pTargetWnd; }
	CWnd* GetTargetWnd( void ) const;
private:
	CWnd* m_pTargetWnd;			// if null, parent dialog is the target

	// base overrides:
protected:
	virtual void OnShowMenu( void ) override;

	// generated stuff
protected:
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );

	DECLARE_MESSAGE_MAP()
};


#endif // MenuPickerButton_h
