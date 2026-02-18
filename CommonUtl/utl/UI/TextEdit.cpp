
#include "pch.h"
#include "TextEdit.h"
#include "Dialog_fwd.h"
#include "Icon.h"
#include "SyncScrolling.h"
#include "StringUtilities.h"
#include "WndUtils.h"
#include "PostCall.h"
#include "resource.h"
#include "utl/Algorithms_fwd.h"
#include "utl/CommonLength.h"
#include "utl/TextClipboard.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	// CTextValidator implementation

	CTextValidator::CTextValidator( const CTextEdit* pEdit )
		: m_pEdit( pEdit )
		, m_inputText( safe_ptr( m_pEdit )->GetText() )
		, m_itemCount( 1 )
	{
		if ( m_pEdit->IsMultiLine() )
		{
			str::SplitLines( m_lines, m_inputText.c_str(), CTextEdit::s_lineEnd );
			m_itemCount = m_lines.size();
		}
	}

	bool CTextValidator::StoreAmendedText( void )
	{
		std::tstring amendedText = str::Join( m_lines, CTextEdit::s_lineEnd );

		if ( amendedText == m_inputText )
			return false;

		m_amendedText.swap( amendedText );
		return true;
	}
}


// CTextEdit implementation

const TCHAR CTextEdit::s_lineEnd[] = _T("\r\n");

CTextEdit::CTextEdit( bool useFixedFont /*= true*/ )
	: CFrameHostCtrl<CEdit>()
	, m_useFixedFont( useFixedFont )
	, m_keepSelOnFocus( false )
	, m_usePasteTransact( false )
	, m_hookThumbTrack( true )
	, m_visibleWhiteSpace( false )
	, m_accel( IDR_EDIT_ACCEL )
	, m_pTextInputCallback( nullptr )
	, m_pSyncScrolling( nullptr )
	, m_lastSelRange( 0, 0 )
{
}

CTextEdit::~CTextEdit()
{
}

void CTextEdit::AddToSyncScrolling( CSyncScrolling* pSyncScrolling )
{
	ASSERT_PTR( pSyncScrolling );
	ASSERT_NULL( m_pSyncScrolling );

	m_pSyncScrolling = pSyncScrolling;
	m_pSyncScrolling->AddCtrl( this );
}

bool CTextEdit::HasInvalidText( void ) const
{
	return false;
}

bool CTextEdit::NormalizeText( void )
{
	return false;
}

enum Whitespace { Tab, LineEnd };
static const struct { LPCWSTR m_pText, m_pEdit; } s_whitespaces[] =
{
	{ L"\t", L"\x2192\t" },
	{ L"\r\n", L"¤\r\n" }
};

std::tstring CTextEdit::GetText( void ) const
{
	std::tstring text;
	ui::GetWindowText( text, m_hWnd );		// avoid an extra copy

	if ( m_visibleWhiteSpace )
		for ( int i = 0; i != COUNT_OF( s_whitespaces ); ++i )
			str::Replace( text, s_whitespaces[i].m_pEdit, s_whitespaces[i].m_pText );

	return text;
}

bool CTextEdit::SetText( const std::tstring& text )
{
	CScopedInternalChange internalChange( this );

	SetModify( FALSE );
	return SetTextImpl( text );
}

bool CTextEdit::SetTextImpl( const std::tstring& text )
{
	m_lastValidText = text;

	if ( m_visibleWhiteSpace )
	{
		std::tstring editText = text;
		for ( int i = 0; i != COUNT_OF( s_whitespaces ); ++i )
			str::Replace( editText, s_whitespaces[i].m_pText, s_whitespaces[i].m_pEdit );

		return ui::SetWindowText( m_hWnd, editText );
	}

	return ui::SetWindowText( m_hWnd, text );
}

