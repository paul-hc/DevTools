
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
	Range<size_t> GetSelectWordRange( const std::tstring& text, const Range<size_t>& sel )
	{
		ASSERT( sel.IsNormalized() );
		ASSERT( sel.m_end <= text.length() );
		Range<size_t> wordRange = sel;

		const std::locale& loc = str::GetUserLocale();

		switch ( word::GetWordStatus( text, sel.m_start, loc ) )
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
			return sel;						// no word hit -> keep the original selection

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
	Range<int> sel = GetSelRange<int>();
	if ( sel.IsEmpty() )
		return SelEmpty;

	return sel.GetSpan<size_t>() == ui::GetWindowText( this ).length() ? SelAllText : SelSubText;
}

bool CTextEditor::HasSel( void ) const
{
	SelType selType = GetSelType();
	return SelSubText == selType || SelAllText == selType;
}

BOOL CTextEditor::PreTranslateMessage( MSG* pMsg )
{
	return m_editorAccel.Translate( pMsg, m_hWnd ) || CTextEdit::PreTranslateMessage( pMsg );
}


// message handlers

BEGIN_MESSAGE_MAP( CTextEditor, CTextEdit )
	ON_WM_CONTEXTMENU()
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND_RANGE( ID_EDIT_WORD_LEFT, ID_EDIT_WORD_SELECT, OnWordSelection )
	ON_COMMAND_RANGE( ID_EDIT_TO_LOWER_CASE, ID_EDIT_TO_UPPER_CASE, OnChangeCase )
	ON_COMMAND_RANGE( ID_EDIT_NUM_INCREMENT, ID_EDIT_NUM_DECREMENT, OnChangeNumber )
END_MESSAGE_MAP()

void CTextEditor::OnContextMenu( CWnd* pWnd, CPoint point )
{
	if ( this == pWnd && ui::IsKeyPressed( VK_CONTROL ) && IsWritable() )
	{
		CMenu contextMenu;
		ui::LoadPopupMenu( contextMenu, IDR_STD_CONTEXT_MENU, ui::TextEditorPopup );
		ui::TrackPopupMenu( contextMenu, this, point, TPM_RIGHTBUTTON );
	}
	else
		CTextEdit::OnContextMenu( pWnd, point );
}

void CTextEditor::OnLButtonDblClk( UINT flags, CPoint point )
{
	CTextEdit::OnLButtonDblClk( flags, point );

	Range<size_t> sel = GetSelRange<int>();
	std::tstring selText = GetSelText();
	str::TrimRight( selText );						// exclude trailing whitespace on word selection

	sel.m_end = sel.m_start + selText.length();
	SetSelRange( sel );
}

void CTextEditor::OnWordSelection( UINT cmdId )
{
	Range<size_t> sel = GetSelRange<int>();
	std::tstring text = ui::GetWindowText( this );
	if ( text.empty() )
		return;

	switch ( cmdId )
	{
		case ID_EDIT_WORD_LEFT:
		case ID_EDIT_WORD_LEFT_EXTEND:
			sel.m_start = word::FindPrevWordBreak( text, sel.m_start );
			if ( cmdId != ID_EDIT_WORD_LEFT_EXTEND )
				sel.m_end = sel.m_start;
			break;
		case ID_EDIT_WORD_RIGHT:
		case ID_EDIT_WORD_RIGHT_EXTEND:
			sel.m_end = word::FindNextWordBreak( text, sel.m_end );
			if ( cmdId != ID_EDIT_WORD_RIGHT_EXTEND )
				sel.m_start = sel.m_end;
			break;
		case ID_EDIT_WORD_SELECT:
			sel = hlp::GetSelectWordRange( text, sel );
			break;
	}
	SetSelRange( sel );
}

void CTextEditor::OnChangeCase( UINT cmdId )
{
	Range<size_t> sel = GetSelRange<int>();
	std::tstring text = ui::GetWindowText( this );
	if ( text.empty() || sel.m_start == text.length() )
	{
		ui::BeepSignal();						// no text to consume
		return;
	}

	switch ( GetSelType() )
	{
		case SelEmpty:
			// change case of current character and advance to next
			++sel.m_end;
			hlp::ChangeCase( text.begin() + sel.m_start, text.begin() + sel.m_end, cmdId );
			sel.m_start = sel.m_end;
			ui::SetWindowText( m_hWnd, text );
			SetModify();		// mark as dirty to enable dialog data exchange
			break;
		case SelSubText:
		case SelAllText:
		{
			std::tstring subText = text.substr( sel.m_start, sel.GetSpan<size_t>() );
			hlp::ChangeCase( subText.begin(), subText.end(), cmdId );
			ReplaceSel( subText.c_str(), TRUE );
			break;
		}
	}
	SetSelRange( sel );
}

void CTextEditor::OnChangeNumber( UINT cmdId )
{
	Range<size_t> sel = GetSelRange<int>();
	std::tstring text = ui::GetWindowText( this );

	if ( num::EnwrapNumericSequence( sel, text ) )							// find the entire digit sequence in the vicinity of selection
	{
		std::tstring numberText = text.substr( sel.m_start, sel.GetSpan<size_t>() );
		UINT number;
		if ( num::ParseNumber( number, numberText ) )
		{
			SetSelRange( sel );
			if ( hlp::ChangeNumber( number, cmdId ) )
			{
				size_t digitsWidth = numberText.length();
				if ( _T('0') == numberText[ 0 ] )
					numberText = str::Format( _T("%0*u"), digitsWidth, number );		// retain the existing leading 0 padding
				else
					numberText = str::Format( _T("%u"), number );

				ReplaceSel( numberText.c_str(), TRUE );
				sel.m_end = sel.m_start + numberText.length();
				SetSelRange( sel );
				return;
			}
		}
	}

	ui::BeepSignal();						// no number text to change
}
