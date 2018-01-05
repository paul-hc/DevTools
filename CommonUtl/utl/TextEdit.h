#ifndef TextEdit_h
#define TextEdit_h
#pragma once

#include "AccelTable.h"
#include "InternalChange.h"
#include "Range.h"


// inhibits EN_CHANGE notifications on internal changes;
// resets the modify flag when setting text programatically.

class CTextEdit : public CEdit
				, public CInternalChange
{
public:
	CTextEdit( bool useFixedFont = true );
	virtual ~CTextEdit();

	virtual bool HasInvalidText( void ) const;
	virtual bool NormalizeText( void );

	std::tstring GetText( void ) const;
	bool SetText( const std::tstring& text );
	void DDX_Text( CDataExchange* pDX, std::tstring& rValue, int ctrlId = 0 );
	void DDX_UiEscapeSeqs( CDataExchange* pDX, std::tstring& rValue, int ctrlId = 0 );

	bool IsWritable( void ) const { return !HasFlag( GetStyle(), ES_READONLY ); }
	bool SetWritable( bool writable ) { return writable != IsWritable() && SetReadOnly( !writable ) != FALSE; }

	bool UseFixedFont( void ) const { return m_useFixedFont; }
	void SetUseFixedFont( bool useFixedFont = true ) { ASSERT_NULL( m_hWnd ); m_useFixedFont = useFixedFont; }

	bool KeepSelOnFocus( void ) const { return m_keepSelOnFocus; }
	void SetKeepSelOnFocus( bool keepSelOnFocus = true ) { ASSERT_NULL( m_hWnd ); m_keepSelOnFocus = keepSelOnFocus; }		// also set ES_NOHIDESEL to retain selection (edit creation only)

	bool HookThumbTrack( void ) const { return m_keepSelOnFocus; }
	void SetHookThumbTrack( bool hookThumbTrack = true ) { m_hookThumbTrack = hookThumbTrack; }

	bool VisibleWhiteSpace( void ) const { return m_visibleWhiteSpace; }
	bool SetVisibleWhiteSpace( bool visibleWhiteSpace = true );

	void SelectAll( void ) { SetSel( 0, -1 ); }

	std::tstring GetSelText( void ) const;

	// multi-line edit
	enum { CaretPos = -1 };

	Range< int > GetLineRangeAt( int charPos = CaretPos ) const;		// line startPos and endPos
	std::tstring GetLineTextAt( int charPos = CaretPos ) const;			// line with the caret

	enum FontSize { Normal, Large };
	static CFont* GetFixedFont( FontSize fontSize = Normal );
	static void SetFixedFont( CWnd* pWnd );

	CScopedInternalChange MakeUserChange( void ) { return CScopedInternalChange( &m_userChange ); }
protected:
	bool IsInternalChange( void ) const;

	virtual void OnValueChanged( void );		// by the user
private:
	bool m_useFixedFont;
	bool m_keepSelOnFocus;						// true: keep old selection when focused; false: select all text when focused (default)
	bool m_hookThumbTrack;						// true: track thumb track scrolling events (edit controls don't send EN_VSCROLL on thumb track scrolling)
	bool m_visibleWhiteSpace;
	CAccelTable m_accel;
protected:
	CInternalChange m_userChange;				// allow derived classes to forces user change mode (when spinning, etc)

	// generated function overrides
	public:
	virtual void PreSubclassWindow( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	afx_msg UINT OnGetDlgCode( void );
	afx_msg void OnHScroll( UINT sbCode, UINT pos, CScrollBar* pScrollBar );
	afx_msg void OnVScroll( UINT sbCode, UINT pos, CScrollBar* pScrollBar );
	afx_msg BOOL OnEnChange_Reflect( void );
	afx_msg BOOL OnEnKillFocus_Reflect( void );
	afx_msg void OnSelectAll( void );

	DECLARE_MESSAGE_MAP()
};


class CImageEdit : public CTextEdit
{
public:
	CImageEdit( void );
	virtual ~CImageEdit();

	CImageList* GetImageList( void ) const { return m_pImageList; }
	void SetImageList( CImageList* pImageList );

	int GetImageIndex( void ) const { return m_imageIndex; }
	bool SetImageIndex( int imageIndex );

	bool HasValidImage( void ) const { return m_pImageList != NULL && m_imageIndex >= 0; }
protected:
	virtual void DrawImage( CDC* pDC, const CRect& imageRect );
private:
	void ResizeNonClient( void );
private:
	CImageList* m_pImageList;
	int m_imageIndex;
	CSize m_imageSize;
	CRect m_imageNcRect;

	enum { ImageSpacing = 0, ImageToTextGap = 2 };
public:
	// generated overrides
	public:
	virtual void PreSubclassWindow( void );
protected:
	afx_msg void OnNcCalcSize( BOOL calcValidRects, NCCALCSIZE_PARAMS* pNcSp );
	afx_msg void OnNcPaint( void );
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnNcLButtonDown( UINT hitTest, CPoint point );
	afx_msg void OnMove( int x, int y );

	DECLARE_MESSAGE_MAP()
};


#endif // TextEdit_h
