
#include "pch.h"
#include "ImageDialogUtils.h"
#include "utl/Algorithms.h"
#include "utl/ContainerOwnership.h"
#include "utl/EnumTags.h"
#include "utl/StringUtilities.h"
#include "utl/UI/Color.h"
#include "utl/UI/DibSection.h"
#include "utl/UI/DibPixels.h"
#include "utl/UI/ImageProxy.h"
#include "utl/UI/Pixel.h"
#include "utl/UI/WndUtilsEx.h"
#include <set>
#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR entry_mode[] = _T("ColorTableMode");
	static const TCHAR entry_uniqueColors[] = _T("UniqueColors");
	static const TCHAR entry_showLabels[] = _T("ShowColorLabels");
}


namespace utl
{
	std::tstring FormatHexColor( const CPixelBGR& color )
	{
		return str::Format( _T("Hex RGB( %02X, %02X, %02X)"), color.m_red, color.m_green, color.m_blue );
	}

	std::tstring FormatColorInfo( const CPixelBGR& color, size_t colorTablePos /*= std::tstring::npos*/, const CPoint& pos /*= CPoint( -1, -1 )*/ )
	{
		std::tstring text = str::Format( _T(" R=%d G=%d B=%d   Hex RGB( %02X, %02X, %02X)"),
			color.m_red, color.m_green, color.m_blue,
			color.m_red, color.m_green, color.m_blue );

		if ( pos != CPoint( -1, -1 ) )
			text += str::Format( _T("  (X=%d, Y=%d)"), pos.x, pos.y );

		if ( colorTablePos != std::tstring::npos )
			text += str::Format( _T("   (color table index: %d)"), colorTablePos );
		return text;
	}

	void HorizGradientAlphaBlend( CDibPixels& rPixels, COLORREF bkColor )
	{
		CSize bitmapSize = rPixels.GetBitmapSize();
		UINT bitsPerPixel = rPixels.GetBitsPerPixel();
		ASSERT( bitsPerPixel >= 24 );

		for ( int y = 0; y != bitmapSize.cy; ++y )
			for ( int x = 0; x != bitmapSize.cx; ++x )
			{
				bool isTranspColor = false;
				if ( 24 == bitsPerPixel && rPixels.GetDib()->HasTranspColor() )
					isTranspColor = rPixels.GetPixel<CPixelBGR>( x, y ).GetColor() == rPixels.GetDib()->GetTranspColor();

				BYTE alpha = isTranspColor ? 0 : static_cast<BYTE>( (double)x / bitmapSize.cx * 255 );	// simple horizontal gradient

				func::AlphaBlend ma( alpha, bkColor );
				if ( 32 == bitsPerPixel )
					ma( rPixels.GetPixel<CPixelBGRA>( x, y ) );
				else
					ma( rPixels.GetPixel<CPixelBGR>( x, y ) );
			}
	}
}


// CColorTableOptions implementation

const CEnumTags& CColorTableOptions::GetTags_Mode( void )
{
	static const CEnumTags tags( _T("Image Color Table|System 1 bit (2 colors)|System 4 bit (16 colors)|System 8 bit (256 colors)") );
	return tags;
}

void CColorTableOptions::Load( const TCHAR* pSection )
{
	m_mode = (Mode)AfxGetApp()->GetProfileInt( pSection, reg::entry_mode, m_mode );
	m_uniqueColors = AfxGetApp()->GetProfileInt( pSection, reg::entry_uniqueColors, m_uniqueColors ) != FALSE;
	m_showLabels = AfxGetApp()->GetProfileInt( pSection, reg::entry_showLabels, m_showLabels ) != FALSE;
}

void CColorTableOptions::Save( const TCHAR* pSection ) const
{
	AfxGetApp()->WriteProfileInt( pSection, reg::entry_mode, m_mode );
	AfxGetApp()->WriteProfileInt( pSection, reg::entry_uniqueColors, m_uniqueColors );
	AfxGetApp()->WriteProfileInt( pSection, reg::entry_showLabels, m_showLabels );
}


// CColorTable implementation

CColorTable::CColorTable( void )
	: m_totalColors( 0 )
{
}

void CColorTable::Clear( void )
{
	m_colors.clear();
	m_totalColors = 0;
	m_selPos = std::tstring::npos;
	m_dupColorsPos.clear();
}

