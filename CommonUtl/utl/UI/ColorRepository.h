#ifndef ColorRepository_h
#define ColorRepository_h
#pragma once

#include "Color.h"
#include "StdColors.h"
#include <afxtempl.h>


#define COLOR_ENTRY( stdColor )  CColorEntry( (stdColor), CColorEntry::FindScopedLiteral( #stdColor ) )


namespace ui
{
	typedef CArray<COLORREF, COLORREF> TMFCColorArray;
	typedef CList<COLORREF, COLORREF> TMFCColorList;


	inline bool IsHalftoneTable( ui::StdColorTable tableType ) { return tableType >= ui::Halftone16_Colors && tableType <= ui::Halftone256_Colors; }
}


class CColorTable;
class CColorStore;


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
	CColorTable( ui::StdColorTable tableType, size_t capacity, int layoutCount = 0 /*default color bar columns*/ );
	~CColorTable();

	void Reset( size_t capacity, int layoutCount );
	void Clear( void );
	void CopyFrom( const CColorTable& srcTable );
	void StoreTableInfo( const CColorStore* pParentStore, UINT tableItemId );

	const CColorStore* GetParentStore( void ) const { return m_pParentStore; }
	UINT GetTableItemId( void ) const { return m_tableItemId; }

	ui::StdColorTable GetTableType( void ) const { return m_tableType; }
	const std::tstring& GetTableName( void ) const;

	bool IsHalftoneTable( void ) const { return ui::IsHalftoneTable( GetTableType() ); }

	bool IsEmpty( void ) const { return m_colors.empty(); }
	const std::vector<CColorEntry>& GetColors( void ) const { return m_colors; }
	const CColorEntry* FindColor( COLORREF rawColor ) const;
	bool ContainsColor( COLORREF rawColor ) const { return FindColor( rawColor ) != nullptr; }
	const CColorEntry* FindEvaluatedColor( COLORREF color ) const;

	int GetColumnCount( void ) const;
	bool GetLayout( OUT size_t* pRowCount, OUT size_t* pColumnCount ) const;

	void Add( const CColorEntry& colorEntry );

	// CMFCColorButton support
	void QueryMfcColors( ui::TMFCColorArray& rColorArray ) const;
	void QueryMfcColors( ui::TMFCColorList& rColorList ) const;
	size_t RegisterColorButtonNames( void ) const;			// for shared color button tooltips stored in CMFCColorButton
	void SetupMfcColors( const ui::TMFCColorArray& customColors, int columnCount = 0 );

	size_t SetupShadesTable( COLORREF selColor, size_t columnCount );	// 3 rows x columnCount - Lighter, Darker, Desaturated shades
private:
	const ui::StdColorTable m_tableType;
	std::vector<CColorEntry> m_colors;
	int m_layoutCount;						// color picker layout: columnCount if positive, rowCount if negative

	const CColorStore* m_pParentStore;
	UINT m_tableItemId;						// menu item ID, used in context menus
};


class CColorStore			// collection of color tables
{
public:
	CColorStore( void ) {}

	const std::vector<CColorTable*>& GetTables( void ) const { return m_colorTables; }
	const CColorTable* GetTableAt( size_t pos ) const { ASSERT( pos < m_colorTables.size() ); return m_colorTables[ pos ]; }
	std::vector<CColorTable*>& RefTables( void ) { return m_colorTables; }

	const CColorTable* FindTable( ui::StdColorTable tableType ) const;
	const CColorTable* FindTableByName( const std::tstring& tableName ) const;

	// color entries lookup
	const CColorEntry* FindColorEntry( COLORREF rawColor ) const;
	void QueryMatchingColors( OUT std::vector<const CColorEntry*>& rColorEntries, COLORREF rawColor ) const;
	std::tstring FormatColorMatch( COLORREF rawColor, bool multiple = true ) const;

	static const CColorTable* LookupSystemTable( void );
protected:
	void Clear( void );		// only for stores that own the color tables
	void AddTable( CColorTable* pNewTable, UINT tableItemId );
private:
	std::vector<CColorTable*> m_colorTables;		// no ownership
};


class CScratchColorStore : public CColorStore		// contains {ui::Shades_Colors, ui::UserCustom_Colors}
{
public:
	CScratchColorStore( void );
	~CScratchColorStore() { Clear(); }		// owns the color tables

	CColorTable* GetShadesTable( void ) const { return m_pShadesTable; }
	CColorTable* GetUserCustomTable( void ) const { return m_pUserCustomTable; }

	size_t UpdateShadesTable( COLORREF selColor, size_t columnCount ) { return m_pShadesTable->SetupShadesTable( selColor, columnCount ); }
	void UpdateUserCustomTable( const ui::TMFCColorArray& customColors, int columnCount = 0 ) { return m_pUserCustomTable->SetupMfcColors( customColors, columnCount ); }
private:
	CColorTable* m_pShadesTable;
	CColorTable* m_pUserCustomTable;
};


class CHalftoneRepository : public CColorStore		// contains {ui::Halftone16_Colors, ui::Halftone20_Colors, ui::Halftone256_Colors}
{
	CHalftoneRepository( void );
	~CHalftoneRepository() { Clear(); }		// owns the color tables
public:
	static const CHalftoneRepository* Instance( void );
private:
	static CColorTable* MakeHalftoneTable( ui::StdColorTable halftoneType, size_t halftoneSize, unsigned int columnCount );
	static CColorTable* SetupHalftoneTable( CColorTable* pHalftoneTable, size_t halftoneSize );
};


class CColorRepository : public CColorStore			// contains {ui::Standard_Colors, ... ui::WindowsSys_Colors}
{
	CColorRepository( void );
	~CColorRepository() { Clear(); }		// owns the color tables
public:
	static const CColorRepository* Instance( void );

	const CColorTable* GetSystemColorTable( void ) const { return m_pWindowsSystemTable; }
private:
	static CColorTable* MakeTable_Standard( void );
	static CColorTable* MakeTable_Dev( void );
	static CColorTable* MakeTable_Office2003( void );
	static CColorTable* MakeTable_Office2007( void );
	static CColorTable* MakeTable_DirectX( void );
	static CColorTable* MakeTable_HTML( void );
	static CColorTable* MakeTable_WindowsSystem( void );
private:
	CColorTable* m_pWindowsSystemTable;
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
