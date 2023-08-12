#ifndef ColorRepository_h
#define ColorRepository_h
#pragma once

#include "Color.h"
#include "StdColors.h"
#include <afxtempl.h>


#define COLOR_ENTRY( stdColor )  CColorEntry( (stdColor), CColorEntry::FindScopedLiteral( #stdColor ) )


class CPalette;


namespace mfc
{
	typedef CArray<COLORREF, COLORREF> TColorArray;
	typedef CList<COLORREF, COLORREF> TColorList;
}


namespace ui
{
	// color table categories:
	//
	inline bool IsHalftoneTable( ui::StdColorTable tableType ) { return tableType >= ui::Halftone16_Colors && tableType <= ui::Halftone256_Colors; }
	inline bool IsRepositoryTable( ui::StdColorTable tableType ) { return tableType >= ui::Office2003_Colors && tableType <= ui::WindowsSys_Colors; }		// originally named colors in CColorRepository?
	inline bool IsWindowsSysTable( ui::StdColorTable tableType ) { return ui::WindowsSys_Colors == tableType; }
	inline bool IsScratchTable( ui::StdColorTable tableType ) { return ui::Shades_Colors == tableType || ui::UserCustom_Colors == tableType; }
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

	bool IsRepoColor( void ) const;
	const CColorTable* GetParentTable( void ) const { return m_pParentTable; }

	COLORREF GetColor( void ) const { return m_color; }
	ui::TDisplayColor EvalColor( void ) const { return ui::EvalColor( m_color ); }

	const std::tstring& GetName( void ) const { return m_name; }
	void SetName( const std::tstring& name ) { m_name = name; }

	std::tstring FormatColor( const TCHAR* pFieldSep = s_fieldSep, bool suffixTableName = true ) const;
	std::tostream& TabularOutColor( IN OUT std::tostream& os, size_t pos ) const;
private:
	COLORREF m_color;
	std::tstring m_name;
public:
	static const TCHAR s_fieldSep[];
	static const TCHAR s_multiLineTipSep[];
private:
	const CColorTable* m_pParentTable;

	friend class CColorTable;
};


class CColorTable : private utl::noncopyable
{
public:
	CColorTable( ui::StdColorTable tableType, size_t capacity, int layoutCount = 0 /*default color bar columns*/ );
	virtual ~CColorTable();

	void Reset( size_t capacity, int layoutCount );
	void Clear( void );
	void CopyFrom( const CColorTable& srcTable );
	void StoreTableInfo( const CColorStore* pParentStore, UINT tableItemId );

	const CColorStore* GetParentStore( void ) const { return m_pParentStore; }
	UINT GetTableItemId( void ) const { return m_tableItemId; }

	ui::StdColorTable GetTableType( void ) const { return m_tableType; }
	const std::tstring& GetTableName( void ) const;
	const std::tstring& GetShortTableName( void ) const;

	bool IsHalftoneTable( void ) const { return ui::IsHalftoneTable( GetTableType() ); }
	bool IsSysColorTable( void ) const { return ui::WindowsSys_Colors == m_tableType; }
	bool IsRepoTable( void ) const;

	bool IsEmpty( void ) const { return m_colors.empty(); }
	const std::vector<CColorEntry>& GetColors( void ) const { return m_colors; }
	const CColorEntry& GetColorAt( size_t pos ) const { ASSERT( pos < m_colors.size() ); return m_colors[ pos ]; }

	ui::TDisplayColor GetDisplayColorAt( size_t pos ) const { return EncodeRawColor( GetColorAt( pos ).GetColor() ); }

	const CColorEntry* FindColor( COLORREF rawColor ) const;
	bool ContainsColor( COLORREF rawColor ) const { return FindColor( rawColor ) != nullptr; }

	int GetColumnCount( void ) const;
	virtual int GetCompactGridColumnCount( void ) const { return GetColumnCount(); }	// for nameless display in CMFCColorBar
	virtual bool BrowseNamedPopupGrid( void ) const { return false; }

	size_t Add( const CColorEntry& colorEntry, size_t atPos = utl::npos );

	std::tostream& TabularOut( IN OUT std::tostream& os ) const;

	// CMFCColorButton support
	void QueryMfcColors( OUT mfc::TColorArray& rColorArray ) const;
	void QueryMfcColors( OUT mfc::TColorList& rColorList ) const;
	void SetupMfcColors( const mfc::TColorArray& customColors, int columnCount = 0 );

