#ifndef TextEdit_h
#define TextEdit_h
#pragma once

#include "AccelTable.h"
#include "InternalChange.h"
#include "Range.h"
#include "BaseFrameHostCtrl.h"


class CSyncScrolling;


// inhibits EN_CHANGE notifications on internal changes;
// resets the modify flag when setting text programatically.

class CTextEdit : public CBaseFrameHostCtrl< CEdit >
				, public CInternalChange
{
	typedef CBaseFrameHostCtrl< CEdit > BaseClass;
public:
	CTextEdit( bool useFixedFont = true );
	virtual ~CTextEdit();

	void AddToSyncScrolling( CSyncScrolling* pSyncScrolling );

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

	bool HookThumbTrack( void ) const { return m_hookThumbTrack; }
	void SetHookThumbTrack( bool hookThumbTrack = true ) { m_hookThumbTrack = hookThumbTrack; }

	bool VisibleWhiteSpace( void ) const { return m_visibleWhiteSpace; }
	bool SetVisibleWhiteSpace( bool visibleWhiteSpace = true );

	void SelectAll( void ) { SetSel( 0, -1 ); }

	std::tstring GetSelText( void ) const;

	// multi-line edit
	typedef int CharPos;												// position of text character in edit's text
	typedef int Line;

	Range< CharPos > GetLineRange( Line linePos ) const;				// line startPos and endPos
	std::tstring GetLineText( Line linePos ) const;

	enum { CaretPos = -1 };

	Range< CharPos > GetLineRangeAt( CharPos charPos = CaretPos ) const { return GetLineRange( LineFromChar( charPos ) ); }
	std::tstring GetLineTextAt( CharPos charPos = CaretPos ) const { return GetLineText( LineFromChar( charPos ) ); }		// by default: text of line with the caret

	// selection
	template< typename IntType >
	Range< IntType > GetSelRange( void ) const;

	template< typename IntType >
	void SetSelRange( const Range< IntType >& sel ) { SetSel( static_cast< CharPos >( sel.m_start ), static_cast< CharPos >( sel.m_end ) ); }

	enum FontSize { Normal, Large };
	static CFont* GetFixedFont( FontSize fontSize = Normal );
	static void SetFixedFont( CWnd* pWnd );

	void EnsureCaretVisible( void ) { PostMessage( EM_SCROLLCARET ); }		// the only way to scroll the edit in order to make the caret visible (if outside client rect)

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

	CSyncScrolling* m_pSyncScrolling;
protected:
	CInternalChange m_userChange;				// allow derived classes to force user change mode (when spinning, etc)
public:
	static const TCHAR s_lineEnd[];

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
	afx_msg BOOL OnEnHScroll_Reflect( void );
	afx_msg BOOL OnEnVScroll_Reflect( void );
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


// template code

template< typename IntType >
Range< IntType > CTextEdit::GetSelRange( void ) const
{
	CharPos startPos, endPos;
	GetSel( startPos, endPos );
	ASSERT( startPos <= endPos && startPos >= 0 );
	return Range< IntType >( static_cast< IntType >( startPos ), static_cast< IntType >( endPos ) );
}


#endif // TextEdit_h
