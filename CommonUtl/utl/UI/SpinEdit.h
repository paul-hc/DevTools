#ifndef SpinEdit_h
#define SpinEdit_h
#pragma once

#include "Range.h"
#include "TextEdit.h"
#include "StringUtilities.h"		// for numeric conversions


class CSpinTargetButton;
namespace num { const std::locale& GetEmptyLocale( void ); }

namespace ui
{
	interface ISpinTarget
	{
		virtual bool SpinBy( int by ) = 0;		// return true to do default spin processing
	};
}


class CSpinEdit : public CTextEdit
	, protected ui::ISpinTarget
{
public:
	CSpinEdit( bool useSpin = true, const std::locale& loc = num::GetEmptyLocale() );
	virtual ~CSpinEdit();

	bool UseSpin( void ) const { return GetSpinButton() != NULL; }
	void SetUseSpin( bool useSpin = true );

	void SetLocale( const std::locale& loc ) { m_locale = loc; }

	CSpinTargetButton* GetSpinButton( void ) const { return m_pSpinButton.get(); }
	CSpinButtonCtrl* GetSpinButtonCtrl( void ) const { return (CSpinButtonCtrl*)m_pSpinButton.get(); }		// no dependency on SpinTargetButton.h

	bool IsWritable( void ) const { return IsWindowEnabled() != FALSE && !HasFlag( GetStyle(), ES_READONLY ); }

	bool IsWrap( void ) const { return HasFlag( GetStyle(), UDS_WRAP ); }
	void SetWrap( bool wrap = true ) { wrap ? ModifyStyle( 0, UDS_WRAP ) : ModifyStyle( UDS_WRAP, 0 ); }

	Range<int> GetValidRange( void ) const { return m_validRange; }
	void SetValidRange( const Range<int>& validRange ) { ASSERT( validRange.IsNormalized() ); m_validRange = validRange; }

	template< typename NumericT > void SetFullRange( void ) { m_validRange = num::FullRange<NumericT>(); }

	int GetNumericValue( bool* pValid = NULL ) const;
	void SetNumericValue( int value );

	template< typename NumericT >
	NumericT GetNumber( bool* pValid = NULL ) const { return static_cast<NumericT>( GetNumericValue( pValid ) ); }

	template< typename NumericT >
	bool ParseNumber( NumericT* pNumber ) const;

	template< typename NumericT >
	void SetNumber( NumericT value ) { SetNumericValue( static_cast<int>( value ) ); }

	template< typename NumericT >
	void DDX_Number( CDataExchange* pDX, NumericT& rValue, int ctrlId = 0 );

	void SpinValueBy( int delta );
	bool CheckValidNumber( int& rNumber ) const;

	// base overrides
	virtual bool HasInvalidText( void ) const;
protected:
	// ui::ISpinTarget interface
	virtual bool SpinBy( int delta );
private:
	std::locale m_locale;
	Range<int> m_validRange;
	std::auto_ptr<CSpinTargetButton> m_pSpinButton;

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
protected:
	afx_msg void OnWindowPosChanged( WINDOWPOS* pWndPos );
	afx_msg void OnEnable( BOOL enable );
	afx_msg void OnStyleChanged( int styleType, STYLESTRUCT* pStyleStruct );

	DECLARE_MESSAGE_MAP()
};


// template code

template< typename NumericT >
bool CSpinEdit::ParseNumber( NumericT* pNumber ) const
{
	bool valid;
	NumericT number = GetNumber< NumericT >( &valid );
	if ( !valid )
		return false;

	*pNumber = number;
	return true;
}

template< typename NumericT >
void CSpinEdit::DDX_Number( CDataExchange* pDX, NumericT& rValue, int ctrlId /*= 0*/ )
{
	if ( ctrlId != 0 )
		DDX_Control( pDX, ctrlId, *this );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		SetNumber< NumericT >( rValue );
	else
		rValue = GetNumber< NumericT >();
}


#endif // SpinEdit_h
