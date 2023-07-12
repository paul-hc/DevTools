#ifndef ColorRepository_h
#define ColorRepository_h
#pragma once

#include "Color.h"
#include "StdColors.h"


#define COLOR_ENTRY( stdColor )  CColorEntry( (stdColor), CColorEntry::FindScopedLiteral( #stdColor ) )


namespace ui
{
	typedef CArray<COLORREF,COLORREF> TMFCColorArray;
	typedef CList<COLORREF,COLORREF> TMFCColorList;
}


class CColorTable;


class CColorEntry		// a named colour
{
public:
	CColorEntry( void ) : m_color( CLR_NONE ), m_pParentTable( nullptr ) {}
	CColorEntry( COLORREF color, const char* pLiteral );		// converts "DarkGray80" to "Dark Gray 80"
	explicit CColorEntry( COLORREF color, const std::tstring& name ) : m_color( color ), m_name( name ), m_pParentTable( nullptr ) {}

	bool operator==( COLORREF rawColor ) const { return m_color == rawColor; }		// for std::find algorithms
	static const char* FindScopedLiteral( const char* pScopedColorName );

	const CColorTable* GetParentTable( void ) const { return m_pParentTable; }

	COLORREF EvalColor( void ) const { return ui::EvalColor( m_color ); }
	std::tstring FormatColor( const TCHAR* pFieldSep = s_fieldSep, bool suffixTableName = true ) const;
public:
	COLORREF m_color;
	std::tstring m_name;

	static const TCHAR s_fieldSep[];
private:
	const CColorTable* m_pParentTable;

	friend class CColorTable;
};


class CColorTable : private utl::noncopyable
{
public:
	CColorTable( ui::StdColorTable tableType, UINT baseCmdId, size_t capacity, int layoutCount = 1 /*single-column*/ );
	~CColorTable();

	ui::StdColorTable GetTableType( void ) const { return m_tableType; }
	const std::tstring& GetTableName( void ) const;

	const std::vector<CColorEntry>& GetColors( void ) const { return m_colors; }
	const CColorEntry* FindColor( COLORREF rawColor ) const;
	bool ContainsColor( COLORREF rawColor ) const { return FindColor( rawColor ) != nullptr; }
	const CColorEntry* FindEvaluatedColor( COLORREF color ) const;

	UINT GetCmdIdAt( size_t index ) const { ASSERT( index < m_colors.size() ); return m_baseCmdId + static_cast<UINT>( index ); }
	size_t FindCmdIndex( UINT cmdId ) const;

	int GetColumnsLayout( void ) const;
	void GetLayout( OUT size_t* pRowCount, OUT size_t* pColumnCount ) const;

	void Add( const CColorEntry& colorEntry );

	void QueryMfcColors( ui::TMFCColorArray& rColorArray ) const;
	void QueryMfcColors( ui::TMFCColorList& rColorList ) const;

	static CColorTable* MakeShadesTable( size_t shadesCount, COLORREF selColor );
private:
	const ui::StdColorTable m_tableType;
	const UINT m_baseCmdId;
	std::vector<CColorEntry> m_colors;
	int m_layoutCount;						// color picker layout: columnCount if positive, rowCount if negative
};


class CColorStore			// collection of color tables
{
public:
	CColorStore( void ) {}

	const std::vector<CColorTable*>& GetTables( void ) const { return m_colorTables; }
	std::vector<CColorTable*>& RefTables( void ) { return m_colorTables; }

	const CColorTable* FindTable( ui::StdColorTable tableType ) const;
	const CColorTable* FindTableByName( const std::tstring& tableName ) const;
	const CColorTable* FindSystemTable( void ) const { return FindTable( ui::WindowsSys_Colors ); }

	// color entries lookup
	const CColorEntry* FindColorEntry( COLORREF rawColor ) const;
	void QueryMatchingColors( OUT std::vector<const CColorEntry*>& rColorEntries, COLORREF rawColor ) const;
	std::tstring FormatColorMatch( COLORREF rawColor, bool multiple = true ) const;
protected:
	std::vector<CColorTable*> m_colorTables;		// no ownership
};


class CColorRepository : public CColorStore
{
	CColorRepository( void );
	~CColorRepository() { Clear(); }
public:
	static const CColorRepository* Instance( void );

	void Clear( void );		// owns the color tables

	const CColorTable* GetTable( ui::StdColorTable tableType ) const { REQUIRE( tableType < ui::_ColorTableCount ); return m_colorTables[ tableType ]; }
	const CColorTable* GetSystemColorTable( void ) const { return GetTable( ui::WindowsSys_Colors ); }
	const CColorTable* GetStockColorTable( void ) const { return GetTable( ui::HTML_Colors ); }

	enum TableMenuBaseCmdId
	{
		BaseId_System = 1000, BaseId_Shades = 2000, BaseId_User = 3000,
		BaseId_Standard = 11000, BaseId_Custom = 12000,
		BaseId_Office2003 = 13000, BaseId_DirectX = 14000, BaseId_HTML = 15000, BaseId_X11 = 16000
	};
private:
	static CColorTable* MakeTable_System( void );
	static CColorTable* MakeTable_Standard( void );
	static CColorTable* MakeTable_Custom( void );
	static CColorTable* MakeTable_Office2003( void );
	static CColorTable* MakeTable_DirectX( void );
	static CColorTable* MakeTable_HTML( void );
};


namespace func
{
	struct ToColorName
	{
		const std::tstring& operator()( const CColorEntry* pColorEntry ) const
		{
			ASSERT_PTR( pColorEntry );
			return pColorEntry->m_name;
		}
	};

	struct ToTableName
	{
		const std::tstring& operator()( const CColorEntry* pColorEntry ) const
		{
			ASSERT_PTR( pColorEntry );
			return pColorEntry->GetParentTable()->GetTableName();
		}
	};

	struct ToQualifiedColorName
	{
		std::tstring operator()( const CColorEntry* pColorEntry, const TCHAR* pSep = _T(" ") ) const
		{
			ASSERT_PTR( pColorEntry );
			return pColorEntry->m_name + pSep + _T('(') + pColorEntry->GetParentTable()->GetTableName() + _T(')');
		}
	};
}


namespace pred
{
	struct HasEvalColor
	{
		HasEvalColor( COLORREF rawColor ) : m_color( ui::EvalColor( rawColor ) ) {}

		bool operator()( const CColorEntry& colorEntry ) const
		{
			return m_color == colorEntry.EvalColor();
		}
	private:
		COLORREF m_color;
	};
}


#endif // ColorRepository_h
