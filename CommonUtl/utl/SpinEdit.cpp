
#include "stdafx.h"
#include "SpinEdit.h"
#include "SpinTargetButton.h"
#include "StringUtilities.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CSpinEdit::CSpinEdit( bool useSpin /*= true*/, const std::locale& loc /*= num::GetEmptyLocale()*/ )
	: CTextEdit( false )
	, m_locale( loc )
	, m_validRange( num::PositiveRange< int >() )
{
	SetUseSpin( useSpin );
}

CSpinEdit::~CSpinEdit()
{
}

void CSpinEdit::SetUseSpin( bool useSpin /*= true*/ )
{
	m_pSpinButton.reset( useSpin ? new CSpinTargetButton( this, this ) : NULL );
}

bool CSpinEdit::SpinBy( int delta )
{
	if ( IsWritable() )
	{
		CScopedInternalChange internalChange( &m_userChange );		// force user change mode so that SetNumericValue will send EN_CHANGE
		ui::TakeFocus( *this );
		SpinValueBy( delta );
		SetModify();
	}
	return true;		// handled (no default processing)
}

bool CSpinEdit::HasInvalidText( void ) const
{
	bool valid;
	GetNumericValue( &valid );
	return !valid || CTextEdit::HasInvalidText();
}

int CSpinEdit::GetNumericValue( bool* pValid /*= NULL*/ ) const
{
	int value;
	bool valid = num::ParseNumber( value, GetText(), m_locale );
	if ( valid && !CheckValidNumber( value ) )
	{
		valid = false;
		ui::BeepSignal( MB_OK );
	}

	if ( pValid != NULL )
		*pValid = valid;

	if ( !valid )
	{	// make default value
		value = 0;
		m_validRange.Constrain( value );
	}
	return value;
}

void CSpinEdit::SetNumericValue( int value )
{
	if ( m_validRange.Constrain( value ) )
		ui::BeepSignal( MB_OK );

	SetText( num::FormatNumber( value, m_locale ) );
}

void CSpinEdit::SpinValueBy( int delta )
{
	ASSERT( IsWritable() );

	bool valid;
	int value = GetNumericValue( &valid );

	if ( valid )
	{
		value += delta;
		if ( !CheckValidNumber( value ) )
			ui::BeepSignal( MB_OK );
	}

	SetNumericValue( value );
}

bool CSpinEdit::CheckValidNumber( int& rNumber ) const
{
	if ( !m_validRange.Constrain( rNumber ) )
		return true;

	if ( IsWrap() )
	{
		rNumber = rNumber >= m_validRange.m_end ? m_validRange.m_start : m_validRange.m_end;
		return true;
	}

	return false;
}

void CSpinEdit::PreSubclassWindow( void )
{
	CTextEdit::PreSubclassWindow();

//	if ( HasFlag( GetStyle(), ES_NUMBER ) )
//		ModifyStyle( ES_NUMBER, 0 );		// shouldn't use number style due to custom formatting

	if ( m_pSpinButton.get() != NULL )
		m_pSpinButton->Create( UDS_ALIGNRIGHT );
}


// message handlers

BEGIN_MESSAGE_MAP( CSpinEdit, CTextEdit )
	ON_WM_WINDOWPOSCHANGED()
	ON_WM_ENABLE()
	ON_WM_STYLECHANGED()
END_MESSAGE_MAP()

void CSpinEdit::OnWindowPosChanged( WINDOWPOS* pWndPos )
{
	bool mustLayout = !HasFlag( pWndPos->flags, SWP_NOMOVE ) || !HasFlag( pWndPos->flags, SWP_NOSIZE );
	CTextEdit::OnWindowPosChanged( pWndPos );

	if ( mustLayout && m_pSpinButton.get() != NULL )
		m_pSpinButton->Layout();
}

void CSpinEdit::OnEnable( BOOL enable )
{
	CTextEdit::OnEnable( enable );
	if ( m_pSpinButton.get() != NULL )
		m_pSpinButton->UpdateState();
}

void CSpinEdit::OnStyleChanged( int styleType, STYLESTRUCT* pStyleStruct )
{
	CTextEdit::OnStyleChanged( styleType, pStyleStruct );
	if ( m_pSpinButton.get() != NULL )
		m_pSpinButton->UpdateState();
}
