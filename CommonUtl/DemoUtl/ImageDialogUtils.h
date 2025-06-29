#ifndef ImageDialogUtils_h
#define ImageDialogUtils_h
#pragma once

#include "utl/Path.h"
#include "utl/UI/Pixel.h"
#include <map>


class CDibPixels;


namespace utl
{
	std::tstring FormatHexColor( const CPixelBGR& color );
	std::tstring FormatColorInfo( const CPixelBGR& color, size_t colorBoardPos = std::tstring::npos, const CPoint& pos = CPoint( -1, -1 ) );

	void HorizGradientAlphaBlend( CDibPixels& rPixels, COLORREF bkColor );
}


class CDibSection;
class CEnumTags;


struct CColorBoardOptions			// persistent options
{
	enum Mode { ImageColorBoard, System_1_bit, System_4_bit, System_8_bit };
	static const CEnumTags& GetTags_Mode( void );

	CColorBoardOptions( void ) : m_uniqueColors( false ), m_mode( ImageColorBoard ), m_showLabels( false ) {}

	void Load( const TCHAR* pSection );
	void Save( const TCHAR* pSection ) const;
public:
	Mode m_mode;
	bool m_uniqueColors;
	bool m_showLabels;
};


struct CColorBoard : public CColorBoardOptions
{
	CColorBoard( void );

	bool IsEmpty( void ) const { return m_colors.empty(); }

	void Clear( void );
	void Build( CDibSection* pDib );

	bool IsDuplicateAt( size_t pos ) const;
	size_t FindUniqueColorPos( size_t pos ) const;

	size_t FindColorPos( COLORREF color ) const;

	bool HasSelectedColor( void ) const { return m_selPos != std::tstring::npos; }
	bool SelectColor( COLORREF color );

	void Draw( CDC* pDC, const CRect& clientRect ) const;
public:
	std::vector<COLORREF> m_colors;
	size_t m_totalColors;
	size_t m_selPos;
	std::vector<size_t> m_dupColorsPos;
};


class CColorBoardRenderer
{
public:
	CColorBoardRenderer( const CColorBoard* pColorBoard, const CRect& clientRect );

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
	const CColorBoard* m_pColorBoard;
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
	std::map<fs::CPath, COLORREF> m_transpColorMap;
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
	std::vector<std::tstring> m_labels;
	std::vector<CDibSection*> m_dibs;
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
		SetNumber<BYTE>( pRefEdit->GetNumber<BYTE>() );
	}
protected:
	// base overrides
	virtual void OnValueChanged( void ) { *m_pChannel = GetNumber<BYTE>(); }
private:
	UINT m_editId;
	BYTE* m_pChannel;
	bool m_pixelChannel;
};


class CPercentEdit : public CSpinEdit
{
public:
	CPercentEdit( UINT editId, TPercent* pPercentage )
		: m_editId( editId ), m_pPercentage( pPercentage ) { ASSERT_PTR( m_pPercentage ); SetValidRange( Range<TPercent>( -100, 100 ) ); }

	void DDX_Percent( CDataExchange* pDX ) { DDX_Number( pDX, *m_pPercentage, m_editId ); }

	void SyncValueWith( const CColorChannelEdit* pRefEdit )
	{
		ASSERT_PTR( pRefEdit );
		CScopedInternalChange scopedUserChange( &m_userChange );
		SetNumber<TPercent>( pRefEdit->GetNumber<TPercent>() );
	}
protected:
	// base overrides
	virtual void OnValueChanged( void ) { *m_pPercentage = GetNumber<TPercent>(); }
private:
	UINT m_editId;
	TPercent* m_pPercentage;
};


#include "utl/UI/SampleView.h"


class CColorSample : public CSampleView
	, public ui::ISampleCallback
{
public:
	CColorSample( ui::ISampleCallback* pRoutePixelInfo = nullptr );

	COLORREF GetColor( void ) const { return m_color; }
	void SetColor( COLORREF color ) { m_color = color; SafeRedraw(); }

	void Reset( void ) { SetColor( CLR_NONE ); }
protected:
	// ui::ui::ISampleCallback interface
	virtual bool RenderSample( CDC* pDC, const CRect& clientRect, CWnd* pCtrl ) implements(ui::ISampleCallback);
	virtual void ShowPixelInfo( const CPoint& pos, COLORREF color, CWnd* pCtrl ) implements(ui::ISampleCallback);
private:
	COLORREF m_color;
	ui::ISampleCallback* m_pRoutePixelInfo;			// multiple samples sharing same info target
};


#endif // ImageDialogUtils_h
