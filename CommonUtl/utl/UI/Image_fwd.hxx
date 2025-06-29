#ifndef Image_fwd_hxx
#define Image_fwd_hxx
#pragma once

#include "Pixel.h"
#include "ScopedBitmapMemDC.h"


// CDibSectionTraits template methods

template< typename PixelFunc >
bool CDibSectionTraits::ForEachInColorTable( const bmp::CSharedAccess& dib, PixelFunc func )
{
	REQUIRE( IsIndexed() && m_hDib == dib.GetBitmapHandle() );

	CDC* pDC = dib.GetBitmapMemDC();
	GetColorTable( pDC );

	for ( std::vector<RGBQUAD>::iterator itRgb = m_colorTable.begin(); itRgb != m_colorTable.end(); ++itRgb )
	{
		CPixelBGR pixel( *itRgb );

		func( pixel );
		pixel.ToRGBQUAD( *itRgb );
	}

	// modify the color table of the DIB selected in pDC
	return ::SetDIBColorTable( *pDC, 0, (UINT)m_colorTable.size(), &m_colorTable.front() ) == m_colorTable.size();
}


#endif // Image_fwd_hxx
