#ifndef ColorRepository_h
#define ColorRepository_h
#pragma once

#include "StdColors.h"


struct CColorEntry		// a named colour
{
	CColorEntry( void ) : m_color( CLR_NONE ) {}
	CColorEntry( COLORREF color, const char* pLiteral );		// converts "DarkGrey80" to "Dark Grey 80"
	explicit CColorEntry( COLORREF color, const std::tstring& name ) : m_color( color ), m_name( name ) {}
public:
	COLORREF m_color;
	std::tstring m_name;
};


struct CColorTable : private utl::noncopyable
{
	CColorTable( size_t rowCount, UINT baseCmdId );

	const CColorEntry* FindColor( COLORREF rawColor ) const;

	bool ContainsColor( COLORREF rawColor ) const { return FindColor( rawColor ) != nullptr; }

	UINT GetCmdIdAt( size_t index ) const;
	size_t FindCmdIndex( UINT cmdId ) const;

	static CColorTable* MakeShades( size_t shadesCount, COLORREF selColor );
public:
	size_t m_rowCount;
	const UINT m_baseCmdId;
	std::vector<CColorEntry> m_colors;
};


class CColorRepository
{
	CColorRepository( void );
	~CColorRepository();
public:
	static const CColorRepository& Instance( void );

	const std::vector<CColorTable*>& GetColorTables( void ) const { return m_colorTables; }
	const CColorEntry* FindEntry( COLORREF rawColor ) const;
	void QueryTablesWithColor( OUT std::vector<CColorTable*>& rColorTables, COLORREF rawColor ) const;

	static CColorTable* GetTable_Standard( void );
	static CColorTable* GetTable_Custom( void );
	static CColorTable* GetTable_Status( void );
	static CColorTable* GetHtmlTable( void );
	static CColorTable* GetPaintTable( void );
	static CColorTable* GetX11Table( void );
	static CColorTable* GetSystemTable( void );

	static CColorTable* GetStockColorTable( void );
public:
	enum TableBaseCmdId
	{
		BaseId_Standard = 123, BaseId_Custom = 200, SysTableBaseId = 325, ShadesTableBaseId = 360,
		X11TableBaseId = 400, HtmlTableBaseId = 500, PaintTableBaseId = 850
	};
private:
	std::vector<CColorTable*> m_colorTables;
};


#endif // ColorRepository_h
