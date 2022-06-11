#ifndef FrameHostCtrl_h
#define FrameHostCtrl_h
#pragma once


// abstract base for controls with an optional frame OR focus rect (edits, combos, etc)

template< typename BaseCtrl >
class CFrameHostCtrl : public BaseCtrl
{
public:
	CFrameHostCtrl( COLORREF frameColor = CLR_NONE, bool showFocus = false )
		: BaseCtrl()
		, m_frameColor( frameColor )
		, m_showFocus( showFocus )
		, m_frameMargins( 0, 0 )
		, m_focusMargins( 0, 0 )
	{
	}

	COLORREF GetFrameColor( void ) const { return frameColor; }
	bool SetFrameColor( COLORREF frameColor );

	bool GetShowFocus( void ) const { return m_showFocus; }
	bool SetShowFocus( bool showFocus = true );

	void SetFrameMargins( int cx = 0, int cy = 0 ) { m_frameMargins.cx = cx; m_frameMargins.cy = cy; Refresh(); }
	void SetFocusMargins( int cx = 0, int cy = 0 ) { m_focusMargins.cx = cx; m_focusMargins.cy = cy; Refresh(); }

	enum FrameType { SolidFrame, FocusFrame };

	CRect GetFrameRect( FrameType frameType ) const;
	CWnd* InvalidateFrame( FrameType frameType );
protected:
	void Refresh( void );
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


#include "FrameHostCtrl.hxx"


#endif // FrameHostCtrl_h
