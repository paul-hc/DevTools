
#include "pch.h"
#include "ImagingGdiPlus.h"
#include "Imaging.h"
#include "DibPixels.h"
#include "ResourceData.h"
#include "Path.h"
#include <afxtoolbarimages.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace gp
{
	namespace detail
	{
		void StoreImage( CDibMeta* pOutDibMeta, HBITMAP hSrcBitmap )
		{
			ASSERT_PTR( pOutDibMeta );

			pOutDibMeta->m_hDib = hSrcBitmap;
			pOutDibMeta->StorePixelFormat();

			if ( pOutDibMeta->m_bitsPerPixel >= 32 && pOutDibMeta->HasAlpha() )		// works for PNG, BMP, ICO
			{
				CDibPixels dibPixels( pOutDibMeta->m_hDib );

				dibPixels.PreMultiplyAlpha();
			}
		}
	}


	CDibMeta LoadImageFromFile( const TCHAR* pFilePath )
	{
		CDibMeta dibMeta;
		CImage image;

		if ( HR_OK( image.Load( pFilePath ) ) )			// load the ATL image
			detail::StoreImage( &dibMeta, image.Detach() );

		return dibMeta;
	}

	CDibMeta LoadImageResource( const TCHAR* pResBmpName, const TCHAR* pImageResType /*= RT_BITMAP*/ )
	{
		CResourceData imageResource( pResBmpName, pImageResType );
		CComPtr<IStream> pImageStream = imageResource.CreateStreamCopy();
		CDibMeta dibMeta;

		if ( pImageStream != nullptr )					// loaded the image data into stream?
		{
			CImage image;

			if ( HR_OK( image.Load( pImageStream ) ) )	// load the ATL image from resource
				detail::StoreImage( &dibMeta, image.Detach() );		// TODO: seems to fail... at least for monochrome bitmaps
		}

		return dibMeta;
	}


	// CAREFUL:
	//	It doesn't work in a regular DLL (such as IDETools.dll) due to GDI+ initialization and release threading issues.
	//	CPngImage uses a ATL::CImage object which manages GDI+ lifetime. GDI+ is not thread safe, and it should be initialized
	//	only from the main thread. The key calls are CImage::CInitGDIPlus::Init() and CImage::CInitGDIPlus::ReleaseGDIPlus().
	//	CPngImage destructor calls ReleaseGDIPlus() and enters a deadlock.

	CDibMeta LoadPngResource( const TCHAR* pResPngName, bool mapTo3DColors /*= false*/ )
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

	CDibMeta LoadPngOrBitmapResource( const TCHAR* pResImageName, bool mapTo3DColors /*= false*/ )
	{
		CDibMeta dibMeta = gp::LoadPngResource( pResImageName, mapTo3DColors );			// try to load PNG image first

		if ( dibMeta.IsValid() )
			return dibMeta;

		return gp::LoadImageResource( pResImageName, RT_BITMAP );
		//return gdi?::LoadBitmapAsDib( pResImageName, mapTo3DColors );
	}

} //namespace gp