bool CTextEdit::ReplaceText( const std::tstring& text, bool canUndo /*= true*/ )
{
	std::tstring oldText = GetText();

	if ( text == oldText )
		return false;

	TLineIndex topLineIndex = GetTopLineIndex();

	// a more functional method based on text selection/replacement, that doesn't break the Undo:
	utl::CCommonLengths<std::tstring> common( oldText, text );
	Range<TCharPos> oldMismatchSelRange = common.GetLeftMismatchRange<TCharPos>();
	std::tstring newSelText = common.MakeRightMismatch();

	SetSelRange( oldMismatchSelRange );
	ReplaceSel( newSelText.c_str(), canUndo );
	ENSURE( GetModify() );

	SetSelRange( common.GetRightMismatchRange<TCharPos>() );		// select the new/changed core text

	SetTopLineIndex( topLineIndex );
	return false;
}

bool CTextEdit::HasPlaceholderTag( void ) const
{	// current text is the placeholder tag?
	return !m_placeholderTag.empty() && m_placeholderTag == GetText();
}

bool CTextEdit::RevertContents( void )
{
	return ReplaceText( m_lastValidText );
}

void CTextEdit::DDX_Text( CDataExchange* pDX, std::tstring& rValue, int ctrlId /*= 0*/ )
{
	if ( nullptr == m_hWnd && ctrlId != 0 )
		::DDX_Control( pDX, ctrlId, *this );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		SetText( rValue );
	else
		rValue = GetText();
}

void CTextEdit::DDX_UiEscapeSeqs( CDataExchange* pDX, std::tstring& rValue, int ctrlId /*= 0*/ )
{
	if ( nullptr == m_hWnd && ctrlId != 0 )
		DDX_Control( pDX, ctrlId, *this );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		SetText( ui::FormatEscapeSeq( rValue ) );
	else
		rValue = ui::ParseEscapeSeqs( GetText() );
}

bool CTextEdit::SetVisibleWhiteSpace( bool visibleWhiteSpace /*= true*/ )
{
	if ( visibleWhiteSpace == m_visibleWhiteSpace )
		return false;

	std::tstring text;
	if ( m_hWnd != nullptr )
		text = GetText();

	m_visibleWhiteSpace = visibleWhiteSpace;

	if ( m_hWnd != nullptr )
		SetText( text );

	return true;
}

void CTextEdit::SetSel( TCharPos startChar, TCharPos endChar, BOOL noCaretScroll /*= false*/ )
{
	m_lastSelRange.SetRange( startChar, endChar );
	__super::SetSel( startChar, endChar, noCaretScroll );
}

std::tstring CTextEdit::GetSelText( void ) const
{
	Range<TCharPos> selRange = GetSelRange();

	return m_lastValidText.substr( selRange.m_start, selRange.GetSpan<size_t>() );		// was ui::GetWindowText( this ).
}

size_t CTextEdit::GetEffectiveLineCount( const std::tstring& text )
{
	size_t lineCount = std::count( text.begin(), text.end(), '\n' ) + 1;

	if ( !text.empty() && str::LastChar( text ) == '\n' )
		--lineCount;		// ignore last empty line

	return lineCount;
}

CTextEdit::TCharPos CTextEdit::GetCaretCharPos( TLineIndex* pCaretLineIndex /*= nullptr*/ ) const
{
	CPoint caretPoint = CWnd::GetCaretPos();		// in client coordinates
	TCharPos caretPos = GetCharFromPoint( caretPoint, pCaretLineIndex );
	return caretPos;
}

bool CTextEdit::SetCaretCharPos( TCharPos caretPos )
{
	CPoint caretPoint = CWnd::GetCaretPos();		// in client coordinates

	if ( !utl::ModifyValue( caretPoint, GetCharPointTL( caretPos ) ) )
		return false;		// caret not changed

	CWnd::SetCaretPos( caretPoint );
	return true;
}

