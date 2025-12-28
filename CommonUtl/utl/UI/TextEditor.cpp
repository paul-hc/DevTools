
#include "pch.h"
#include "TextEditor.h"
#include "MenuUtilities.h"
#include "StringUtilities.h"
#include "WndUtils.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace hlp
{
	Range<size_t> GetSelectWordRange( const std::tstring& text, const Range<size_t>& selRange )
	{
		ASSERT( selRange.IsNormalized() );
		ASSERT( selRange.m_end <= text.length() );
		Range<size_t> wordRange = selRange;

		const std::locale& loc = str::GetUserLocale();

		switch ( word::GetWordStatus( text, selRange.m_start, loc ) )
		{
			case word::WordStart:
			case word::Whitespace:
				break;
			case word::WordCore:
			case word::WordEnd:
				wordRange.m_start = word::FindPrevWordBreak( text, wordRange.m_start, loc );
				break;
		}
		if ( !std::isalnum( text[ wordRange.m_start ], loc ) )
			return selRange;						// no word hit -> keep the original selection

		wordRange.m_end = word::FindNextWordBreak( text, wordRange.m_start, loc );
		return wordRange;
	}

	template< typename IteratorT >
	void ChangeCase( IteratorT itStart, IteratorT itEnd, UINT cmdId )
	{
		switch ( cmdId )
		{
			case ID_EDIT_TO_LOWER_CASE:	std::transform( itStart, itEnd, itStart, func::ToLower() ); break;
			case ID_EDIT_TO_UPPER_CASE:	std::transform( itStart, itEnd, itStart, func::ToUpper() ); break;
			default: ASSERT( false );
		}
	}

	bool ChangeNumber( UINT& rNumber, UINT cmdId )
	{
		switch ( cmdId )
		{
			case ID_EDIT_NUM_INCREMENT:
				if ( UINT_MAX == rNumber )
					return false;
				++rNumber;
				break;
			case ID_EDIT_NUM_DECREMENT:
				if ( 0 == rNumber )
					return false;
				--rNumber;
				break;
			default: ASSERT( false );
		}
		return true;
	}
}


// CTextEditor implementation

CTextEditor::CTextEditor( bool useFixedFont /*= false*/ )
	: CTextEdit( useFixedFont )
	, m_editorAccel( IDR_TEXT_EDITOR_ACCEL )
{
}

CTextEditor::~CTextEditor()
{
}

CTextEditor::SelType CTextEditor::GetSelType( void ) const
{
	Range<TCharPos> selRange = GetSelRange();

	if ( selRange.IsEmpty() )
		return SelEmpty;
	if ( selRange.GetSpan<int>() == GetWindowTextLength() )
		return SelAllText;

	return SelSubText;
}

bool CTextEditor::HasSel( void ) const
{
	SelType selType = GetSelType();
	return SelSubText == selType || SelAllText == selType;
}

bool CTextEditor::HandleWordSelection( UINT cmdId )
{
	std::tstring text = ui::GetWindowText( this );
	if ( text.empty() )
		return false;

	Range<size_t> selRange = GetSelRangeAs<size_t>( true ), oldSelRange = selRange;

	switch ( cmdId )
	{
		case ID_EDIT_WORD_LEFT:
		case ID_EDIT_WORD_LEFT_EXTEND:
			selRange.m_end = word::FindPrevWordBreak( text, selRange.m_end );

			if ( cmdId != ID_EDIT_WORD_LEFT_EXTEND )
				selRange.m_start = selRange.m_end;		// collapse selection at START
			break;
		case ID_EDIT_WORD_RIGHT:
		case ID_EDIT_WORD_RIGHT_EXTEND:
			selRange.m_end = word::FindNextWordBreak( text, selRange.m_end );

			if ( cmdId != ID_EDIT_WORD_RIGHT_EXTEND )
				selRange.m_start = selRange.m_end;		// collapse selection at END
			break;
		case ID_EDIT_WORD_SELECT:
			selRange = hlp::GetSelectWordRange( text, selRange );
			break;
	}

	static size_t s_cnt = 0; TRACE_( "-[%d] oldSelRange=[%d, %d] caret=%d at %s  ->  selRange=[%d, %d]", s_cnt++,
		oldSelRange.m_start, oldSelRange.m_end, GetCaretCharPos(), oldSelRange.IsNormalized() ? "END" : "START", selRange.m_start, selRange.m_end );

	return SetSelRange( selRange );
}