	bool BuildPalette( OUT CPalette* pPalette ) const;

	size_t SetupShadesTable( COLORREF selColor, size_t columnCount );	// 3 rows x columnCount - Lighter, Darker, Desaturated shades
protected:
	virtual int ToDisplayColumnCount( int columnCount ) const;

	virtual void OnTableChanged( void ) {}
	virtual ui::TDisplayColor EncodeRawColor( COLORREF rawColor ) const { ASSERT( ui::IsRealColor( rawColor ) ); return rawColor; }

	bool Remove( COLORREF rawColor );
	size_t Clamp( size_t maxCount );
private:
	const ui::StdColorTable m_tableType;
	std::vector<CColorEntry> m_colors;
	int m_layoutCount;						// color picker layout: columnCount if positive, rowCount if negative

	const CColorStore* m_pParentStore;
	UINT m_tableItemId;						// menu item ID, used in context menus
};


#include <unordered_map>


class CSystemColorTable : public CColorTable	// table of Windows System colors, that require custom encoding for display colors
{
public:
	CSystemColorTable( ui::StdColorTable tableType, size_t capacity, int columnCount, UINT compactGridColumnCount );
	virtual ~CSystemColorTable();

	virtual int GetCompactGridColumnCount( void ) const;
	virtual bool BrowseNamedPopupGrid( void ) const { return true; }		// top-to-bottom filled named grid
protected:
	// base overrides
	virtual void OnTableChanged( void ) overrides(CColorTable);
	virtual ui::TDisplayColor EncodeRawColor( COLORREF rawColor ) const override;
private:
	UINT m_compactGridColumnCount;			// used for display in CMFCColorBar (compact grid, nameless)
	std::unordered_map<COLORREF, ui::TDisplayColor> m_displaySysColors;		// raw -> unique encoded colors, to disambiguate selected color in MFC color bars
};


class CCommandModel;


class CRecentColorTable : public CSystemColorTable	// table of most recent used colors; may also contain system colors, so it inherits from CSystemColorTable
{
public:
	CRecentColorTable( size_t maxColors = 20 );

	// push means at the front of the table (LIFO)
	bool PushColor( COLORREF rawColor, COLORREF autoColor /*= CLR_NONE*/ );
	size_t PushColorHistory( const CCommandModel* pCmdModel, COLORREF autoColor /*= CLR_NONE*/ );
private:
	bool PushFrontColorImpl( COLORREF rawColor, COLORREF autoColor );
protected:
	// base overrides
	virtual int ToDisplayColumnCount( int columnCount ) const overrides(CColorTable);
	virtual void OnTableChanged( void ) overrides(CSystemColorTable);
private:
	size_t m_maxColors;		// history limit
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
	CScratchColorStore( void );
	~CScratchColorStore() { Clear(); }		// owns the color tables
public:
	static CScratchColorStore* Instance( void );

	CColorTable* GetShadesTable( void ) const { return m_pShadesTable; }
	CColorTable* GetUserCustomTable( void ) const { return m_pUserCustomTable; }
	CRecentColorTable* GetRecentTable( void ) const { return m_pRecentTable; }

	size_t UpdateShadesTable( COLORREF selColor, size_t columnCount ) { return m_pShadesTable->SetupShadesTable( selColor, columnCount ); }
	void UpdateUserCustomTable( const mfc::TColorArray& customColors, int columnCount = 0 ) { return m_pUserCustomTable->SetupMfcColors( customColors, columnCount ); }
private:
	CColorTable* m_pShadesTable;
	CColorTable* m_pUserCustomTable;
	CRecentColorTable* m_pRecentTable;		// most recently used colors
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


class CColorRepository : public CColorStore			// contains {ui::Office2003_Colors, ... ui::WindowsSys_Colors}
{
	CColorRepository( void );
	~CColorRepository() { Clear(); }		// owns the color tables
public:
	static const CColorRepository* Instance( void );

	const CColorTable* GetSystemColorTable( void ) const { return m_pWindowsSystemTable; }
private:
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
			return pColorEntry->GetName();
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
			return pColorEntry->GetName() + pSep + _T('(') + pColorEntry->GetParentTable()->GetTableName() + _T(')');
		}
	};
}


#endif // ColorRepository_h