CTextEdit::TCharPos CTextEdit::GetAnchorCharPos( TLineIndex* pAnchorLineIndex /*= nullptr*/ ) const
{
	TCharPos caretPos = GetCaretCharPos( pAnchorLineIndex );
	Range<TCharPos> selRange = GetSelRange();

	ASSERT( caretPos == selRange.m_start || caretPos == selRange.m_end );	// caret should be either START or END of selction

	if ( selRange.IsEmpty() )
		return caretPos;		// anchor is the caret when selection is empty

	TCharPos anchorPos = ( caretPos == selRange.m_end ) ? selRange.m_start : selRange.m_end;

	if ( pAnchorLineIndex != nullptr )
		*pAnchorLineIndex = LineFromChar( anchorPos );

	return anchorPos;
}

CTextEdit::TCharPos CTextEdit::GetCharFromPoint( const CPoint& clientPoint, TLineIndex* pLineIndex /*= nullptr*/ ) const
{
	int result = CharFromPos( clientPoint );

	if ( pLineIndex != nullptr )
		*pLineIndex = static_cast<TLineIndex>( HIWORD( result ) );

	return static_cast<TCharPos>( LOWORD( result ) );
}

Range<CTextEdit::TCharPos> CTextEdit::GetLineRange( TLineIndex lineIndex ) const
{
	Range<TCharPos> lineRange( LineToCharPos( lineIndex ) );

	lineRange.m_end += LineLength( lineRange.m_start );
	return lineRange;
}

std::tstring CTextEdit::GetLineText( TLineIndex lineIndex ) const
{
	// Note: careful with LineLength() - the 'nLine' parameter is the index of the first character on the line (TCharPos), not the line index
	size_t length = GetLineRange( lineIndex ).GetSpan<size_t>();
	std::vector<TCHAR> lineBuffer( length + 1 );

	size_t newLength = GetLine( lineIndex, ARRAY_SPAN_V( lineBuffer ) );
	ENSURE( length == newLength ); newLength;
	lineBuffer[length] = _T('\0');
	return &lineBuffer.front();
}

bool CTextEdit::IsCaretAtSelStart( void ) const
{
	Range<TCharPos> selRange = GetSelRange();

	return !selRange.IsEmpty() && GetCaretCharPos() == selRange.m_start;
}

Range<CTextEdit::TCharPos> CTextEdit::GetSelRange( bool swapIfCaretAtStart /*= false*/ ) const
{
	Range<TCharPos> selRange;

	GetSel( selRange.m_start, selRange.m_end );
	ASSERT( selRange.m_start <= selRange.m_end && selRange.m_start >= 0 );

	if ( swapIfCaretAtStart && !selRange.IsEmpty() )
	{
		TCharPos caretPos = GetCaretCharPos();

		if ( caretPos == selRange.m_start )
			selRange.SwapBounds();			// invert the range to signal that caret is at start: [anchor, caret], e.g. for caret at 5, [5, 8] -> [8, 5]
		else
			ASSERT( caretPos == selRange.m_end );
	}

	return selRange;
}

bool CTextEdit::ClearSelection( bool collapseToEnd /*= true*/ )
{
	Range<TCharPos> selRange = GetSelRange();

	if ( selRange.IsEmpty() )
		return false;

	if ( collapseToEnd )
		selRange.m_start = selRange.m_end;
	else
		selRange.m_end = selRange.m_start;

	return SetSelRange( selRange );		// collapse selection to END, which we assume as the caret
}

std::pair< Range<CTextEdit::TCharPos>, bool > CTextEdit::SelectLine( TLineIndex lineIndex )
{
	Range<CTextEdit::TCharPos> selRange = GetLineRange( lineIndex );

	return std::make_pair( selRange, selRange.m_start != -1 && SetSelRange( selRange ) );
}

