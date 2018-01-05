
#include "stdafx.h"
#include "ImagingWic.h"
#include "Imaging.h"
#include "ResourceData.h"
#include "StreamStdTypes.h"
#include "StructuredStorage.h"
#include "Utilities.h"
#include "BaseApp.h"

#pragma comment( lib, "windowscodecs.lib" )		// link to WIC

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace wic
{
	// CImagingFactory implementation

	CImagingFactory::CImagingFactory( void )
	{
		HR_OK( m_pFactory.CoCreateInstance( CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER ) );

		app::GetGlobalResources()->GetSharedResources().AddComPtr( m_pFactory );			// will release the factory singleton in ExitInstance()
	}

	CImagingFactory::~CImagingFactory()
	{
	}

	CImagingFactory& CImagingFactory::Instance( void )
	{
		static CImagingFactory factory;
		return factory;
	}


	// CBitmapDecoder implementation

	bool CBitmapDecoder::CreateFromFile( const fs::CFlexPath& flexPath )
	{
		if ( CComPtr< IStream > pStream = fs::OpenStreamOnFile( flexPath ) )
			return CreateFromStream( pStream );

		return false;
	}

	bool CBitmapDecoder::CreateFromStream( IStream* pStream, const IID* pDecoderId /*= NULL*/ )
	{
		ASSERT_PTR( pStream );
		m_pDecoder = NULL;
		m_frameCount = 0;

		return
			Good( CImagingFactory::Factory()->CreateDecoderFromStream( pStream, pDecoderId, WICDecodeMetadataCacheOnDemand, &m_pDecoder ) ) &&
			Good( m_pDecoder->GetFrameCount( &m_frameCount ) );
	}

	bool CBitmapDecoder::HasContainerFormat( const GUID& containerFormatId ) const
	{
		ASSERT_PTR( m_pDecoder );
		GUID format;
		return
			HR_OK( m_pDecoder->GetContainerFormat( &format ) ) &&
			IsEqualGUID( containerFormatId, format ) != FALSE;
	}

	CComPtr< IWICBitmapFrameDecode > CBitmapDecoder::GetFrameAt( UINT framePos ) const
	{
		ASSERT_PTR( m_pDecoder );
		CComPtr< IWICBitmapFrameDecode > pFrame;
		if ( Good( m_pDecoder->GetFrame( framePos, &pFrame ) ) )
			return pFrame;
		return NULL;
	}

	CComPtr< IWICBitmapSource > CBitmapDecoder::ConvertFrameAt( UINT framePos, const WICPixelFormatGUID* pPixelFormat /*= &GUID_WICPixelFormat32bppPBGRA*/ ) const
	{
		if ( CComPtr< IWICBitmapFrameDecode > pSrcBitmap = GetFrameAt( framePos ) )
		{
			if ( NULL == pPixelFormat )			// auto-format: find the best fitting destination format
			{
				CBitmapFormat srcFormat( pSrcBitmap );

				if ( srcFormat.m_hasAlphaChannel )
					pPixelFormat = &GUID_WICPixelFormat32bppPBGRA;		// 32bpp BGRA with pre-multiplied alpha (to preserve the alpha channel)
				else if ( 16 == srcFormat.m_bitsPerPixel )
					pPixelFormat = &GUID_WICPixelFormat16bppBGR555;		// preserve 16 bpp
				else
					pPixelFormat = &GUID_WICPixelFormat24bppBGR;		// 24bpp BGR
			}

			CComPtr< IWICBitmapSource > pDestBitmap;
			if ( Good( ::WICConvertBitmapSource( *pPixelFormat, pSrcBitmap, &pDestBitmap ) ) )
				return pDestBitmap;
		}
		return NULL;
	}

	CComPtr< IWICMetadataQueryReader > CBitmapDecoder::GetDecoderMetadata( void ) const
	{
		CComPtr< IWICMetadataQueryReader > pGlobalMetadata;
		if ( HR_OK( m_pDecoder->GetMetadataQueryReader( &pGlobalMetadata ) ) )
				return pGlobalMetadata;

		return NULL;
	}

	CComPtr< IWICMetadataQueryReader > CBitmapDecoder::GetFrameMetadataAt( UINT framePos ) const
	{
		CComPtr< IWICMetadataQueryReader > pFrameMetadata;
		if ( CComPtr< IWICBitmapFrameDecode > pFrame = GetFrameAt( framePos ) )
			if ( HR_OK( pFrame->GetMetadataQueryReader( &pFrameMetadata ) ) )
				return pFrameMetadata;

		return NULL;
	}

	int CBitmapDecoder::GetDecoderFlags( void ) const
	{
		int decoderFlags = 0;

		ASSERT_PTR( m_pDecoder );
		CComPtr< IWICBitmapDecoderInfo > pDecoderInfo;
		if ( HR_OK( m_pDecoder->GetDecoderInfo( &pDecoderInfo ) ) )
		{
			BOOL flag;
			if ( HR_OK( pDecoderInfo->DoesSupportMultiframe( &flag ) ) )
				SetFlag( decoderFlags, MultiFrame, flag != FALSE );

			if ( HR_OK( pDecoderInfo->DoesSupportAnimation( &flag ) ) )
				SetFlag( decoderFlags, Animation, flag != FALSE && m_frameCount > 1 );

			if ( HR_OK( pDecoderInfo->DoesSupportLossless( &flag ) ) )
				SetFlag( decoderFlags, Lossless, flag != FALSE );
		}
		return decoderFlags;
	}


	// CBitmapFormat implementation

	bool CBitmapFormat::Construct( IWICBitmapSource* pBitmap )
	{
		m_size.cx = m_size.cy = 0;
		m_pixelFormatId = GUID_NULL;
		m_bitsPerPixel = 0;
		m_channelCount = 0;
		m_hasAlphaChannel = false;
		m_numRepresentation = WICPixelFormatNumericRepresentationUnspecified;

		bool allGood = true;
		if ( pBitmap != NULL )
		{
			m_size = wic::GetBitmapSize( pBitmap );
			CComPtr< IWICPixelFormatInfo > pPixelFormatInfo = wic::GetPixelFormatInfo( pBitmap );
			ASSERT_PTR( pPixelFormatInfo );

			CNoThrow check;
			check.Good( pPixelFormatInfo->GetFormatGUID( &m_pixelFormatId ), &allGood );
			check.Good( pPixelFormatInfo->GetBitsPerPixel( &m_bitsPerPixel ), &allGood );
			check.Good( pPixelFormatInfo->GetChannelCount( &m_channelCount ), &allGood );

			if ( CComQIPtr< IWICPixelFormatInfo2 > pPixelFormatInfo2 = pPixelFormatInfo )
			{
				BOOL supportsTransparency = FALSE;
				if ( check.Good( pPixelFormatInfo2->SupportsTransparency( &supportsTransparency ) ) )
					m_hasAlphaChannel = supportsTransparency != FALSE;

				check.Good( pPixelFormatInfo2->GetNumericRepresentation( &m_numRepresentation ), &allGood );
			}
			else
				m_hasAlphaChannel = m_bitsPerPixel >= 32;
		}
		return allGood;
	}

	bool CBitmapFormat::Equals( const CBitmapFormat& right ) const
	{
		if ( this == &right )
			return true;
		return
			m_size == right.m_size &&
			m_pixelFormatId == right.m_pixelFormatId &&
			m_bitsPerPixel == right.m_bitsPerPixel &&
			m_channelCount == right.m_channelCount &&
			m_hasAlphaChannel == right.m_hasAlphaChannel &&
			m_numRepresentation == right.m_numRepresentation;
	}


#ifdef _DEBUG

	std::tstring CBitmapFormat::FormatDbg( const TCHAR sep[] ) const
	{
		std::tostringstream oss;

		if ( !IsValid() )
			oss << _T("NULL bitmap");
		else
		{
			using namespace stream;

			oss
				<< sep << m_size
				<< sep << m_bitsPerPixel << _T("bit");

			if ( m_hasAlphaChannel )
				oss << sep << _T("alpha");

			if ( IsIndexed() )
				oss << sep << _T("palette colors=") << GetPaletteColorCount();
		}
		return oss.str();
	}

#endif // _DEBUG



	// CBitmapOrigin implementation

	CBitmapOrigin::CBitmapOrigin( IWICBitmapSource* pBitmap, bool throwMode /*= false*/ )
		: CThrowMode( throwMode )
		, m_pSrcBitmap( pBitmap )
		, m_bmpFmt( m_pSrcBitmap )
	{
		ASSERT_PTR( m_pSrcBitmap );
	}

	bool CBitmapOrigin::IsSourceTrueBitmap( void ) const
	{
		CComQIPtr< IWICBitmap > pFullBitmap = m_pSrcBitmap;
		return pFullBitmap != NULL;
	}

	bool CBitmapOrigin::DetachSourceToBitmap( WICBitmapCreateCacheOption detachOption /*= WICBitmapCacheOnLoad*/ )
	{
		if ( IsSourceTrueBitmap() )
		{
			ASSERT( false );				// detach once
			return false;					// already a true bitmap, no detaching
		}
		if ( CComPtr< IWICBitmap > pDetachedBitmap = wic::CreateBitmapFromSource( m_pSrcBitmap, detachOption ) )
		{
			CBitmapFormat newFormat( pDetachedBitmap );
			ASSERT( m_bmpFmt.Equals( newFormat ) );			// format should be invariant
			m_bmpFmt = newFormat;
			return true;
		}
		TRACE( _T(" * DetachSourceToBitmap() failed!\n") );
		return false;
	}

	bool CBitmapOrigin::SaveBitmapToFile( const TCHAR* pDestFilePath, const GUID* pContainerFormatId /*= NULL*/ )
	{
		CComPtr< IWICStream > pDestWicStream;
		return
			Good( CImagingFactory::Factory()->CreateStream( &pDestWicStream ) ) &&
			Good( pDestWicStream->InitializeFromFilename( pDestFilePath, GENERIC_WRITE ) ) &&
			SaveBitmapToWicStream( pDestWicStream, ResolveContainerFormatId( pContainerFormatId, pDestFilePath ) );
	}

	bool CBitmapOrigin::SaveBitmapToStream( IStream* pDestStream, const GUID& containerFormatId )
	{
		CComPtr< IWICStream > pDestWicStream;
		return
			Good( CImagingFactory::Factory()->CreateStream( &pDestWicStream ) ) &&
			Good( pDestWicStream->InitializeFromIStream( pDestStream ) ) &&
			Good( SaveBitmapToWicStream( pDestWicStream, containerFormatId ) );
	}

	bool CBitmapOrigin::SaveBitmapToWicStream( IWICStream* pDestWicStream, const GUID& containerFormatId )
	{
		ASSERT_PTR( pDestWicStream );

		CComPtr< IWICBitmapEncoder > pEncoder;
		if ( !Good( CImagingFactory::Factory()->CreateEncoder( containerFormatId, NULL, &pEncoder ) ) )			// create the encoder for the corresponding format
			return false;

		if ( !Good( pEncoder->Initialize( pDestWicStream, WICBitmapEncoderNoCache ) ) )
			return false;

		CComPtr< IWICBitmapFrameEncode > pDestFrameEncode = CreateDestFrameEncode( pEncoder );

		if ( !Good( ConvertToFrameEncode( pDestFrameEncode ) ) )
			return false;

		// save the file
		if ( !Good( pDestFrameEncode->Commit() ) )
			return false;
		if ( !Good( pEncoder->Commit() ) )
			return false;

		return true;
	}

	const GUID& CBitmapOrigin::FindContainerFormatId( const TCHAR* pDestFilePath )
	{
		const TCHAR* pExt = path::FindExt( pDestFilePath );

		if ( path::EquivalentPtr( _T(".jpg"), pExt ) ||
			 path::EquivalentPtr( _T(".jpeg"), pExt ) ||
			 path::EquivalentPtr( _T(".jpe"), pExt ) ||
			 path::EquivalentPtr( _T(".jfif"), pExt ) )
			return GUID_ContainerFormatJpeg;
		else if ( path::EquivalentPtr( _T(".tif"), pExt ) ||
				  path::EquivalentPtr( _T(".tiff"), pExt ) )
			return GUID_ContainerFormatTiff;
		else if ( path::EquivalentPtr( _T(".gif"), pExt ) )
			return GUID_ContainerFormatGif;
		else if ( path::EquivalentPtr( _T(".png"), pExt ) )
			return GUID_ContainerFormatPng;
		else if ( path::EquivalentPtr( _T(".wmp"), pExt ) )
			return GUID_ContainerFormatWmp;
		else if ( path::EquivalentPtr( _T(".ico"), pExt ) ||
				  path::EquivalentPtr( _T(".cur"), pExt ) )
			return GUID_ContainerFormatIco;

		return GUID_ContainerFormatBmp;			// default is bitmap encoding
	}

	const GUID& CBitmapOrigin::ResolveContainerFormatId( const GUID* pFormatId, const TCHAR* pDestFilePath )
	{
		return pFormatId != NULL ? *pFormatId : FindContainerFormatId( pDestFilePath );
	}

	CComPtr< IWICBitmapFrameEncode > CBitmapOrigin::CreateDestFrameEncode( IWICBitmapEncoder* pEncoder )
	{
		CComPtr< IWICBitmapFrameEncode > pDestFrameEncode;
		CComPtr< IPropertyBag2 > pPropertyBag;
		if ( Good( pEncoder->CreateNewFrame( &pDestFrameEncode, &pPropertyBag ) ) )
			if ( Good( pDestFrameEncode->Initialize( pPropertyBag ) ) )
				if ( Good( pDestFrameEncode->SetPixelFormat( &m_bmpFmt.m_pixelFormatId ) ) )
				{
					double dpiX, dpiY;
					if ( HR_OK( m_pSrcBitmap->GetResolution( &dpiX, &dpiY ) ) )
						pDestFrameEncode->SetResolution( dpiX, dpiY );

					return pDestFrameEncode;
				}

		return NULL;
	}

	bool CBitmapOrigin::ConvertToFrameEncode( IWICBitmapFrameEncode* pDestFrameEncode ) const
	{
		ASSERT_PTR( pDestFrameEncode );

		if ( Good( pDestFrameEncode->SetSize( m_bmpFmt.m_size.cx, m_bmpFmt.m_size.cy ) ) )
		{
			CComPtr< IWICPalette > pPalette = ClonePalette();
			if ( pPalette != NULL )								// indexed?
				pDestFrameEncode->SetPalette( pPalette );

			WICRect rect = { 0, 0, m_bmpFmt.m_size.cx, m_bmpFmt.m_size.cy };
			return Good( pDestFrameEncode->WriteSource( m_pSrcBitmap, &rect ) );
		}

		return false;
	}

	CComPtr< IWICPalette > CBitmapOrigin::ClonePalette( void ) const
	{
		CComPtr< IWICPalette > pPalette;
		if ( UINT paletteCount = m_bmpFmt.GetPaletteColorCount() )
			if ( Good( CImagingFactory::Factory()->CreatePalette( &pPalette ) ) )
				if ( Good( m_pSrcBitmap->CopyPalette( pPalette ) ) )		// may fail if not indexed
					if ( Good( pPalette->InitializeFromBitmap( m_pSrcBitmap, paletteCount, FALSE ) ) )
						return pPalette;

		return NULL;
	}

} //namespace wic


