
#include "pch.h"
#include "ImagingGdiPlus.h"
#include "Imaging.h"
#include "DibPixels.h"
#include "Path.h"
#include <afxtoolbarimages.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace gp
{
	CDibMeta LoadImageFromFile( const TCHAR* pFilePath )
	{
		CDibMeta dibMeta;
		CImage image;
		if ( HR_OK( image.Load( pFilePath ) ) )		// load the ATL image
		{
			dibMeta.m_hDib = image.Detach();
			dibMeta.StorePixelFormat();

			if ( dibMeta.m_bitsPerPixel >= 32 && path::MatchExt( pFilePath, _T(".png") ) )
			{
				CDibPixels dibPixels( dibMeta.m_hDib );
				dibPixels.PreMultiplyAlpha();
			}
		}
		return dibMeta;
	}

	// CAREFUL:
	//	It doesn't work in a regular DLL (such as IDETools.dll) due to GDI+ initialization and release threading issues.
	//	CPngImage uses a ATL::CImage object which manages GDI+ lifetime. GDI+ is not thread safe, and it should be initialized
	//	only from the main thread. The key calls are CImage::CInitGDIPlus::Init() and CImage::CInitGDIPlus::ReleaseGDIPlus().
	//	CPngImage destructor calls ReleaseGDIPlus() and enters a deadlock.

	CDibMeta LoadPng( const TCHAR* pResPngName, bool mapTo3DColors /*= false*/ )
	{
		CPngImage pngImage;
		if ( pngImage.Load( pResPngName, CScopedResInst::Get() ) )
		{
			CDibMeta dibMeta( (HBITMAP)pngImage.Detach() );
			if ( dibMeta.IsValid() )
			{
				if ( dibMeta.StorePixelFormat() )
				{
					if ( dibMeta.m_bitsPerPixel >= 32 )
					{
						CDibPixels dibPixels( dibMeta.m_hDib );
						dibPixels.PreMultiplyAlpha();
					}

					if ( mapTo3DColors )
						gdi::MapBmpTo3dColors( dibMeta.m_hDib );

					return dibMeta;
				}
			}
		}

		return CDibMeta();
	}

	CDibMeta LoadPngOrBitmap( const TCHAR* pResImageName, bool mapTo3DColors /*= false*/ )
	{
		CDibMeta dibMeta = LoadPng( pResImageName, mapTo3DColors );			// try to load PNG image first
		if ( dibMeta.IsValid() )
			return dibMeta;

		return gdi::LoadBitmapAsDib( pResImageName, mapTo3DColors );
	}

} //namespace gp
