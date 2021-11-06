
#include "stdafx.h"
#include "LogPalette.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CLogPalette::CLogPalette( const COLORREF colors[], size_t count )
	: m_pBuffer( new BYTE[ sizeof( LOGPALETTE ) + count * sizeof( PALETTEENTRY ) ] )
	, m_pLogPalette( reinterpret_cast<LOGPALETTE*>( m_pBuffer ) )
{
	m_pLogPalette->palVersion = 0x300;				// Windows 3.0?
	m_pLogPalette->palNumEntries = static_cast<WORD>( count );
	for ( size_t i = 0; i != count; ++i )
		SetAt( i, colors[ i ] );
}

CLogPalette::~CLogPalette()
{
	delete[] m_pBuffer;
}

void CLogPalette::SetAt( size_t pos, COLORREF color )
{
	m_pLogPalette->palPalEntry[ pos ].peRed = GetRValue( color );
	m_pLogPalette->palPalEntry[ pos ].peGreen = GetGValue( color );
	m_pLogPalette->palPalEntry[ pos ].peBlue = GetBValue( color );
	m_pLogPalette->palPalEntry[ pos ].peFlags = PC_EXPLICIT;
}