namespace wic
{
	// WIC utilities

	CDibMeta LoadImageFromFile( const TCHAR* pFilePath, UINT framePos /*= 0*/ )
	{
		CDibMeta dibMeta;
		if ( 0 == framePos && path::MatchExt( pFilePath, _T(".bmp") ) )
		{
			dibMeta = gdi::LoadBitmapAsDib( pFilePath );			// for bitmap files prefer LoadBitmapAsDib() as it preserves the original DIB bpp
			if ( dibMeta.IsValid() )								// it fails for 32 bit bitmaps, though
				return dibMeta;
		}

		CComPtr< IWICBitmapDecoder > pDecoder;					// create a decoder for the given image file
		if ( HR_OK( CImagingFactory::Factory()->CreateDecoderFromFilename( pFilePath, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder ) ) )
		{
			// load the desired frame as WIC bitmap
			CComPtr< IWICBitmapSource > pBitmap = ExtractFrameBitmap( dibMeta, pDecoder, framePos );
			if ( pBitmap != NULL )
				CreateDibSection( dibMeta, pBitmap );			// create a top-down DIB section containing the image
		}

		return dibMeta;
	}

	UINT QueryImageFrameCount( const TCHAR* pFilePath )
	{
		UINT frameCount = 0;
		CComPtr< IWICBitmapDecoder > pDecoder;		// create a decoder for the given image file
		if ( HR_OK( CImagingFactory::Factory()->CreateDecoderFromFilename( pFilePath, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder ) ) )
			frameCount = GetFrameCount( pDecoder );
		return frameCount;
	}


