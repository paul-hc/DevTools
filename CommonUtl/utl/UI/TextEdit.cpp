
#include "stdafx.h"
#include "TextEdit.h"
#include "Dialog_fwd.h"
#include "Icon.h"
#include "SyncScrolling.h"
#include "StringUtilities.h"
#include "WndUtils.h"
#include "PostCall.h"
#include "utl/TextClipboard.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


static ACCEL s_editKeys[] =
{
	{ FVIRTKEY | FCONTROL, _T('C'), ID_EDIT_COPY },
	{ FVIRTKEY | FCONTROL, VK_INSERT, ID_EDIT_COPY },
	{ FVIRTKEY | FCONTROL, _T('X'), ID_EDIT_CUT },
	{ FVIRTKEY | FSHIFT, VK_DELETE, ID_EDIT_CUT },
	{ FVIRTKEY | FCONTROL, _T('V'), ID_EDIT_PASTE },
	{ FVIRTKEY | FSHIFT, VK_INSERT, ID_EDIT_PASTE },
	{ FVIRTKEY | FCONTROL, _T('A'), ID_EDIT_SELECT_ALL }
};

const TCHAR CTextEdit::s_lineEnd[] = _T("\r\n");

CTextEdit::CTextEdit( bool useFixedFont /*= true*/ )
	: CFrameHostCtrl<CEdit>()
	, m_useFixedFont( useFixedFont )
	, m_keepSelOnFocus( false )
	, m_usePasteTransact( false )
	, m_hookThumbTrack( true )
	, m_visibleWhiteSpace( false )
	, m_accel( ARRAY_PAIR( s_editKeys ) )
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
			str::Replace( text, s_whitespaces[ i ].m_pEdit, s_whitespaces[ i ].m_pText );

	return text;
}

bool CTextEdit::SetText( const std::tstring& text )
{
	CScopedInternalChange internalChange( this );
	SetModify( FALSE );

	if ( m_visibleWhiteSpace )
	{
		std::tstring editText = text;
		for ( int i = 0; i != COUNT_OF( s_whitespaces ); ++i )
			str::Replace( editText, s_whitespaces[ i ].m_pText, s_whitespaces[ i ].m_pEdit );

		return ui::SetWindowText( m_hWnd, editText );
	}

	return ui::SetWindowText( m_hWnd, text );
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
	Range<int> sel;
	GetSel( sel.m_start, sel.m_end );
	return ui::GetWindowText( this ).substr( sel.m_start, sel.GetSpan<size_t>() );
}

Range<CTextEdit::TCharPos> CTextEdit::GetLineRange( TLine linePos ) const
{
	Range<int> lineRange( LineIndex( linePos ) );
	lineRange.m_end += LineLength( lineRange.m_start );
	return lineRange;
}

std::tstring CTextEdit::GetLineText( TLine linePos ) const
{
	// Note: careful with LineLength() - the 'nLine' parameter is the index of the first character on the line (TCharPos), not the line index
	size_t length = GetLineRange( linePos ).GetSpan<size_t>();
	std::vector< TCHAR > lineBuffer( length + 1 );

	size_t newLength = GetLine( linePos, ARRAY_PAIR_V( lineBuffer ) );
	ENSURE( length == newLength ); newLength;
	lineBuffer[ length ] = _T('\0');
	return &lineBuffer.front();
}

CFont* CTextEdit::GetFixedFont( FontSize fontSize /*= Normal*/ )
{
	static CFont s_fixedFont[ 2 ];
	if ( nullptr == s_fixedFont[ fontSize ].GetSafeHandle() )
		ui::MakeStandardControlFont( s_fixedFont[ fontSize ], ui::CFontInfo( _T("Consolas"), ui::Regular, Normal == fontSize ? 100 : 120 ) );		// "Courier New"
	return &s_fixedFont[ fontSize ];
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

void CTextEdit::OnValueChanged( void )
{
}

void CTextEdit::_WatchSelChange( void )
{
	if ( !IsInternalChange() )
	{
		Range<TCharPos> selRange = GetSelRange<TCharPos>();

		if ( selRange != m_lastSelRange )
		{
			m_lastSelRange = selRange;
			ui::SendCommandToParent( m_hWnd, CTextEdit::EN_USERSELCHANGE );		// notify to parent of the selection change
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
			Range<TCharPos> selRange = GetSelRange<TCharPos>();
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
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
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
	if ( IsInternalChange() ||		// doesn't propagate internal changes
		 HasInvalidText() )			// ignore invalid input text (partial input)
		return TRUE;				// skip parent routing

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
	Range<int> selRange = GetSelRange<int>();

	if ( !selRange.IsEmpty() )
		Copy();										// WM_COPY: copy the selected text
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
	pCmdUI->Enable( !GetSelRange<int>().IsEmpty() );
}

void CTextEdit::OnUpdate_Writeable( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( IsWritable() );
}

void CTextEdit::OnUpdate_WriteableHasSel( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( IsWritable() && !GetSelRange<int>().IsEmpty() );
}
