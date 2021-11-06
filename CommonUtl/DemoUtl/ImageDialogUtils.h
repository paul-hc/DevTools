#ifndef ImageDialogUtils_h
#define ImageDialogUtils_h
#pragma once

#include "utl/ContainerUtilities.h"
#include "utl/Path.h"
#include "utl/UI/Pixel.h"
#include <map>


class CDibPixels;


namespace utl
{
	std::tstring FormatHexColor( const CPixelBGR& color );
	std::tstring FormatColorInfo( const CPixelBGR& color, size_t colorTablePos = std::tstring::npos, const CPoint& pos = CPoint( -1, -1 ) );

	void HorizGradientAlphaBlend( CDibPixels& rPixels, COLORREF bkColor );
}


class CDibSection;
class CEnumTags;


struct CColorTableOptions			// persistent options
{
	enum Mode { ImageColorTable, System_1_bit, System_4_bit, System_8_bit };
	static const CEnumTags& GetTags_Mode( void );

	CColorTableOptions( void ) : m_uniqueColors( false ), m_mode( ImageColorTable ) {}

	void Load( const TCHAR* pSection );
	void Save( const TCHAR* pSection ) const;
public:
	Mode m_mode;
	bool m_uniqueColors;
	bool m_showLabels;
};


struct CColorTable : public CColorTableOptions
{
	CColorTable( void );

	bool IsEmpty( void ) const { return m_colors.empty(); }

	void Clear( void );
	void Build( CDibSection* pDib );

	bool IsDuplicateAt( size_t pos ) const { return utl::Contains( m_dupColorsPos, pos ); }
	size_t FindUniqueColorPos( size_t pos ) const { return utl::FindPos( m_colors, m_colors[ pos ] ); }

	size_t FindColorPos( COLORREF color ) const { return utl::FindPos( m_colors, color ); }

	bool HasSelectedColor( void ) const { return m_selPos != std::tstring::npos; }
	bool SelectColor( COLORREF color );

	void Draw( CDC* pDC, const CRect& clientRect ) const;
public:
	std::vector< COLORREF > m_colors;
	size_t m_totalColors;
	size_t m_selPos;
	std::vector< size_t > m_dupColorsPos;
};


class CColorTableRenderer
{
public:
	CColorTableRenderer( const CColorTable* pColorTable, const CRect& clientRect );

	void Draw( CDC* pDC ) const;

	CRect MakeCellRect( UINT x, UINT y ) const;
	CRect MakeCellRect( size_t pos ) const { return MakeCellRect( (UINT)pos % m_columns, (UINT)pos / m_columns ); }
private:
	CSize ComputeCoreSize( void );
	CSize ComputeCellLayout( const CRect& rect );
	void AlignTableCore( void );

	void EraseEdges( CDC* pDC ) const;
	void DrawSelectedCell( CDC* pDC ) const;
	bool CanShowLabels( CDC* pDC ) const;
private:
	const CColorTable* m_pColorTable;
	CRect m_clientRect;
	CRect m_coreRect;

	// cell layout
	UINT m_columns, m_rows;
	CSize m_cellSize;
};


class CImageTranspColors
{
public:
	CImageTranspColors( void ) {}

	void Load( const TCHAR* pSection );
	void Save( const TCHAR* pSection ) const;

	COLORREF Lookup( const fs::CPath& imagePath ) const;
	bool Register( const fs::CPath& imagePath, COLORREF transpColor );
	bool Unregister( const fs::CPath& imagePath ) { return 1 == m_transpColorMap.erase( imagePath ); }
private:
	std::map< fs::CPath, COLORREF > m_transpColorMap;
	static const TCHAR s_entry[];
};


struct CModeData
{
	CModeData( const TCHAR* pLabels );		// use "|" as separator
	~CModeData() { Clear(); }

	unsigned int GetZoneCount( void ) const { return static_cast<unsigned int>( m_labels.size() ); }
	void Clear( void );
	void PushDib( std::auto_ptr<CDibSection>& rpDib ) { m_dibs.push_back( rpDib.release() ); ENSURE( m_dibs.size() < m_labels.size() ); }
public:
	std::vector< std::tstring > m_labels;
	std::vector< CDibSection* > m_dibs;
};


#include "utl/UI/SpinEdit.h"


class CColorChannelEdit : public CSpinEdit
{
public:
	CColorChannelEdit( UINT editId, BYTE* pChannel, bool pixelChannel = false )
		: m_editId( editId ), m_pChannel( pChannel ), m_pixelChannel( pixelChannel ) { ASSERT_PTR( m_pChannel ); SetFullRange<BYTE>(); }

	bool IsPixelChannel( void ) const { return m_pixelChannel; }		// must regenerate effect DIBs
	void DDX_Channel( CDataExchange* pDX ) { DDX_Number( pDX, *m_pChannel, m_editId ); }

	void SyncValueWith( const CColorChannelEdit* pRefEdit )
	{
		ASSERT_PTR( pRefEdit );
		CScopedInternalChange scopedUserChange( &m_userChange );
		SetNumber< BYTE >( pRefEdit->GetNumber< BYTE >() );
	}
protected:
	// base overrides
	virtual void OnValueChanged( void ) { *m_pChannel = GetNumber< BYTE >(); }
private:
	UINT m_editId;
	BYTE* m_pChannel;
	bool m_pixelChannel;
};


#include "utl/UI/SampleView.h"


class CColorSample : public CSampleView
				   , public ISampleCallback
{
public:
	CColorSample( ISampleCallback* pRoutePixelInfo = NULL ) : CSampleView( this ), m_color( CLR_NONE ), m_pRoutePixelInfo( pRoutePixelInfo ) {}

	COLORREF GetColor( void ) const { return m_color; }
	void SetColor( COLORREF color ) { m_color = color; SafeRedraw(); }

	void Reset( void ) { SetColor( CLR_NONE ); }
protected:
	virtual bool RenderSample( CDC* pDC, const CRect& clientRect );
	virtual void ShowPixelInfo( const CPoint& pos, COLORREF color );
private:
	COLORREF m_color;
	ISampleCallback* m_pRoutePixelInfo;			// multiple samples sharing same info target
};


#endif // ImageDialogUtils_h