	CDibMeta LoadPng( const TCHAR* pResPngName, bool mapTo3DColors /*= false*/ )
	{
		CDibMeta dibMeta = LoadImageWithType( pResPngName, _T("PNG") );
		if ( dibMeta.IsValid() )
			if ( mapTo3DColors )
				gdi::MapBmpTo3dColors( dibMeta.m_hDib );

		return dibMeta;
	}


	CDibMeta LoadPngOrBitmap( const TCHAR* pResImageName, bool mapTo3DColors /*= false*/ )
	{
		CDibMeta dibMeta = LoadPng( pResImageName, mapTo3DColors );		// try to load PNG image first
		if ( !dibMeta.IsValid() )
			dibMeta = gdi::LoadBitmapAsDib( pResImageName, mapTo3DColors );
		return dibMeta;
	}

	CDibMeta LoadImageWithType( const TCHAR* pResImageName, const TCHAR* pResType )
	{
		CDibMeta dibMeta;

		if ( const IID* pDecoderClsId = FindDecoderIdFromResource( pResType ) )
		{
			CResourceData imageResource( pResImageName, pResType );
			CComPtr< IStream > pImageStream = imageResource.CreateStreamCopy();

			if ( pImageStream != NULL )											// loaded the image data into stream?
			{
				// load the WIC bitmap
				CComPtr< IWICBitmapSource > pBitmap = LoadBitmapFromStream( dibMeta, pImageStream, *pDecoderClsId );
				if ( pBitmap != NULL )
					CreateDibSection( dibMeta, pBitmap );				// create a DIB section containing the image
			}
		}
		else
			ASSERT( false );		// unknown image type

		return dibMeta;
	}

