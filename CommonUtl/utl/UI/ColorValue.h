#ifndef ColorValue_h
#define ColorValue_h
#pragma once

#include "Color.h"


class CColorValue		// a pair of color value (mutable), and Auto (default) value (immutable)
{
public:
	explicit CColorValue( COLORREF autoColor ) : m_color( CLR_NONE ), m_autoColor( autoColor ) { ASSERT( !ui::IsUndefinedColor( m_autoColor ) ); }
	CColorValue( COLORREF color, COLORREF autoColor ) : m_color( color ), m_autoColor( autoColor ) { ASSERT( !ui::IsUndefinedColor( m_autoColor ) ); }

	COLORREF Get( void ) const { return m_color; }
	void Set( COLORREF color ) { m_color = color; }
	COLORREF* GetPtr( void ) { return &m_color; }

	void Reset( void ) { m_color = m_autoColor; }

	COLORREF GetAutoColor( void ) const { return m_autoColor; }

	COLORREF GetActual( void ) const { return IsAutoColor() ? m_autoColor : m_color; }
	ui::TDisplayColor Evaluate( void ) const { return ui::EvalColor( GetActual() ); }

	bool IsAutoColor( void ) const { return CLR_NONE == m_color || CLR_DEFAULT == m_color; }
	bool IsSysColor( void ) const { return ui::IsSysColor( GetActual() ); }
private:
	CColorValue& operator=( COLORREF autoColor );		// hidden (not imlemented) - call Set() method explicitly
private:
	COLORREF m_color;
	COLORREF m_autoColor;		// the default color (non-const for assignment operator to work)
};


#endif // ColorValue_h