std::pair< Range<CTextEdit::TCharPos>, bool > CTextEdit::SelectLineRange( const Range<TLineIndex>& selLineRange )
{
	ASSERT( selLineRange.IsNormalized() );

	Range<CTextEdit::TCharPos> selRange = GetLineRange( selLineRange.m_start );

	if ( !selLineRange.IsEmpty() )
		selRange.m_end = GetLineRange( selLineRange.m_end ).m_end;		// extend for multiple lines

	return std::make_pair( selRange, selRange.m_start != -1 && selRange.m_end != -1 && SetSelRange( selRange ) );
}

Range<CTextEdit::TLineIndex> CTextEdit::LineRangeFromCharRange( const Range<TCharPos>& selRange, bool intuitiveSel /*= true*/ ) const
{
	ASSERT( selRange.IsNormalized() );

	Range<TLineIndex> selLineRange( LineFromChar( selRange.m_start ), LineFromChar( selRange.m_end ) );

	if ( intuitiveSel )
		AdjustIntuitiveLineSelection( &selLineRange, selRange );

	ENSURE( selLineRange.IsNormalized() );
	//TRACE( "+ CTextEdit::LineRangeFromCharRange(): selRange=(%d, %d)  selLineRange=(%d, %d)\n", selRange.m_start, selRange.m_end, selLineRange.m_start, selLineRange.m_end );
	return selLineRange;
}

bool CTextEdit::AdjustIntuitiveLineSelection( IN OUT Range<TLineIndex>* pSelLineRange, const Range<TCharPos>& selRange ) const
{	// returns true if line selection changed
	ASSERT( selRange.IsNormalized() );
	ASSERT( pSelLineRange != nullptr && pSelLineRange->IsNormalized() );

	if ( pSelLineRange->IsEmpty() )
		return false;		// nothing to adjust if selection is not multi-line

	Range<TLineIndex> origSelLineRange = *pSelLineRange;

	// Intuitive text selection - make selected lines based on what the user sees as selected:
	//	- exclude caret from selection if either SelStart or SelEnd bounds are not at least partially text-selected in the corresponding bound lines.
	//
	Range<TCharPos> firstLineChRange = GetLineRange( pSelLineRange->m_start );
	if ( selRange.m_start == firstLineChRange.m_end )			// SelStart on first line with no trailing overlap (right at line End)?
		++pSelLineRange->m_start;

	Range<TCharPos> lastLineChRange = GetLineRange( pSelLineRange->m_end );
	if ( selRange.m_end == lastLineChRange.m_start )			// SelEnd on last line with no leading overlap (right on line Begin)?
		--pSelLineRange->m_end;

	if ( !pSelLineRange->IsNormalized() )						// SelStart on a line End and SelEnd at line Begin (empty text selection)?
		pSelLineRange->SetEmptyRange( GetCaretLineIndex() );	// pick the caret line as selected

	return *pSelLineRange != origSelLineRange;		// true if line selection changed
}

bool CTextEdit::SetTopLineIndex( TLineIndex topLineIndex )
{
	TLineIndex currTopLineIndex = GetTopLineIndex();

	if ( topLineIndex < 0 || topLineIndex == currTopLineIndex )
		return false;

	LineScroll( topLineIndex - currTopLineIndex, 0 );		// scroll vertically RELATIVE to current top index!
	//TRACE( "+ CTextEdit::SetTopLineIndex(): topLineIndex=(%d)  newTopLineIndex=%d  currTopLineIndex=%d\n", topLineIndex, GetTopLineIndex(), currTopLineIndex );
	return true;
}

CFont* CTextEdit::GetFixedFont( FontSize fontSize /*= Normal*/ )
{
	static CFont s_fixedFont[2];
	if ( nullptr == s_fixedFont[fontSize].GetSafeHandle() )
		ui::MakeStandardControlFont( s_fixedFont[fontSize], ui::CFontInfo( _T("Consolas"), ui::Regular, Normal == fontSize ? 100 : 120 ) );		// "Courier New"
	return &s_fixedFont[fontSize];
}

