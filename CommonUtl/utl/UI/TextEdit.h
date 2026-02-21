#ifndef TextEdit_h
#define TextEdit_h
#pragma once

#include "AccelTable.h"
#include "InternalChange.h"
#include "MultiValueBase.h"
#include "FrameHostCtrl.h"
#include "TextEdit_fwd.h"
#include "utl/Range.h"


class CSyncScrolling;


#define ON_EN_USER_SELCHANGE( id, memberFxn )	ON_CONTROL( CTextEdit::EN_USER_SELCHANGE, id, memberFxn )


// inhibits EN_CHANGE notifications on internal changes;
// resets the modify flag when setting text programatically.

class CTextEdit : public CFrameHostCtrl<CEdit>
	, public CInternalChange
	, public ui::CMultiValueBase
{
	typedef CFrameHostCtrl<CEdit> TBaseClass;

	// hidden base methods
	using TBaseClass::GetSel;
	using TBaseClass::SetSel;
	// multi-line hidden "overridden: methods
	using TBaseClass::LineFromChar;
	using TBaseClass::LineIndex;
	using TBaseClass::LineLength;
public:
	CTextEdit( bool useFixedFont = true );
	virtual ~CTextEdit();

	enum NotifCode { EN_USER_SELCHANGE = EN_CHANGE + 10 };

	void SetTextInputCallback( ui::ITextInput* pTextInputCallback ) { m_pTextInputCallback = pTextInputCallback; }
	void AddToSyncScrolling( CSyncScrolling* pSyncScrolling );

	virtual bool HasInvalidText( void ) const;
	virtual bool NormalizeText( void );

	std::tstring GetText( void ) const;
	bool SetText( const std::tstring& text );
	void DDX_Text( CDataExchange* pDX, std::tstring& rValue, int ctrlId = 0 );
	void DDX_UiEscapeSeqs( CDataExchange* pDX, std::tstring& rValue, int ctrlId = 0 );

	bool ReplaceText( const std::tstring& text, bool canUndo = true );

	bool IsMultiLine( void ) const { return HasFlag( GetStyle(), ES_MULTILINE ); }

	bool IsReadOnly( void ) const { return HasFlag( GetStyle(), ES_READONLY ); }
	bool IsWritable( void ) const { return !IsReadOnly(); }
	bool SetWritable( bool writable ) { return writable != IsWritable() && SetReadOnly( !writable ) != FALSE; }

	virtual bool InMultiValuesMode( void ) const implement;		// ui::CMultiValueBase implementation
	virtual bool SetMultiValuesMode( void );

	bool UseFixedFont( void ) const { return m_useFixedFont; }
	void SetUseFixedFont( bool useFixedFont = true ) { ASSERT_NULL( m_hWnd ); m_useFixedFont = useFixedFont; }

	bool KeepSelOnFocus( void ) const { return m_keepSelOnFocus; }
	void SetKeepSelOnFocus( bool keepSelOnFocus = true ) { ASSERT_NULL( m_hWnd ); m_keepSelOnFocus = keepSelOnFocus; }		// also set ES_NOHIDESEL to retain selection (edit creation only)

	bool UsePasteTransact( void ) const { return m_usePasteTransact; }
	void SetUsePasteTransact( bool usePasteTransact = true ) { ASSERT_NULL( m_hWnd ); m_usePasteTransact = usePasteTransact; }

	bool HookThumbTrack( void ) const { return m_hookThumbTrack; }
	void SetHookThumbTrack( bool hookThumbTrack = true ) { m_hookThumbTrack = hookThumbTrack; }

	bool VisibleWhiteSpace( void ) const { return m_visibleWhiteSpace; }
	bool SetVisibleWhiteSpace( bool visibleWhiteSpace = true );

	void SelectAll( void ) { SetSel( 0, -1 ); }

	std::tstring GetSelText( void ) const;

	// ignore last line if empty
	size_t GetEffectiveLineCount( void ) const { return GetEffectiveLineCount( GetText() ); }
	static size_t GetEffectiveLineCount( const std::tstring& text );
public:
	typedef int TCharPos;		// position of text character in edit's text
	typedef int TLineIndex;		// 0-based line number

	// client coordinates conversion
	CPoint GetCharPointTL( TCharPos charPos ) const { return PosFromChar( charPos ); }
	TCharPos GetCharFromPoint( const CPoint& clientPoint, TLineIndex* pLineIndex = nullptr ) const;

	enum { CaretPos = -1 };

	// multi-line edit:

	// base "overrides" in terms of line/char type aliases:
	TLineIndex LineFromChar( TCharPos charPos = CaretPos ) const { return CEdit::LineFromChar( charPos ); }
	TCharPos LineToCharPos( TLineIndex lineIndex = -1 ) const { return CEdit::LineIndex( lineIndex ); }
	int LineLength( TLineIndex lineIndex = -1 ) const { return CEdit::LineLength( lineIndex ); }
public:
	// exact caret position (opposite to selection anchor)
	TCharPos GetCaretCharPos( TLineIndex* pCaretLineIndex = nullptr ) const;
	bool SetCaretCharPos( TCharPos caretPos );

	TCharPos GetAnchorCharPos( TLineIndex* pAnchorLineIndex = nullptr ) const;		// selection anchor position (opposite to the caret)
	TLineIndex GetCaretLineIndex( void ) const { return LineFromChar( CaretPos ); }

	Range<TCharPos> GetLineRange( TLineIndex lineIndex ) const;			// line <startPos, endPos>
	std::tstring GetLineText( TLineIndex lineIndex ) const;

	Range<TCharPos> GetLineRangeAt( TCharPos charPos = CaretPos ) const { return GetLineRange( LineFromChar( charPos ) ); }
	std::tstring GetLineTextAt( TCharPos charPos = CaretPos ) const { return GetLineText( LineFromChar( charPos ) ); }		// by default: text of line with the caret

	// selection
	bool IsCaretAtSelStart( void ) const;
	Range<TCharPos> GetSelRange( bool swapIfCaretAtStart = false ) const;

	template< typename NumericT >
	Range<NumericT> GetSelRangeAs( bool swapIfCaretAtStart = false ) const { return GetSelRange( swapIfCaretAtStart ).AsRange<NumericT>(); }

	template< typename NumericT >
	bool SetSelRange( const Range<NumericT>& selRange, bool noCaretScroll = false );	// if range is inverted (not-normalized), the caret is placed to the START, otherwise to END

	void SetSel( TCharPos startChar, TCharPos endChar, BOOL noCaretScroll = false );	// has to stay BOOL to avoid overload resolution with base method

	// line selection
	bool ClearSelection( bool collapseToEnd = true );
	std::pair< Range<TCharPos>, bool > SelectLine( TLineIndex lineIndex );				// selects the entire line
	std::pair< Range<TCharPos>, bool > SelectLineRange( const Range<TLineIndex>& selLineRange );

	bool IsSelMultiLine( void ) const { return !GetSelLineRange().IsEmpty(); }
	Range<TLineIndex> LineRangeFromCharRange( const Range<TCharPos>& selRange, bool intuitiveSel = true ) const;
	Range<TLineIndex> GetSelLineRange( void ) const { return LineRangeFromCharRange( GetSelRange() ); }

	// top index
	TLineIndex GetTopLineIndex( void ) const { return CEdit::GetFirstVisibleLine(); }
	bool SetTopLineIndex( TLineIndex topLineIndex );

	enum FontSize { Normal, Large };
	static CFont* GetFixedFont( FontSize fontSize = Normal );
	static void SetFixedFont( CWnd* pWnd );

	void EnsureCaretVisible( void ) { PostMessage( EM_SCROLLCARET ); }		// the only way to scroll the edit in order to make the caret visible (if outside client rect)

	CScopedInternalChange MakeUserChange( void ) { return CScopedInternalChange( &m_userChange ); }
protected:
	bool IsInternalChange( void ) const;
	bool SetTextImpl( const std::tstring& text );	// retains Modified state as is
	bool RevertContents( void );			// kind of more targeted Undo()
	bool AdjustIntuitiveLineSelection( IN OUT Range<TLineIndex>* pSelLineRange, const Range<TCharPos>& selRange ) const;		// returns true if line selection changed

	virtual bool ValidateText( ui::CTextValidator& rValidator ) implement;
	virtual void OnValueChanged( void );	// by the user
	virtual COLORREF GetCustomTextColor( void ) const;	// override to allow customize text color (depending on text state, etc)
private:
	void _WatchSelChange( void );
private:
	bool m_useFixedFont;
	bool m_keepSelOnFocus;					// true: keep old selection when focused; false: select all text when focused (default)
	bool m_usePasteTransact;				// true: supress EN_CHANGE during WM_PASTE/WM_UNDO input, and send one EN_CHANGE after text pasted
	bool m_hookThumbTrack;					// true: track thumb track scrolling events (edit controls don't send EN_VSCROLL on thumb track scrolling)
	bool m_visibleWhiteSpace;
	CAccelTable m_accel;

	ui::ITextInput* m_pTextInputCallback;
	CSyncScrolling* m_pSyncScrolling;
	std::tstring m_lastValidText;
	Range<TCharPos> m_lastSelRange;
protected:
	CInternalChange m_userChange;			// allow derived classes to force user change mode (when spinning, etc)
public:
	static const TCHAR s_lineEnd[];

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	afx_msg UINT OnGetDlgCode( void );
	afx_msg HBRUSH CtlColor( CDC* pDC, UINT ctlColor );
	afx_msg void OnHScroll( UINT sbCode, UINT pos, CScrollBar* pScrollBar );
	afx_msg void OnVScroll( UINT sbCode, UINT pos, CScrollBar* pScrollBar );
	afx_msg LRESULT OnEmReplaceSel( WPARAM wParam, LPARAM lParam );
	afx_msg LRESULT OnPasteOrUndo( WPARAM, LPARAM );
	afx_msg BOOL OnEnChange_Reflect( void );
	afx_msg BOOL OnEnKillFocus_Reflect( void );
	afx_msg BOOL OnEnHScroll_Reflect( void );
	afx_msg BOOL OnEnVScroll_Reflect( void );
	virtual void OnEditCopy( void );
	afx_msg void OnEditCut( void );
	afx_msg void OnEditPaste( void );
	afx_msg void OnUpdateEditPaste( CCmdUI* pCmdUI );
	afx_msg void OnEditClear( void );
	afx_msg void OnEditClearAll( void );
	afx_msg void OnSelectAll( void );
	afx_msg void OnUpdate_HasSel( CCmdUI* pCmdUI );
	afx_msg void OnUpdate_Writeable( CCmdUI* pCmdUI );
	afx_msg void OnUpdate_WriteableHasSel( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


// CTextEdit template code

template< typename NumericT >
bool CTextEdit::SetSelRange( const Range<NumericT>& selRange, bool noCaretScroll /*= false*/ )
{
	Range<TCharPos> chSelRange( selRange );

	if ( chSelRange == GetSelRange() )
		return false;

	SetSel( chSelRange.m_start, chSelRange.m_end, noCaretScroll );
	return true;
}


#endif // TextEdit_h