BOOL CTextEditor::PreTranslateMessage( MSG* pMsg )
{
	return
		m_editorAccel.Translate( pMsg, m_hWnd ) ||
		__super::PreTranslateMessage( pMsg );
}


// message handlers

BEGIN_MESSAGE_MAP( CTextEditor, CTextEdit )
	ON_WM_CONTEXTMENU()
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND_RANGE( ID_EDIT_WORD_LEFT, ID_EDIT_WORD_SELECT, OnWordSelection )
	ON_COMMAND_RANGE( ID_EDIT_TO_LOWER_CASE, ID_EDIT_TO_UPPER_CASE, OnChangeCase )
	ON_COMMAND_RANGE( ID_EDIT_NUM_INCREMENT, ID_EDIT_NUM_DECREMENT, OnChangeNumber )
END_MESSAGE_MAP()

void CTextEditor::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	if ( this == pWnd && ui::IsKeyPressed( VK_CONTROL ) && IsWritable() )
		ui::TrackContextMenu( IDR_STD_CONTEXT_MENU, ui::TextEditorPopup, this, screenPos );
	else
		__super::OnContextMenu( pWnd, screenPos );
}

void CTextEditor::OnLButtonDblClk( UINT flags, CPoint point )
{
	__super::OnLButtonDblClk( flags, point );

	Range<size_t> selRange = GetSelRangeAs<size_t>();
	std::tstring selText = GetSelText();
	str::TrimRight( selText );						// exclude trailing whitespace on word selection

	selRange.m_end = selRange.m_start + selText.length();
	SetSelRange( selRange );
}

void CTextEditor::OnWordSelection( UINT cmdId )
{
	HandleWordSelection( cmdId );
}

void CTextEditor::OnChangeCase( UINT cmdId )
{
	Range<size_t> selRange = GetSelRangeAs<size_t>();
	bool caretAtStart = IsCaretAtSelStart();
	std::tstring text = ui::GetWindowText( this );

	if ( text.empty() || selRange.m_start == text.length() )
	{
		ui::BeepSignal();						// no text to consume
		return;
	}

	TLineIndex topLineIndex = GetTopLineIndex();
	SelType selType = GetSelType();

	switch ( selType )
	{
		case SelEmpty:
			// change case of current character and advance to next
			++selRange.m_end;
			hlp::ChangeCase( text.begin() + selRange.m_start, text.begin() + selRange.m_end, cmdId );
			selRange.m_start = selRange.m_end;
			SetTextImpl( text );
			SetModify();		// mark as dirty to enable dialog data exchange
			break;
		case SelSubText:
		case SelAllText:
		{
			std::tstring subText = text.substr( selRange.m_start, selRange.GetSpan<size_t>() );
			hlp::ChangeCase( subText.begin(), subText.end(), cmdId );
			ReplaceSel( subText.c_str(), TRUE );
			ENSURE( GetModify() );
			break;
		}
	}

	if ( !selRange.IsEmpty() && caretAtStart )
		selRange.SwapBounds();		// restore caret in the original direction

	SetSelRange( selRange );
	SetTopLineIndex( topLineIndex );

	if ( selType != SelEmpty )
		InvalidateFrame( FocusFrame );		// prevent focus frame disappearing
}

void CTextEditor::OnChangeNumber( UINT cmdId )
{
	Range<size_t> selRange = GetSelRangeAs<size_t>();
	std::tstring text = ui::GetWindowText( this );

	if ( num::EnwrapNumericSequence( selRange, text ) )							// find the entire digit sequence in the vicinity of selection
	{
		std::tstring numberText = text.substr( selRange.m_start, selRange.GetSpan<size_t>() );
		UINT number;
		if ( num::ParseNumber( number, numberText ) )
		{
			SetSelRange( selRange );
			if ( hlp::ChangeNumber( number, cmdId ) )
			{
				size_t digitsWidth = numberText.length();
				if ( _T('0') == numberText[ 0 ] )
					numberText = str::Format( _T("%0*u"), digitsWidth, number );		// retain the existing leading 0 padding
				else
					numberText = str::Format( _T("%u"), number );

				ReplaceSel( numberText.c_str(), TRUE );
				selRange.m_end = selRange.m_start + numberText.length();
				SetSelRange( selRange );
				return;
			}
		}
	}

	ui::BeepSignal();						// no number text to change
}
