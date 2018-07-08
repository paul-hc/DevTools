#ifndef BaseFrameHostCtrl_h
#define BaseFrameHostCtrl_h
#pragma once


// abstract base for controls with an optional frame OR focus rect (edits, combos, etc)

template< typename BaseCtrl >
abstract class CBaseFrameHostCtrl : public BaseCtrl
{
protected:
	CBaseFrameHostCtrl( COLORREF frameColor = CLR_NONE, bool showFocus = false )
		: BaseCtrl()
		, m_frameColor( frameColor )
		, m_showFocus( showFocus )
		, m_frameMargins( 0, 0 )
		, m_focusMargins( 0, 0 )
	{
	}
public:
	bool SetFrameColor( COLORREF frameColor );
	bool SetShowFocus( bool showFocus = true );

	void SetFrameMargins( int cx = 0, int cy = 0 ) { ASSERT_NULL( m_hWnd ); m_frameMargins.cx = cx; m_frameMargins.cy = cy; }
	void SetFocusMargins( int cx = 0, int cy = 0 ) { ASSERT_NULL( m_hWnd ); m_focusMargins.cx = cx; m_focusMargins.cy = cy; }
private:
	COLORREF m_frameColor;
	bool m_showFocus;
	CSize m_frameMargins;
	CSize m_focusMargins;

	// generated stuff
protected:
	afx_msg void OnPaint( void );
	afx_msg void OnSetFocus( CWnd* pOldWnd );
	afx_msg void OnKillFocus( CWnd* pNewWnd );

	DECLARE_MESSAGE_MAP()
};


#include "BaseFrameHostCtrl.hxx"


#endif // BaseFrameHostCtrl_h
