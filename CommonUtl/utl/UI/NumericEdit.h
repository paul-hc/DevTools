#ifndef NumericEdit_h
#define NumericEdit_h
#pragma once

#include "TextEdit.h"
#include "StringUtilities.h"
#include "WndUtils.h"


namespace num { enum Format { Decimal, Hexa }; }


// Note: CSpinEdit is the preferred class for number editing
//
template< typename Value >
class CNumericEdit : public CTextEdit
{
public:
	CNumericEdit( num::Format numFormat = num::Decimal ) : m_numFormat( numFormat ) {}

	bool IsNull( void ) const { Value value; return ParseValue( value, GetText() ); }
	bool SetNull( void ) { return SetText( m_nullText ); }

	bool GetValue( Value* pValue ) const { ASSERT_PTR( pValue ); return ParseValue( *pValue, GetText() ); }
	bool SetValue( Value value ) { return SetText( FormatValue( value ) ); }


	// base overrides

	virtual bool HasInvalidText( void ) const
	{
		std::tstring text = GetText();
		Value value;
		return !text.empty() && !ParseValue( value, text );
	}

	virtual bool NormalizeText( void )
	{
		std::tstring text = GetText();
		if ( text.empty() )
			return false;
		Value value;
		if ( !ParseValue( value, text ) )
		{
			ui::BeepSignal( MB_ICONWARNING );
			return false;
		}
		return SetValue( value );
	}
protected:
	std::tstring FormatValue( Value value ) const
	{
		if ( num::Hexa == m_numFormat )
			return num::FormatHexNumber( value );

		return num::FormatNumber( value );
	}

	bool ParseValue( Value& rValue, const std::tstring& text ) const
	{
		if ( num::Hexa == m_numFormat )
			return num::ParseHexNumber( rValue, text );

		return num::ParseNumber( rValue, text );
	}
private:
	num::Format m_numFormat;
	std::tstring m_nullText;
};


#endif // NumericEdit_h
