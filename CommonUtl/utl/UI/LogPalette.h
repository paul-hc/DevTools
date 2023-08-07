#ifndef LogPalette_h
#define LogPalette_h
#pragma once


class CColorTable;


class CLogPalette
{
public:
	CLogPalette( size_t count );
	CLogPalette( const COLORREF colors[], size_t count );
	CLogPalette( const CColorTable& colorTable );
	~CLogPalette();

	LOGPALETTE* Get( void ) { return m_pLogPalette; }

	void SetAt( size_t pos, COLORREF color );

	bool MakePalette( OUT CPalette* pPalette ) const;

	static bool MakePalette( OUT CPalette* pPalette, const COLORREF colors[], size_t count );
private:
	void Construct( size_t count );
private:
	BYTE* m_pBuffer;
	LOGPALETTE* m_pLogPalette;
};


#endif // LogPalette_h
