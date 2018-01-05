#ifndef LogPalette_h
#define LogPalette_h
#pragma once


class CLogPalette
{
public:
	CLogPalette( const COLORREF colors[], size_t count );
	~CLogPalette();

	LOGPALETTE* Get( void ) { return m_pLogPalette; }

	void SetAt( size_t pos, COLORREF color );

	void MakePalette( CPalette* pPalette ) const
	{
		VERIFY( pPalette->CreatePalette( m_pLogPalette ) );
	}

	static void MakePalette( CPalette* pPalette, const COLORREF colors[], size_t count )
	{
		CLogPalette logPalette( colors, count );
		VERIFY( pPalette->CreatePalette( logPalette.Get() ) );
	}
private:
	BYTE* m_pBuffer;
	LOGPALETTE* m_pLogPalette;
};


#endif // LogPalette_h
