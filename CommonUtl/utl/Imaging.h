#ifndef Imaging_h
#define Imaging_h
#pragma once

#include "Image_fwd.h"


namespace ui
{
	CDibMeta LoadImageFromFile( const TCHAR* pFilePath, ui::ImagingApi api = ui::WicApi, UINT framePos = 0 );
	CDibMeta LoadPng( UINT pngId, ui::ImagingApi api = ui::WicApi, bool mapTo3DColors = false );
}


namespace gdi
{
	extern const TCHAR g_imageFileFilter[];


	// load bitmap from resource or from file
	CDibMeta LoadBitmapAsDib( const TCHAR* pBmpName, bool mapTo3DColors = false );

	bool MapBmpTo3dColors( HBITMAP& rhBitmap, bool useRGBQUAD = false, COLORREF clrSrc = CLR_NONE, COLORREF clrDest = CLR_NONE );
}


#endif // Imaging_h