bool CColorTable::IsDuplicateAt( size_t pos ) const
{
	return utl::Contains( m_dupColorsPos, pos );
}

size_t CColorTable::FindUniqueColorPos( size_t pos ) const
{
	return utl::FindPos( m_colors, m_colors[ pos ] );
}

size_t CColorTable::FindColorPos( COLORREF color ) const
{
	return utl::FindPos( m_colors, color );
}

void CColorTable::Build( CDibSection* pDib )
{
	Clear();
	switch ( m_mode )
	{
		case ImageColorTable:
			if ( pDib != nullptr && pDib->IsIndexed() )
			{
				CDibSectionInfo info( pDib->GetHandle() );
				CScopedBitmapMemDC scopedBitmap( pDib );
				const std::vector<RGBQUAD>& colorTable = info.GetColorTable( pDib->GetBitmapMemDC() );

				m_colors.reserve( m_totalColors = colorTable.size() );
				for ( std::vector<RGBQUAD>::const_iterator itRgb = colorTable.begin(); itRgb != colorTable.end(); ++itRgb )
				{
					COLORREF color = CPixelBGR( *itRgb ).GetColor();
					if ( m_uniqueColors )
						utl::AddUnique( m_colors, color );
					else
						m_colors.push_back( color );
				}
			}
			break;
		case System_1_bit:
			CHalftoneColorTable::MakeColorTable( m_colors, m_totalColors = 1 << 1 );
			break;
		case System_4_bit:
			CHalftoneColorTable::MakeColorTable( m_colors, m_totalColors = 1 << 4 );
			break;
		case System_8_bit:
			CHalftoneColorTable::MakeColorTable( m_colors, m_totalColors = 1 << 8 );
			break;
	}

	std::set<COLORREF> uniqueColors;
	for ( size_t i = 0; i != m_colors.size(); ++i )
		if ( !uniqueColors.insert( m_colors[ i ] ).second )
			m_dupColorsPos.push_back( i );
}

bool CColorTable::SelectColor( COLORREF color )
{
	m_selPos = utl::FindPos( m_colors, color );
	return m_selPos != std::tstring::npos;
}

void CColorTable::Draw( CDC* pDC, const CRect& clientRect ) const
{
	if ( IsEmpty() )
	{
		::FillRect( *pDC, &clientRect, GetSysColorBrush( COLOR_BTNFACE ) );
		pDC->SetTextColor( GetSysColor( COLOR_GRAYTEXT ) );
		pDC->DrawText( _T("No color table"), -1, const_cast<CRect*>( &clientRect ), DT_SINGLELINE | DT_NOPREFIX | DT_CENTER | DT_VCENTER );
	}
	else
	{
		CColorTableRenderer renderer( this, clientRect );
		renderer.Draw( pDC );
	}
}


// CColorTableRenderer implementation

CColorTableRenderer::CColorTableRenderer( const CColorTable* pColorTable, const CRect& clientRect )
	: m_pColorTable( pColorTable )
	, m_clientRect( clientRect )
	, m_coreRect( CPoint( 0, 0 ), ComputeCoreSize() )
	, m_cellSize( ComputeCellLayout( m_coreRect ) )
{
	ASSERT_PTR( m_pColorTable );

	AlignTableCore();
}

CSize CColorTableRenderer::ComputeCoreSize( void )
{
	CSize coreSize = m_clientRect.Size();

	if ( 2 == m_pColorTable->m_colors.size() )
	{
		// limit the size of monochrome core (cell gets too big)
		coreSize.cx -= coreSize.cx / 5;
		coreSize.cy -= coreSize.cy / 5;

		enum { MaxCellWidth = 200, MaxCellHeight = 100 };
		coreSize.cx = std::min<long>( MaxCellWidth * 2, coreSize.cx );
		coreSize.cy = std::min<long>( MaxCellHeight, coreSize.cy );
	}
	return coreSize;
}