void CTextEdit::SetFixedFont( CWnd* pWnd )
{
	pWnd->SetFont( GetFixedFont() );
	if ( CEdit* pEdit = dynamic_cast<CEdit*>( pWnd ) )
		pEdit->SetTabStops( 16 );				// default tab stop is 32 dialog base units (8 chars), reduce it to half (4 chars)
	pWnd->Invalidate();
}

bool CTextEdit::IsInternalChange( void ) const
{
	if ( m_userChange.IsInternalChange() )
		return false;							// forced user change - allows derived classes to use SetText()

	return CInternalChange::IsInternalChange();
}

bool CTextEdit::ValidateText( ui::CTextValidator& rValidator ) implement
{
	static const TCHAR s_title[] = _T("Text Validation");

	if ( HasInvalidText() )
	{
		RevertContents();
		ui::ShowInfoTip( this, s_title, _T("Please input valid text!"), MB_ICONERROR );
		return false;		// validation error
	}

	if ( IsMultiLine() && rValidator.HasLineMismatch() )
	{	// invalid input: prohibit changing the number of items (lines)
		RevertContents();

		std::tstring message = str::Format( _T("Attempted to input %d lines!\nYou must input exactly %d lines of text."), rValidator.m_lines.size(), rValidator.m_itemCount );
		ui::ShowInfoTip( this, s_title, message, MB_ICONERROR );
		return false;		// validation error
	}

	if ( m_pTextInputCallback != nullptr )
	{
		ui::ITextInput::Result result = m_pTextInputCallback->OnEditInput( rValidator );

		switch ( result )
		{
			case ui::ITextInput::Success:
			case ui::ITextInput::Warning:
				if ( rValidator.StoreAmendedText() )
				{
					ReplaceText( rValidator.m_amendedText );		// revert to valid amended text
					result = ui::ITextInput::Warning;
				}
				if ( ui::ITextInput::Warning == result )
					ui::ShowInfoTip( this, s_title, _T("Filtered input text."), MB_ICONWARNING );
				break;
			case ui::ITextInput::Error:
				RevertContents();
				ui::ShowInfoTip( this, s_title, _T("Input error!"), MB_ICONERROR );
				return false;
		}
	}

	return true;
}

void CTextEdit::OnValueChanged( void )
{
}

COLORREF CTextEdit::GetCustomTextColor( void ) const
{
	return CLR_NONE;
}

void CTextEdit::_WatchSelChange( void )
{
	if ( !IsInternalChange() )
	{
		Range<TCharPos> selRange = GetSelRange();

		if ( selRange != m_lastSelRange )
		{
			m_lastSelRange = selRange;
			ui::SendCommandToParent( m_hWnd, CTextEdit::EN_USER_SELCHANGE );		// notify to parent of the selection change

			//TRACE( "- m_lastSelRange=[%d, %d]\n", m_lastSelRange.m_start, m_lastSelRange.m_end );
		}
	}
}

void CTextEdit::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();

	if ( m_useFixedFont )
		SetFixedFont( this );
}

BOOL CTextEdit::PreTranslateMessage( MSG* pMsg )
{
	if ( WM_KEYDOWN == pMsg->message && m_hWnd == pMsg->hwnd )
		if ( !ui::IsKeyPressed( VK_SHIFT ) )
		{
			Range<TCharPos> selRange = GetSelRange();
			if ( !selRange.IsEmpty() )

			// collapse selection in the direction of the key
			switch ( pMsg->wParam )
			{
				case VK_LEFT:
				case VK_RIGHT:
					if ( !ui::IsKeyPressed( VK_CONTROL ) )
					{
						if ( VK_LEFT == pMsg->wParam )
							SetSel( selRange.m_start, selRange.m_start );
						else
							SetSel( selRange.m_end, selRange.m_end );
						return TRUE;										// eat the key
					}
					break;
				case VK_UP:
					SetSel( selRange.m_start, selRange.m_start );
					break;
				case VK_DOWN:
					SetSel( selRange.m_end, selRange.m_end );
					break;
			}
		}

	return
		m_accel.Translate( pMsg, m_hWnd ) ||
		__super::PreTranslateMessage( pMsg );
}