	const IID* FindDecoderIdFromPath( const TCHAR* pFilePath )
	{
		const TCHAR* pExt = path::FindExt( pFilePath );

		if ( path::EquivalentPtr( _T(".bmp"), pExt ) ||
			 path::EquivalentPtr( _T(".dib"), pExt ) )
			return &CLSID_WICBmpDecoder;
		else if ( path::EquivalentPtr( _T(".ico"), pExt ) ||
				  path::EquivalentPtr( _T(".cur"), pExt ) )
			return &CLSID_WICIcoDecoder;
		else if ( path::EquivalentPtr( _T(".jpg"), pExt ) ||
			 path::EquivalentPtr( _T(".jpeg"), pExt ) ||
			 path::EquivalentPtr( _T(".jpe"), pExt ) ||
			 path::EquivalentPtr( _T(".jfif"), pExt ) )
			return &CLSID_WICJpegDecoder;
		else if ( path::EquivalentPtr( _T(".tif"), pExt ) ||
				  path::EquivalentPtr( _T(".tiff"), pExt ) )
			return &CLSID_WICTiffDecoder;
		else if ( path::EquivalentPtr( _T(".gif"), pExt ) )
			return &CLSID_WICGifDecoder;
		else if ( path::EquivalentPtr( _T(".png"), pExt ) )
			return &CLSID_WICPngDecoder;
		else if ( path::EquivalentPtr( _T(".wmp"), pExt ) )
			return &CLSID_WICWmpDecoder;

		return &GUID_ContainerFormatBmp;			// default is bitmap encoding
	}