CSize CColorTableRenderer::ComputeCellLayout( const CRect& rect )
{
	ASSERT( !m_pColorTable->IsEmpty() );

	if ( 2 == m_pColorTable->m_colors.size() )
	{
		m_columns = 2;
		m_rows = 1;
	}
	else if ( 16 == m_pColorTable->m_colors.size() )
	{
		m_columns = 8;
		m_rows = 2;
	}
	else
	{
		m_columns = static_cast<UINT>( sqrt( (double)m_pColorTable->m_colors.size() ) );

		for ( ; ; ++m_columns )
		{
			m_rows = static_cast<UINT>( m_pColorTable->m_colors.size() ) / m_columns;
			CSize cellSize( rect.Width() / m_columns, rect.Height() / m_rows );
			if ( ui::GetAspectRatio( cellSize ) <= 1.5 )
				break;
		}

		if ( m_columns * m_rows < (UINT)m_pColorTable->m_colors.size() )
			++m_rows;			// add an extra row if there is a reminder
	}

	return CSize( rect.Width() / m_columns, rect.Height() / m_rows );		// cell size
}

void CColorTableRenderer::AlignTableCore( void )
{
	// center the table
	m_coreRect.SetRect( 0, 0, m_cellSize.cx * m_columns, m_cellSize.cy * m_rows );
	ui::CenterRect( m_coreRect, m_clientRect );
	if ( 16 == m_pColorTable->m_colors.size() )
	{
		m_coreRect.top = m_clientRect.top;
		m_coreRect.bottom = m_clientRect.bottom;			// cover vertically entirely
	}
}

CRect CColorTableRenderer::MakeCellRect( UINT x, UINT y ) const
{
	ASSERT( x < m_columns );
	ASSERT( y < m_rows );
	CPoint pos = m_coreRect.TopLeft() + CSize( m_cellSize.cx * x, m_cellSize.cy * y );
	return CRect( pos, m_cellSize );
}

void CColorTableRenderer::Draw( CDC* pDC ) const
{
	ASSERT( !m_pColorTable->IsEmpty() );

	EraseEdges( pDC );				// erase the background on the edges (no covered by cells)

	bool showLabels = CanShowLabels( pDC );
	HBRUSH hBkBrush = GetSysColorBrush( COLOR_BTNFACE );

	for ( size_t pos = 0, totalSize = m_columns * m_rows; pos != totalSize; ++pos )
	{
		CRect cellRect = MakeCellRect( pos );
		if ( pos < m_pColorTable->m_colors.size() )
		{
			::FillRect( *pDC, &cellRect, CBrush( m_pColorTable->m_colors[ pos ] ) );

			if ( showLabels )
			{
				pDC->SetTextColor( ui::GetContrastColor( m_pColorTable->m_colors[ pos ] ) );
				std::tstring label = num::FormatNumber( pos );
				pDC->DrawText( label.c_str(), static_cast<int>( label.length() ), &cellRect, DT_SINGLELINE | DT_NOPREFIX | DT_CENTER | DT_VCENTER );
			}

			if ( m_pColorTable->IsDuplicateAt( pos ) )
				pDC->DrawEdge( &cellRect, BDR_SUNKENOUTER, BF_RECT );
		}
		else
			::FillRect( *pDC, &cellRect, hBkBrush );
	}

	DrawSelectedCell( pDC );				// draw at the end so that the selection frame is not obscured by table drawing
}

void CColorTableRenderer::EraseEdges( CDC* pDC ) const
{
	if ( m_clientRect == m_coreRect )
		return;

	CRgn edgeRegion;
	edgeRegion.CreateRectRgnIndirect( &m_clientRect );
	ui::CombineWithRegion( &edgeRegion, m_coreRect, RGN_DIFF );
	::FillRgn( *pDC, edgeRegion, GetSysColorBrush( COLOR_BTNFACE ) );
}

void CColorTableRenderer::DrawSelectedCell( CDC* pDC ) const
{
	if ( !m_pColorTable->HasSelectedColor() )
		return;

	CRect cellRect = MakeCellRect( m_pColorTable->m_selPos );
	cellRect.InflateRect( 2, 2 );
	cellRect &= m_clientRect;

	::FrameRect( *pDC, &cellRect, GetSysColorBrush( COLOR_WINDOW ) );
	cellRect.DeflateRect( 1, 1 );
	::FrameRect( *pDC, &cellRect, GetSysColorBrush( COLOR_HIGHLIGHT ) );
	cellRect.DeflateRect( 1, 1 );
	::FrameRect( *pDC, &cellRect, GetSysColorBrush( COLOR_WINDOW ) );
}

