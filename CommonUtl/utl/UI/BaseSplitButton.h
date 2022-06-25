#ifndef BaseSplitButton_h
#define BaseSplitButton_h
#pragma once

#include "IconButton.h"


class CBaseSplitButton : public CIconButton
{
protected:
	CBaseSplitButton( void );
	virtual ~CBaseSplitButton();

	enum ButtonSide { LeftSide, RightSide };
	enum SplitState { Normal, HotButton, HotRhs, RhsPressed };

	SplitState GetSplitState( void ) const { return m_splitState; }
	bool SetSplitState( SplitState splitState );
public:
	// overridables
	virtual bool HasRhsPart( void ) const = 0;
	virtual CRect GetRhsPartRect( const CRect* pClientRect = NULL ) const = 0;
protected:
	virtual void DrawRhsPart( CDC* pDC, const CRect& clientRect );
	virtual void DrawFocus( CDC* pDC, const CRect& clientRect );

	void RedrawRhsPart( void );

	bool IsOwnerDraw( void ) const { return BS_OWNERDRAW == ( GetStyle() & BS_TYPEMASK ); }
private:
	SplitState m_splitState;
protected:
	enum { DefaultBorderSpacing = 1, FocusSpacing = 2 };

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
protected:
	virtual void DrawItem( DRAWITEMSTRUCT* pDrawItem );
protected:
	afx_msg void OnMouseMove( UINT flags, CPoint point );
	afx_msg LRESULT OnMouseLeave( WPARAM wParam, LPARAM lParam );
	afx_msg void OnPaint( void );

	DECLARE_MESSAGE_MAP()
};


#endif // BaseSplitButton_h