	const IID* FindDecoderIdFromResource( const TCHAR* pResType )
	{
		if ( RT_CURSOR == pResType || RT_ICON == pResType )
			return &CLSID_WICIcoDecoder;
		else if ( RT_BITMAP == pResType )
			return &CLSID_WICBmpDecoder;
		else if ( !IS_INTRESOURCE( pResType ) )
			if ( str::Equals< str::IgnoreCase >( pResType, _T("PNG") ) )
				return &CLSID_WICPngDecoder;
			else if ( str::Equals< str::IgnoreCase >( pResType, _T("GIF") ) )
				return &CLSID_WICGifDecoder;
			else if ( str::Equals< str::IgnoreCase >( pResType, _T("TIFF") ) )
				return &CLSID_WICTiffDecoder;
			else if ( str::Equals< str::IgnoreCase >( pResType, _T("JPG") ) )
				return &CLSID_WICJpegDecoder;
			else if ( str::Equals< str::IgnoreCase >( pResType, _T("WMP") ) )
				return &CLSID_WICWmpDecoder;

		return NULL;
	}

	// load a PNG image from the specified stream (using Windows Imaging Component)
	CComPtr< IWICBitmapSource > LoadBitmapFromStream( CDibMeta& rDibMeta, IStream* pImageStream, const IID& decoderId )
	{
		ASSERT_PTR( pImageStream );

		CComPtr< IWICBitmapSource > pBitmap;

		CComPtr< IWICBitmapDecoder > pDecoder;
		if ( HR_OK( pDecoder.CoCreateInstance( decoderId, NULL, CLSCTX_INPROC_SERVER ) ) )			// create WIC's image decoder (PNG, JPG, etc)
			if ( HR_OK( pDecoder->Initialize( pImageStream, WICDecodeMetadataCacheOnLoad ) ) )		// load the image
				pBitmap = ExtractFrameBitmap( rDibMeta, pDecoder, 0 );

		return pBitmap;
	}