bool CColorTableRenderer::CanShowLabels( CDC* pDC ) const
{
	if ( m_pColorTable->m_showLabels )
	{
		CSize textSize = ui::GetTextSize( pDC, num::FormatNumber( m_pColorTable->m_colors.size() - 1 ).c_str() );
		return textSize.cx <= m_cellSize.cx && textSize.cy <= m_cellSize.cy;
	}
	return false;
}


// CImageTranspColors implementation

const TCHAR CImageTranspColors::s_entry[] = _T("TranspColors");


void CImageTranspColors::Load( const TCHAR* pSection )
{
	std::vector<std::tstring> items;
	str::Split( items, (LPCTSTR)AfxGetApp()->GetProfileString( pSection, s_entry ), _T(";") );

	for ( std::vector<std::tstring>::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
	{
		size_t sepPos = itItem->find( _T('|') );
		if ( sepPos != std::tstring::npos )
		{
			fs::CPath imagePath( itItem->substr( 0, sepPos ) );
			COLORREF transpColor = CLR_NONE;
			if ( ui::ParseHtmlColor( &transpColor, &(*itItem)[ sepPos + 1 ] ) )
				if ( imagePath.FileExist() && transpColor != CLR_NONE )
					m_transpColorMap[ imagePath ] = transpColor;
		}
	}
}

void CImageTranspColors::Save( const TCHAR* pSection ) const
{
	std::vector<std::tstring> items; items.reserve( m_transpColorMap.size() );

	for ( std::map<fs::CPath, COLORREF>::const_iterator itImage = m_transpColorMap.begin(); itImage != m_transpColorMap.end(); ++itImage )
		if ( itImage->first.FileExist() && itImage->second != CLR_NONE )
			items.push_back( str::Format( _T("%s|%s"), itImage->first.GetPtr(), ui::FormatHtmlColor( itImage->second ).c_str() ) );

	AfxGetApp()->WriteProfileString( pSection, s_entry, str::Join( items, _T(";") ).c_str() );
}

COLORREF CImageTranspColors::Lookup( const fs::CPath& imagePath ) const
{
	std::map<fs::CPath, COLORREF>::const_iterator itFound = m_transpColorMap.find( imagePath );
	return itFound != m_transpColorMap.end() ? itFound->second : CLR_NONE;
}

bool CImageTranspColors::Register( const fs::CPath& imagePath, COLORREF transpColor )
{
	if ( imagePath.FileExist() && transpColor != CLR_NONE )
	{
		m_transpColorMap[ imagePath ] = transpColor;
		return true;
	}
	Unregister( imagePath );
	return false;
}


// CModeData implementation

CModeData::CModeData( const TCHAR* pLabels )
{
	str::Split( m_labels, pLabels, _T("|") );
	static const std::tstring space = _T(" ");
	for ( std::vector<std::tstring>::iterator itLabel = m_labels.begin(); itLabel != m_labels.end(); ++itLabel )
		*itLabel = space + *itLabel + space;			// pad with spaces for better looking labels

	m_dibs.resize( m_labels.size(), nullptr );			// fill with null placeholders
}

void CModeData::Clear( void )
{
	std::for_each( m_dibs.begin(), m_dibs.end(), func::Delete() );
	std::fill( m_dibs.begin(), m_dibs.end(), (CDibSection*)nullptr );
}


// CColorSample implementation

bool CColorSample::RenderSample( CDC* pDC, const CRect& clientRect )
{
	if ( m_color != CLR_NONE )
	{
		CBrush brush( m_color );
		pDC->FillRect( &clientRect, &brush );
	}
	else
	{
		::FillRect( *pDC, &clientRect, GetSysColorBrush( COLOR_BTNFACE ) );
		CScopedDrawText scopedDrawText( pDC, this, GetParent()->GetFont(), GetSysColor( COLOR_GRAYTEXT ) );
		pDC->DrawText( _T("None"), -1, const_cast<CRect*>( &clientRect ), DT_SINGLELINE | DT_NOPREFIX | DT_CENTER | DT_VCENTER );
	}

	return true;
}

void CColorSample::ShowPixelInfo( const CPoint& pos, COLORREF color )
{
	if ( m_pRoutePixelInfo != nullptr )
		m_pRoutePixelInfo->ShowPixelInfo( pos, color );
}
