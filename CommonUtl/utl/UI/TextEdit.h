#ifndef TextEdit_h
#define TextEdit_h
#pragma once

#include "AccelTable.h"
#include "InternalChange.h"
#include "Range.h"
#include "FrameHostCtrl.h"


class CSyncScrolling;


#define ON_EN_USERSELCHANGE( id, memberFxn )	ON_CONTROL( CTextEdit::EN_USERSELCHANGE, id, memberFxn )


// inhibits EN_CHANGE notifications on internal changes;
// resets the modify flag when setting text programatically.

class CTextEdit : public CFrameHostCtrl<CEdit>
	, public CInternalChange
{
	typedef CFrameHostCtrl<CEdit> TBaseClass;

	using TBaseClass::SetSel;		// hidden base methods
public:
	CTextEdit( bool useFixedFont = true );
	virtual ~CTextEdit();

	enum NotifCode { EN_USERSELCHANGE = EN_CHANGE + 10 };

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

	bool UsePasteTransact( void ) const { return m_usePasteTransact; }
	void SetUsePasteTransact( bool usePasteTransact = true ) { ASSERT_NULL( m_hWnd ); m_usePasteTransact = usePasteTransact; }

	bool HookThumbTrack( void ) const { return m_hookThumbTrack; }
	void SetHookThumbTrack( bool hookThumbTrack = true ) { m_hookThumbTrack = hookThumbTrack; }

	bool VisibleWhiteSpace( void ) const { return m_visibleWhiteSpace; }
	bool SetVisibleWhiteSpace( bool visibleWhiteSpace = true );

	void SelectAll( void ) { SetSel( 0, -1 ); }

	std::tstring GetSelText( void ) const;

	// multi-line edit
	typedef int TCharPos;											// position of text character in edit's text
	typedef int TLine;

	enum { CaretPos = -1 };

	TLine GetCaretLineIndex( void ) const { return LineFromChar( CaretPos ); }
	Range<TCharPos> GetLineRange( TLine linePos ) const;			// line startPos and endPos
	std::tstring GetLineText( TLine linePos ) const;

	Range<TCharPos> GetLineRangeAt( TCharPos charPos = CaretPos ) const { return GetLineRange( LineFromChar( charPos ) ); }
	std::tstring GetLineTextAt( TCharPos charPos = CaretPos ) const { return GetLineText( LineFromChar( charPos ) ); }		// by default: text of line with the caret

	// selection
	template< typename IntType >
	Range<IntType> GetSelRange( void ) const;

	template< typename IntType >
	void SetSelRange( const Range<IntType>& sel, bool noCaretScroll = false ) { SetSel( static_cast<TCharPos>( sel.m_start ), static_cast<TCharPos>( sel.m_end ), noCaretScroll ); }

	void SetSel( TCharPos startChar, TCharPos endChar, BOOL noCaretScroll = false );	// has to stay BOOL to avoid overload resolution with base method

	enum FontSize { Normal, Large };
	static CFont* GetFixedFont( FontSize fontSize = Normal );
	static void SetFixedFont( CWnd* pWnd );

	void EnsureCaretVisible( void ) { PostMessage( EM_SCROLLCARET ); }		// the only way to scroll the edit in order to make the caret visible (if outside client rect)

	CScopedInternalChange MakeUserChange( void ) { return CScopedInternalChange( &m_userChange ); }
protected:
	bool IsInternalChange( void ) const;

	virtual void OnValueChanged( void );		// by the user
private:
	void _WatchSelChange( void );
private:
	bool m_useFixedFont;
	bool m_keepSelOnFocus;						// true: keep old selection when focused; false: select all text when focused (default)
	bool m_usePasteTransact;					// true: supress EN_CHANGE during WM_PASTE/WM_UNDO input, and send one EN_CHANGE after text pasted
	bool m_hookThumbTrack;						// true: track thumb track scrolling events (edit controls don't send EN_VSCROLL on thumb track scrolling)
	bool m_visibleWhiteSpace;
	CAccelTable m_accel;

	CSyncScrolling* m_pSyncScrolling;
	Range<TCharPos> m_lastSelRange;
protected:
	CInternalChange m_userChange;				// allow derived classes to force user change mode (when spinning, etc)
public:
	static const TCHAR s_lineEnd[];

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	afx_msg UINT OnGetDlgCode( void );
	afx_msg void OnHScroll( UINT sbCode, UINT pos, CScrollBar* pScrollBar );
	afx_msg void OnVScroll( UINT sbCode, UINT pos, CScrollBar* pScrollBar );
	afx_msg LRESULT OnPasteOrUndo( WPARAM, LPARAM );
	afx_msg BOOL OnEnChange_Reflect( void );
	afx_msg BOOL OnEnKillFocus_Reflect( void );
	afx_msg BOOL OnEnHScroll_Reflect( void );
	afx_msg BOOL OnEnVScroll_Reflect( void );
	afx_msg void OnEditCopy( void );
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


// template code

template< typename IntType >
Range<IntType> CTextEdit::GetSelRange( void ) const
{
	TCharPos startPos, endPos;
	GetSel( startPos, endPos );
	ASSERT( startPos <= endPos && startPos >= 0 );
	return Range<IntType>( static_cast<IntType>( startPos ), static_cast<IntType>( endPos ) );
}


#endif // TextEdit_h