	UINT GetFrameCount( IWICBitmapDecoder* pDecoder )
	{
		ASSERT_PTR( pDecoder );
		UINT frameCount = 0;
		if ( HR_OK( pDecoder->GetFrameCount( &frameCount ) ) )
			return frameCount;
		return 0;
	}

	// load a bitmap frame from the decoder
	CComPtr< IWICBitmapSource > ExtractFrameBitmap( CDibMeta& rDibMeta, IWICBitmapDecoder* pDecoder, UINT framePos /*= 0*/ )
	{
		ASSERT_PTR( pDecoder );

		CComPtr< IWICBitmapSource > pBitmap;

		// check for the presence of the first frame in the bitmap
		UINT frameCount = 0;
		if ( HR_OK( pDecoder->GetFrameCount( &frameCount ) ) && frameCount != 0 )
		{
			framePos = std::min( frameCount - 1, framePos );		// limit frame pos to an existing one

			// load the first frame (i.e. the image)
			CComPtr< IWICBitmapFrameDecode > pFrame;
			if ( HR_OK( pDecoder->GetFrame( framePos, &pFrame ) ) )
			{
				// store SOURCE pixel format because the returned DIB is 32 bit regardless of having an alpha channel
				GetPixelFormat( rDibMeta.m_bitsPerPixel, rDibMeta.m_channelCount, pFrame );
				if ( rDibMeta.HasAlpha() )
					// convert the image to 32bpp BGRA format with pre-multiplied alpha
					//   (it may not be stored in that format natively in the PNG resource,
					//   but we need this format to create the DIB to use on-screen)
					WICConvertBitmapSource( GUID_WICPixelFormat32bppPBGRA, pFrame, &pBitmap );
				else if ( 16 == rDibMeta.m_bitsPerPixel )
					WICConvertBitmapSource( GUID_WICPixelFormat16bppBGR555, pFrame, &pBitmap );		// preserve 16 bpp
				else
					WICConvertBitmapSource( GUID_WICPixelFormat24bppBGR, pFrame, &pBitmap );		// convert to 24 bpp source of 4/8/24 bpp
			}
		}

		return pBitmap;
	}


	// WIC types

	CComPtr< IWICBitmapSource > LoadBitmapFromFile( const TCHAR* pFilePath, UINT framePos /*= 0*/ )
	{
		CComPtr< IWICBitmapDecoder > pDecoder;					// create a decoder for the given image file
		if ( HR_OK( CImagingFactory::Factory()->CreateDecoderFromFilename( pFilePath, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder ) ) )
		{
			CDibMeta dibMeta;
			return ExtractFrameBitmap( dibMeta, pDecoder, framePos );		// load the desired frame as WIC bitmap
		}
		return NULL;
	}

