#ifndef Image_fwd_hxx
#define Image_fwd_hxx
#pragma once

#include "Pixel.h"


// CDibSectionInfo template code

template< typename PixelFunc >
bool CDibSectionInfo::ForEachInColorTable( const bmp::CSharedAccess& dib, PixelFunc func )
{
	ASSERT( IsIndexed() && m_hDib == dib.GetHandle() );
	CDC* pDC = dib.GetBitmapMemDC();
	GetColorTable( pDC );

	for ( std::vector< RGBQUAD >::iterator itRgb = m_colorTable.begin(); itRgb != m_colorTable.end(); ++itRgb )
	{
		CPixelBGR pixel( *itRgb );
		func( pixel );
		itRgb->rgbBlue = pixel.m_blue;
		itRgb->rgbGreen = pixel.m_green;
		itRgb->rgbRed = pixel.m_red;
	}
	// modify the color table of the DIB selected in pDC
	return ::SetDIBColorTable( *pDC, 0, (UINT)m_colorTable.size(), &m_colorTable.front() ) == m_colorTable.size();
}


#endif // Image_fwd_hxx