// message handlers

BEGIN_MESSAGE_MAP( CTextEdit, TBaseClass )
	ON_WM_GETDLGCODE()
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_MESSAGE( EM_REPLACESEL, OnEmReplaceSel )
	ON_MESSAGE( WM_PASTE, OnPasteOrUndo )
	ON_MESSAGE( WM_UNDO, OnPasteOrUndo )
	ON_CONTROL_REFLECT_EX( EN_CHANGE, OnEnChange_Reflect )
	ON_CONTROL_REFLECT_EX( EN_KILLFOCUS, OnEnKillFocus_Reflect )
	ON_CONTROL_REFLECT_EX( EN_HSCROLL, OnEnHScroll_Reflect )
	ON_CONTROL_REFLECT_EX( EN_VSCROLL, OnEnVScroll_Reflect )
	ON_COMMAND( ID_EDIT_COPY, OnEditCopy )
	ON_COMMAND( ID_EDIT_CUT, OnEditCut )
	ON_UPDATE_COMMAND_UI( ID_EDIT_CUT, OnUpdate_WriteableHasSel )
	ON_COMMAND( ID_EDIT_CLEAR, OnEditClear )
	ON_UPDATE_COMMAND_UI( ID_EDIT_CLEAR, OnUpdate_WriteableHasSel )
	ON_COMMAND( ID_EDIT_CLEAR_ALL, OnEditClearAll )
	ON_UPDATE_COMMAND_UI( ID_EDIT_CLEAR_ALL, OnUpdate_Writeable )
	ON_COMMAND( ID_EDIT_PASTE, OnEditPaste )
	ON_UPDATE_COMMAND_UI( ID_EDIT_PASTE, OnUpdateEditPaste )
	ON_COMMAND( ID_EDIT_SELECT_ALL, OnSelectAll )
END_MESSAGE_MAP()

UINT CTextEdit::OnGetDlgCode( void )
{
	UINT code = __super::OnGetDlgCode();

	if ( m_keepSelOnFocus )
		ClearFlag( code, DLGC_HASSETSEL );

	if ( GetShowFocus() )
		InvalidateFrame( FocusFrame );			// one of the few reliable ways to keep the focus rect properly drawn, since the edit draws directly to DC oftenly (without WM_PAINT)

	// WM_GETDLGCODE gets called frequently while editing and changing text selection - this is the right time to monitor for selection changes
	if ( !IsInternalChange() )
		ui::PostCall( this, &CTextEdit::_WatchSelChange );
	return code;
}

HBRUSH CTextEdit::CtlColor( CDC* pDC, UINT ctlColor )
{
	ctlColor;

	bool readOnly = IsReadOnly();
	COLORREF textColor = CLR_NONE;

	if ( HasPlaceholderTag() )
		textColor = ::GetSysColor( COLOR_SCROLLBAR );	// ~ group-box frame color
	else
		textColor = GetCustomTextColor();				// allow custom color highlight

	if ( CLR_NONE == textColor && !readOnly )
		return nullptr;		// no color customization

	COLORREF bkColor = ::GetSysColor( readOnly ? COLOR_BTNFACE : COLOR_WINDOW );		// gray background if read-only in both dialogs and property pages
	HBRUSH hBkBrush = ::GetSysColorBrush( readOnly ? COLOR_BTNFACE : COLOR_WINDOW );

	pDC->SetBkColor( bkColor );

	if ( textColor != CLR_NONE )
		pDC->SetTextColor( textColor );

	return hBkBrush;
}

void CTextEdit::OnHScroll( UINT sbCode, UINT pos, CScrollBar* pScrollBar )
{
	__super::OnHScroll( sbCode, pos, pScrollBar );

	if ( m_hookThumbTrack && SB_THUMBTRACK == sbCode )
		ui::SendCommandToParent( m_hWnd, EN_HSCROLL );
}