	CComPtr< IWICBitmapSource > LoadBitmapFromStream( IStream* pImageStream, const IID& decoderId )
	{
		CDibMeta dibMeta;
		return LoadBitmapFromStream( dibMeta, pImageStream, decoderId );
	}


	CSize GetBitmapSize( IWICBitmapSource* pBitmap )
	{
		UINT width = 0, height = 0;
		if ( pBitmap != NULL )
			pBitmap->GetSize( &width, &height );
		return CSize( width, height );
	}

	CComPtr< IWICPixelFormatInfo > GetPixelFormatInfo( IWICBitmapSource* pBitmap )
	{
		ASSERT_PTR( pBitmap );

		WICPixelFormatGUID pixelFormatClsId;
		if ( HR_OK( pBitmap->GetPixelFormat( &pixelFormatClsId ) ) )
		{
			CComPtr< IWICComponentInfo > pBitmapInfo;
			if ( HR_OK( CImagingFactory::Factory()->CreateComponentInfo( pixelFormatClsId, &pBitmapInfo ) ) )
			{
				CComQIPtr< IWICPixelFormatInfo > pPixelFormatInfo( pBitmapInfo );
				return pPixelFormatInfo;
			}
		}

		return NULL;
	}

	bool GetPixelFormat( UINT& rBitsPerPixel, UINT& rChannelCount, IWICBitmapSource* pBitmap )
	{
		CComPtr< IWICPixelFormatInfo > pPixelFormatInfo = GetPixelFormatInfo( pBitmap );
		return
			pPixelFormatInfo != NULL &&
			HR_OK( pPixelFormatInfo->GetBitsPerPixel( &rBitsPerPixel ) ) &&
			HR_OK( pPixelFormatInfo->GetChannelCount( &rChannelCount ) );
	}

	bool HasAlphaChannel( IWICBitmapSource* pBitmap )
	{
		ASSERT_PTR( pBitmap );
		UINT bitsPerPixel, channelCount;
		if ( GetPixelFormat( bitsPerPixel, channelCount, pBitmap ) )
			return 32 == bitsPerPixel && 4 == channelCount;
		return false;
	}


	HBITMAP CreateDibSection( IWICBitmapSource* pBitmap )
	{
		ASSERT_PTR( pBitmap );
		HBITMAP hBitmap = NULL;

		UINT bitsPerPixel, channelCount;
		if ( !GetPixelFormat( bitsPerPixel, channelCount, pBitmap ) )
			bitsPerPixel = 32;			// assume source pBitmap which is 32bpp PBGRA

		// get image attributes and check for valid image
		CSize bmpSize = GetBitmapSize( pBitmap );

		if ( bmpSize.cx != 0 && bmpSize.cy != 0 )
		{
			BITMAPINFO dibInfo;
			ZeroMemory( &dibInfo, sizeof( dibInfo ) );

			dibInfo.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
			dibInfo.bmiHeader.biWidth = bmpSize.cx;
			dibInfo.bmiHeader.biHeight = -bmpSize.cy;				// negative height indicates a top-down DIB
			dibInfo.bmiHeader.biPlanes = 1;
			dibInfo.bmiHeader.biBitCount = (WORD)bitsPerPixel;		// must match the source pBitmap which is 32bpp PBGRA (regardless of the source)
			dibInfo.bmiHeader.biCompression = BI_RGB;

			// create a DIB section that can hold the image
			HDC hScreenDC = ::GetDC( NULL );

			BYTE* pPixels = NULL;
			hBitmap = ::CreateDIBSection( hScreenDC, &dibInfo, DIB_RGB_COLORS, (void**)&pPixels, NULL, 0 );

			::ReleaseDC( NULL, hScreenDC );

			if ( hBitmap != NULL )
			{
				// extract the image into the HBITMAP
				UINT stride = gdi::GetDibStride( bmpSize.cx, bitsPerPixel );			// number of bytes per scanline
				UINT bufferSize = gdi::GetDibBufferSize( bmpSize.cy, stride );			// total size of the image

				if ( !HR_OK( pBitmap->CopyPixels( NULL, stride, bufferSize, pPixels ) ) )
				{
					::DeleteObject( hBitmap );		// couldn't extract image, delete HBITMAP
					hBitmap = NULL;
				}
			}
		}
		return hBitmap;
	}

