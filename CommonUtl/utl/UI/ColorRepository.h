#ifndef ColorRepository_h
#define ColorRepository_h
#pragma once

#include "StdColors.h"


class CEnumTags;
class CColorBatch;


namespace ui
{
	enum ColorBatch
	{
		System_Colors, Standard_Colors, Custom_Colors, DirectX_Colors, HTML_Colors, X11_Colors,	// color-repo batches
		Shades_Colors, User_Colors,																// implementation (not based in repository)
		_ColorBatchCount
	};

	const CEnumTags& GetTags_ColorBatch( void );
}


#define COLOR_ENTRY( stdColor )  CColorEntry( (stdColor), CColorEntry::FindScopedLiteral( #stdColor ) )


class CColorEntry		// a named colour
{
public:
	CColorEntry( void ) : m_color( CLR_NONE ), m_pBatch( nullptr ) {}
	CColorEntry( COLORREF color, const char* pLiteral );		// converts "DarkGrey80" to "Dark Grey 80"
	explicit CColorEntry( COLORREF color, const std::tstring& name ) : m_color( color ), m_name( name ), m_pBatch( nullptr ) {}

	bool operator==( COLORREF rawColor ) const { return m_color == rawColor; }		// for std::find algorithms
	static const char* FindScopedLiteral( const char* pScopedColorName );

	const CColorBatch* GetColorBatch( void ) const { return m_pBatch; }
public:
	COLORREF m_color;
	std::tstring m_name;
private:
	const CColorBatch* m_pBatch;

	friend class CColorBatch;
};


class CColorBatch : private utl::noncopyable
{
public:
	CColorBatch( ui::ColorBatch batchType, UINT baseCmdId, size_t capacity, int layoutCount = 1 /*single-column*/ );
	~CColorBatch();

	ui::ColorBatch GetBatchType( void ) const { return m_batchType; }
	const std::tstring& GetBatchName( void ) const;

	const std::vector<CColorEntry>& GetColors( void ) const { return m_colors; }
	const CColorEntry* FindColor( COLORREF rawColor ) const;
	bool ContainsColor( COLORREF rawColor ) const { return FindColor( rawColor ) != nullptr; }

	UINT GetCmdIdAt( size_t index ) const { ASSERT( index < m_colors.size() ); return m_baseCmdId + static_cast<UINT>( index ); }
	size_t FindCmdIndex( UINT cmdId ) const;

	void GetLayout( OUT size_t* pRowCount, OUT size_t* pColumnCount ) const;

	void Add( const CColorEntry& colorEntry );

	static CColorBatch* MakeShadesBatch( size_t shadesCount, COLORREF selColor );
private:
	const ui::ColorBatch m_batchType;
	const UINT m_baseCmdId;
	std::vector<CColorEntry> m_colors;
	int m_layoutCount;						// color picker layout: columnCount if positive, rowCount if negative
};


class CColorBatchGroup
{
public:
	CColorBatchGroup( void ) {}

	const std::vector<CColorBatch*>& GetBatches( void ) const { return m_colorBatches; }
	std::vector<CColorBatch*>& RefBatches( void ) { return m_colorBatches; }

	const CColorBatch* FindBatch( ui::ColorBatch batchType ) const;
	const CColorBatch* FindSystemBatch( void ) const { return FindBatch( ui::System_Colors ); }

	// color entries lookup
	const CColorEntry* FindColorEntry( COLORREF rawColor ) const;
	void QueryMatchingColors( OUT std::vector<const CColorEntry*>& rColorEntries, COLORREF rawColor ) const;
	std::tstring FormatColorMatch( COLORREF rawColor, bool multiple = true ) const;
protected:
	std::vector<CColorBatch*> m_colorBatches;		// no ownership
};


class CColorRepository : public CColorBatchGroup
{
	CColorRepository( void );
	~CColorRepository() { Clear(); }
public:
	static const CColorRepository* Instance( void );

	void Clear( void );		// owns the color batches

	const CColorBatch* GetBatch( ui::ColorBatch batchType ) const { REQUIRE( batchType < ui::_ColorBatchCount ); return m_colorBatches[ batchType ]; }
	const CColorBatch* GetSystemBatch( void ) const { return GetBatch( ui::System_Colors ); }
	const CColorBatch* GetStockColorBatch( void ) const { return GetBatch( ui::HTML_Colors ); }

	enum BatchMenuBaseCmdId
	{
		BaseId_System = 1000, BaseId_Shades = 2000, BaseId_User = 3000,
		BaseId_Standard = 11000, BaseId_Custom = 12000,
		BaseId_DirectX = 13000, BaseId_HTML = 14000, BaseId_X11 = 15000
	};
private:
	static CColorBatch* MakeBatch_System( void );
	static CColorBatch* MakeBatch_Standard( void );
	static CColorBatch* MakeBatch_Custom( void );
	static CColorBatch* MakeBatch_DirectX( void );
	static CColorBatch* MakeBatch_HTML( void );
	static CColorBatch* MakeBatch_X11( void );
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

	struct ToBatchName
	{
		const std::tstring& operator()( const CColorEntry* pColorEntry ) const
		{
			ASSERT_PTR( pColorEntry );
			return pColorEntry->GetColorBatch()->GetBatchName();
		}
	};

	struct ToQualifiedColorName
	{
		std::tstring operator()( const CColorEntry* pColorEntry, const TCHAR* pSep = _T(" ") ) const
		{
			ASSERT_PTR( pColorEntry );
			return pColorEntry->m_name + pSep + _T('(') + pColorEntry->GetColorBatch()->GetBatchName() + _T(')');
		}
	};
}


#endif // ColorRepository_h