void CTextEdit::OnVScroll( UINT sbCode, UINT pos, CScrollBar* pScrollBar )
{
	__super::OnVScroll( sbCode, pos, pScrollBar );

	if ( m_hookThumbTrack && SB_THUMBTRACK == sbCode )
		ui::SendCommandToParent( m_hWnd, EN_VSCROLL );
}

LRESULT CTextEdit::OnEmReplaceSel( WPARAM wParam, LPARAM lParam )
{
	BOOL canUndo = (BOOL)wParam;
	const TCHAR* pNewText = (const TCHAR*)lParam;
	canUndo, pNewText;

	LRESULT result;
	{	// Supress internal EN_CHANGE notifications sent by "Edit" during CEdit::ReplaceSel().
		// This is in order to prevent intermediate input validation errors during the transacted calls to EditML_DeleteText() and EditML_InsertText() calls.
		CScopedInternalChange internalChange( this );
		result = Default();
	}

	ui::SendCommandToParent( m_hWnd, EN_CHANGE );		// notify once at the end of replacing text transaction
	return result;
}

LRESULT CTextEdit::OnPasteOrUndo( WPARAM, LPARAM )
{
	LRESULT result;
	{	// supress internal EN_CHANGE notifications sent by "Edit" during Paste/Undo (for DeleteText, InsertText)
		CScopedInternalChange internalChange( this );
		result = Default();
	}
	ui::SendCommandToParent( m_hWnd, EN_CHANGE );		// notify once at the end
	return result;
}

BOOL CTextEdit::OnEnChange_Reflect( void )
{
	if ( IsInternalChange() )		// don't propagate internal changes
		return TRUE;				// skip parent routing

	CScopedInternalChange pageChange( this );
	ui::CTextValidator validator( this );

	if ( !ValidateText( validator ) )
		return TRUE;				// skip parent routing

	m_lastValidText = GetText();
	OnValueChanged();
	return FALSE;					// continue routing
}

BOOL CTextEdit::OnEnKillFocus_Reflect( void )
{
	NormalizeText();
	return FALSE;					// continue routing
}

BOOL CTextEdit::OnEnHScroll_Reflect( void )
{
	if ( m_pSyncScrolling != nullptr && m_pSyncScrolling->SyncHorizontal() )
		m_pSyncScrolling->Synchronize( this );

	return FALSE;					// continue routing
}

BOOL CTextEdit::OnEnVScroll_Reflect( void )
{
	if ( m_pSyncScrolling != nullptr && m_pSyncScrolling->SyncVertical() )
		m_pSyncScrolling->Synchronize( this );

	return FALSE;					// continue routing
}

void CTextEdit::OnEditCopy( void )
{
	Range<TCharPos> selRange = GetSelRange();

	if ( !selRange.IsEmpty() )
		Copy();											// WM_COPY: copy the selected text
	else
		CTextClipboard::CopyText( GetText(), m_hWnd );	// copy the entire text
}

void CTextEdit::OnEditCut( void )
{
	Cut();
}

void CTextEdit::OnEditPaste( void )
{
	Paste();
}

void CTextEdit::OnUpdateEditPaste( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( IsWritable() && CTextClipboard::CanPasteText() );
}

void CTextEdit::OnEditClear( void )
{
	Clear();
}

void CTextEdit::OnEditClearAll( void )
{
	SelectAll();
	Clear();
}

void CTextEdit::OnSelectAll( void )
{
	SelectAll();
}

void CTextEdit::OnUpdate_HasSel( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !GetSelRange().IsEmpty() );
}

void CTextEdit::OnUpdate_Writeable( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( IsWritable() );
}

void CTextEdit::OnUpdate_WriteableHasSel( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( IsWritable() && !GetSelRange().IsEmpty() );
}