	bool CreateDibSection( CDibMeta& rDibMeta, IWICBitmapSource* pBitmap )
	{
		rDibMeta.m_hDib = CreateDibSection( pBitmap );
		if ( NULL == rDibMeta.m_hDib )
			return false;
		rDibMeta.m_orientation = gdi::TopDown;
		return true;
	}


	// transformations

	CComPtr< IWICBitmap > CreateBitmapFromSource( IWICBitmapSource* pBitmapSource, WICBitmapCreateCacheOption createOption /*= WICBitmapCacheOnDemand*/ )
	{
		// after convertion it releases any dependencies to pBitmapSource (IStream, HFILE, etc)
		CComPtr< IWICBitmap > pBitmapCopy;
		HR_OK( CImagingFactory::Factory()->CreateBitmapFromSource( pBitmapSource, createOption, &pBitmapCopy ) );
		return pBitmapCopy;
	}

	CComPtr< IWICBitmapScaler > ScaleBitmap( IWICBitmapSource* pWicBitmap, const CSize& newSize, WICBitmapInterpolationMode interpolationMode /*= WICBitmapInterpolationModeFant*/ )
	{
		ASSERT_PTR( pWicBitmap );

		CComPtr< IWICBitmapScaler > pScaledBitmap;
		if ( HR_OK( CImagingFactory::Factory()->CreateBitmapScaler( &pScaledBitmap ) ) )
			if ( HR_OK( pScaledBitmap->Initialize( pWicBitmap, newSize.cx, newSize.cy, interpolationMode ) ) )
				return &*pScaledBitmap;			// up-cast

		return NULL;
	}

	CComPtr< IWICBitmapScaler > ScaleBitmapToBounds( IWICBitmapSource* pWicBitmap, const CSize& boundsSize,
													 WICBitmapInterpolationMode interpolationMode /*= WICBitmapInterpolationModeFant*/ )
	{
		// WICBitmapInterpolationModeFant: smoother, less sharp edges; good for shrinking
		// WICBitmapInterpolationModeHighQualityCubic: ?
		//
		return ScaleBitmap( pWicBitmap, ui::StretchToFit( boundsSize, GetBitmapSize( pWicBitmap ) ), interpolationMode  );
	}



	namespace cvt
	{
		// bitmap type conversion

		CComPtr< IWICBitmapSource > ToWicBitmap( HBITMAP hBitmap, WICBitmapAlphaChannelOption alphaChannel /*= WICBitmapUsePremultipliedAlpha*/ )
		{
			ASSERT_PTR( hBitmap );
			CComPtr< IWICBitmap > pWicBitmap;
			if ( HR_OK( CImagingFactory::Factory()->CreateBitmapFromHBITMAP( hBitmap, NULL, alphaChannel, &pWicBitmap ) ) )
				return &*pWicBitmap;				// up-cast
			return NULL;
		}

	} //namespace cvt


#ifdef _DEBUG

	namespace dbg
	{
		// diagnostics

		bool InspectPixelFormatInfo( IWICBitmapSource* pBitmap )
		{
			CComPtr< IWICPixelFormatInfo > pPixelFormatInfo = GetPixelFormatInfo( pBitmap );
			if ( NULL == pPixelFormatInfo )
				return false;

			// few precanned formats to compare in debugging
			GUID_WICPixelFormat32bppBGRA, GUID_WICPixelFormat32bppPBGRA, GUID_WICPixelFormat32bppRGBA, GUID_WICPixelFormat32bppPRGBA;
			GUID_WICPixelFormat24bppBGR, GUID_WICPixelFormat24bppRGB;

			WICPixelFormatGUID pixelFormatId;
			pPixelFormatInfo->GetFormatGUID( &pixelFormatId );

			UINT bitsPerPixel;
			pPixelFormatInfo->GetBitsPerPixel( &bitsPerPixel );

			UINT channelCount;
			pPixelFormatInfo->GetChannelCount( &channelCount );
			return true;
		}

	} //namespace dbg

#endif

} //namespace wic
