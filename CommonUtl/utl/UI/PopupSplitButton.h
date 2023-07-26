#ifndef PopupSplitButton_h
#define PopupSplitButton_h
#pragma once

#include "BaseSplitButton.h"
#include "ThemeItem.h"


class CPopupSplitButton : public CBaseSplitButton
{
public:
	CPopupSplitButton( UINT menuId = 0, int popupIndex = 0 );
	virtual ~CPopupSplitButton();

	void LoadMenu( UINT menuId, int popupIndex );

	CWnd* GetTargetWnd( void ) const { return m_pTargetWnd; }
	void SetTargetWnd( CWnd* pTargetWnd ) { m_pTargetWnd = pTargetWnd; }

	bool IsThemed( void ) const { return m_dropItem.IsThemed(); }
public:
	virtual void DropDown( void );

	// base overrides
	virtual bool HasRhsPart( void ) const { return m_contextMenu.GetSafeHmenu() != nullptr; }
	virtual CRect GetRhsPartRect( const CRect* pClientRect = nullptr ) const;
protected:
	virtual void DrawRhsPart( CDC* pDC, const CRect& clientRect );
protected:
	enum { DropWidth = 18 };

	CMenu m_contextMenu;
	CWnd* m_pTargetWnd;				// receives popup menu commands
private:
	CThemeItem m_dropItem;
protected:
	// generated overrides
protected:
	// message map functions
	afx_msg void OnLButtonDown( UINT flags, CPoint point );

	DECLARE_MESSAGE_MAP()
};


#endif // PopupSplitButton_h
