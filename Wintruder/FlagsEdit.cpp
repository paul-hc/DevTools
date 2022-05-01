
#include "stdafx.h"
#include "FlagsEdit.h"
#include "resource.h"
#include "utl/TextClipboard.h"
#include "utl/UI/CtrlUiState.h"
#include "utl/UI/TextEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CFlagsEdit::CFlagsEdit( void )
	: CEdit()
	, CBaseFlagsCtrl( this )
{
}

CFlagsEdit::~CFlagsEdit()
{
}

void CFlagsEdit::InitControl( void )
{
	if ( m_hWnd != NULL )
		SetReadOnly( !HasValidMask() );
}

void CFlagsEdit::OutputFlags( void )
{
	if ( NULL == m_hWnd )
		return;

	CScopedInternalChange change( this );
	CEditUiState scopedSel( *this );
	ui::SetWindowText( m_hWnd, str::Format( _T("0x%08X"), GetFlags() ) );
}

DWORD CFlagsEdit::InputFlags( bool* pValid /*= NULL*/ ) const
{
	ASSERT( HasValidMask() );
	std::tstring text = ui::GetWindowText( this );
	str::ToLower( text );

	DWORD flags = GetFlags();
	bool valid = 1 == _stscanf( text.c_str(), _T(" 0x %x"), &flags );
	if ( pValid != NULL )
		*pValid = valid;
	return flags;
}

bool CFlagsEdit::HandleTextChange( void )
{
	if ( IsInternalChange() || !HasValidMask() )
		return false;

	bool valid;
	DWORD newFlags = InputFlags( &valid ), flagsMask = GetFlagsMask();
	if ( !valid )
		return false;

	if ( ( newFlags & flagsMask ) != newFlags )
	{
		ui::BeepSignal();		// error: attempt to enter flags outside of mask 
		TRACE( _T(" -CFlagsEdit::HandleTextChange(): newFlags=0x%08X are outside of mask=0x%08X\n"), newFlags & flagsMask, flagsMask );
	}

	newFlags &= flagsMask;
	return UserSetFlags( newFlags );		// true if flags have changed
}

void CFlagsEdit::PreSubclassWindow( void )
{
	CEdit::PreSubclassWindow();
	CTextEdit::SetFixedFont( this );
	InitControl();
}


// message handlers

BEGIN_MESSAGE_MAP( CFlagsEdit, CEdit )
	ON_CONTROL_REFLECT_EX( EN_CHANGE, OnEnChange_Reflect )
	ON_MESSAGE( WM_COPY, OnWmCopy )
	ON_COMMAND( CM_COPY_FORMATTED, OnCopy )
	ON_UPDATE_COMMAND_UI( CM_COPY_FORMATTED, OnUpdateCopy )
END_MESSAGE_MAP()

BOOL CFlagsEdit::PreTranslateMessage( MSG* pMsg )
{
	return m_accel.Translate( pMsg, m_hWnd ) || CEdit::PreTranslateMessage( pMsg );
}

BOOL CFlagsEdit::OnEnChange_Reflect( void )
{
	return !HandleTextChange();
}

LRESULT CFlagsEdit::OnWmCopy( WPARAM, LPARAM )
{
	OnCopy();
	return 0L;
}

void CFlagsEdit::OnCopy( void )
{
	CTextClipboard::CopyText( Format(), m_hWnd );
}

void CFlagsEdit::OnUpdateCopy( CCmdUI* /*pCmdUI*/ )
{
}
