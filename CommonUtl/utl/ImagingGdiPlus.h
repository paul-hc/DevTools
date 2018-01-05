#ifndef ImagingGdiPlus_h
#define ImagingGdiPlus_h
#pragma once

#include "Image_fwd.h"


// GDI+ imaging through CImage class in ATL

namespace gp
{
	// CAREFUL:
	//	It doesn't work in a regular DLL (such as IDETools.dll) due to GDI+ initialization and release threading issues.
	//	CPngImage uses a ATL::CImage object which manages GDI+ lifetime. GDI+ is not thread safe, and it should be initialized
	//	only from the main thread. The key calls are CImage::CInitGDIPlus::Init() and CImage::CInitGDIPlus::ReleaseGDIPlus().
	//	CPngImage destructor calls ReleaseGDIPlus() and enters a deadlock.
	//

	CDibMeta LoadImageFromFile( const TCHAR* pFilePath );

	CDibMeta LoadPng( const TCHAR* pResPngName, bool mapTo3DColors = false );
	CDibMeta LoadPngOrBitmap( const TCHAR* pResImageName, bool mapTo3DColors = false );		// PNG or BMP images
}


#endif // ImagingGdiPlus_h
