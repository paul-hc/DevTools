#ifndef ImagingWic_h
#define ImagingWic_h
#pragma once

#include "FlexPath.h"
#include "Image_fwd.h"
#include "ErrorHandler.h"
#include "ImagingWic_fwd.h"


class CEnumTags;


// WIC: Windows Imaging Component (from DirectX)

namespace wic
{
	// singleton to create WIC COM objects
	//
	class CImagingFactory
	{
		CImagingFactory( void );
		~CImagingFactory();

		static CImagingFactory& Instance( void );
	public:
		static IWICImagingFactory* Factory( void ) { return &*Instance().m_pFactory; }
	private:
		CComPtr<IWICImagingFactory> m_pFactory;
	};


	typedef int TDecoderFlags;


	// wrapper for IWICBitmapDecoder: works with file streams and embedded file streams
	//
	class CBitmapDecoder : public CErrorHandler
	{
	public:
		CBitmapDecoder( utl::ErrorHandling handlingMode = utl::CheckMode );
		CBitmapDecoder( const CBitmapDecoder& right );
		CBitmapDecoder( const fs::CFlexPath& imagePath, utl::ErrorHandling handlingMode = utl::CheckMode );

		bool IsValid( void ) const { return m_frameCount != 0; }
		UINT GetFrameCount( void ) const { return m_frameCount; }

		IWICBitmapDecoder* GetDecoder( void ) const { return m_pDecoder; }

		bool CreateFromFile( const fs::CFlexPath& imagePath );
		bool CreateFromStream( IStream* pStream, const IID* pVendorId = nullptr );

		bool HasContainerFormat( const GUID& containerFormatId ) const;
		CComPtr<IWICBitmapFrameDecode> GetFrameAt( UINT framePos ) const;
		CComPtr<IWICBitmapSource> ConvertFrameAt( UINT framePos, const WICPixelFormatGUID* pPixelFormat = &GUID_WICPixelFormat32bppPBGRA ) const;		// releases any source dependencies (IStream, HFILE, etc)

        bool GetPixelFormatIds( std::vector<GUID*>& rPixelFormatIds ) const;

		// metadata
		CComPtr<IWICMetadataQueryReader> GetDecoderMetadata( void ) const;				// global
		CComPtr<IWICMetadataQueryReader> GetFrameMetadataAt( UINT framePos ) const;		// per frame

		enum DecoderFlag
		{
			MultiFrame	= BIT_FLAG( 0 ),
			Animation	= BIT_FLAG( 1 ),
			Lossless	= BIT_FLAG( 2 ),
			ChromaKey	= BIT_FLAG( 3 )
		};

		TDecoderFlags GetDecoderFlags( void ) const;
	private:
		CComPtr<IWICBitmapDecoder> m_pDecoder;
		UINT m_frameCount;
	};


	struct CBitmapFormat
	{
		CBitmapFormat( IWICBitmapSource* pBitmap = nullptr ) { Construct( pBitmap ); }

		bool Construct( IWICBitmapSource* pBitmap );

		bool IsValid( void ) const { return m_bitsPerPixel != 0; }
		bool IsIndexed( void ) const { return m_bitsPerPixel <= 8; }
		bool Equals( const CBitmapFormat& right ) const;

		UINT GetPaletteColorCount( void ) const { return IsIndexed() ? ( 1 << m_bitsPerPixel ) : 0; }

		std::tstring FormatDbg( const TCHAR sep[] ) const;
	public:
		CSize m_size;
		WICPixelFormatGUID m_pixelFormatId;
		UINT m_bitsPerPixel;
		UINT m_channelCount;
		bool m_hasAlphaChannel;
		WICPixelFormatNumericRepresentation m_numRepresentation;
	};


	// WIC bitmap with format and save support
	//
	class CBitmapOrigin : public CErrorHandler
	{
	public:
		CBitmapOrigin( IWICBitmapSource* pSrcBitmap, utl::ErrorHandling handlingMode = utl::CheckMode );

		bool IsSourceTrueBitmap( void ) const;			// WICBitmap vs IWICBitmapFrameDecode, IWICBitmapScaler, etc

		// converts IWICBitmapSource to a IWICBitmap, releasing any bitmap source dependencies (IStream, HFILE, etc)
		bool DetachSourceToBitmap( WICBitmapCreateCacheOption detachOption = WICBitmapCacheOnLoad );

		void ReleaseSource( void ) { m_pSrcBitmap = nullptr; }

		bool SaveBitmapToFile( const TCHAR* pDestFilePath, const GUID* pContainerFormatId = nullptr );		// e.g. GUID_ContainerFormatJpeg
		bool SaveBitmapToStream( IStream* pDestStream, const GUID& containerFormatId );

		bool IsValid( void ) const { return m_pSrcBitmap != nullptr && m_bmpFmt.IsValid(); }

		IWICBitmapSource* GetSourceBitmap( void ) const { return m_pSrcBitmap; }
		const CBitmapFormat& GetBmpFmt( void ) const { return m_bmpFmt; }

		const fs::CFlexPath& GetOriginalPath( void ) const { return m_originalPath; }
		void SetOriginalPath( const fs::CFlexPath& originalPath ) { m_originalPath = originalPath; }

		static const GUID& FindContainerFormatId( const TCHAR* pDestFilePath );
	private:
		bool SaveBitmapToWicStream( IWICStream* pDestWicStream, const GUID& containerFormatId );
		CComPtr<IWICBitmapFrameEncode> CreateDestFrameEncode( IWICBitmapEncoder* pEncoder );
		bool ConvertToFrameEncode( IWICBitmapFrameEncode* pDestFrameEncode ) const;

		CComPtr<IWICPalette> ClonePalette( void ) const;

		static const GUID& ResolveContainerFormatId( const GUID* pFormatId, const TCHAR* pDestFilePath );
	private:
		CComPtr<IWICBitmapSource> m_pSrcBitmap;		// could be source frame (scaled, etc) or a detached IWICBitmap copy
		CBitmapFormat m_bmpFmt;
		fs::CFlexPath m_originalPath;					// optional: file path that the bitmap was loaded from
	};
}


namespace wic
{
	// using WIC for PNG loading is the preferred method since it doesn't rely on GDI+ (with its thread safety release deadlock in DLLs)

	CDibMeta LoadImageFromFile( const TCHAR* pFilePath, UINT framePos = 0 );
	UINT QueryImageFrameCount( const TCHAR* pFilePath );

	CDibMeta LoadPng( const TCHAR* pResPngName, bool mapTo3DColors = false );
	CDibMeta LoadPngOrBitmap( const TCHAR* pResImageName, bool mapTo3DColors = false );		// PNG or BMP images
	CDibMeta LoadImageWithType( const TCHAR* pResImageName, const TCHAR* pResType );

	CComPtr<IWICBitmapSource> LoadBitmapFromStream( IStream* pImageStream, UINT framePos = 0 );				// using auto-decoder

	CComPtr<IWICBitmapSource> LoadBitmapFromStream( CDibMeta& rDibMeta, IStream* pImageStream, const IID* pDecoderId );
	UINT GetFrameCount( IWICBitmapDecoder* pDecoder );
	CComPtr<IWICBitmapSource> ExtractFrameBitmap( CDibMeta& rDibMeta, IWICBitmapDecoder* pDecoder, UINT framePos = 0 );

	// create a 24/32 bpp DIB (top-down) from the specified WIC bitmap
	HBITMAP CreateDibSection( IWICBitmapSource* pBitmap );
	bool CreateDibSection( CDibMeta& rDibMeta, IWICBitmapSource* pBitmap );

	// WIC info
	CSize GetBitmapSize( IWICBitmapSource* pBitmap );

	CComPtr<IWICPixelFormatInfo> GetPixelFormatInfo( IWICBitmapSource* pBitmap );
	bool GetPixelFormat( UINT& rBitsPerPixel, UINT& rChannelCount, IWICBitmapSource* pBitmap );
	bool HasAlphaChannel( IWICBitmapSource* pBitmap );


	// transformations
	CComPtr<IWICBitmap> CreateBitmapFromSource( IWICBitmapSource* pBitmapSource, WICBitmapCreateCacheOption createOption = WICBitmapCacheOnDemand );		// true copy on-demand

	CComPtr<IWICBitmapScaler> ScaleBitmap( IWICBitmapSource* pWicBitmap, const CSize& newSize, WICBitmapInterpolationMode interpolationMode = WICBitmapInterpolationModeFant );
	CComPtr<IWICBitmapScaler> ScaleBitmapToBounds( IWICBitmapSource* pWicBitmap, const CSize& boundsSize, WICBitmapInterpolationMode interpolationMode = WICBitmapInterpolationModeFant );


	namespace cvt
	{
		inline CComPtr<IWICBitmapSource> ToWicBitmap( IWICBitmapSource* pWicBitmap ) { return pWicBitmap; }
		CComPtr<IWICBitmapSource> ToWicBitmap( HBITMAP hBitmap, WICBitmapAlphaChannelOption alphaChannel = WICBitmapUsePremultipliedAlpha );
	}
}


namespace wic
{
	// WIC bitmap persistence: load/save to file/stream

	enum ImageFormat { BmpFormat, JpegFormat, TiffFormat, GifFormat, PngFormat, IcoFormat, WmpFormat, UnknownImageFormat };

	const CEnumTags& GetTags_ImageFormat( void );

	ImageFormat FindFileImageFormat( const TCHAR* pFilePath );
	ImageFormat FindResourceImageFormat( const TCHAR* pResType );

	inline bool IsValidFileImageFormat( const TCHAR* pFilePath ) { return FindFileImageFormat( pFilePath ) != UnknownImageFormat; }

	const TCHAR* GetDefaultImageFormatExt( ImageFormat imageType );
	bool ReplaceImagePathExt( fs::CPath& rSaveImagePath, ImageFormat imageType );

	const IID* GetDecoderId( ImageFormat imageType );
	const IID* GetEncoderId( ImageFormat imageType );
	const IID* GetContainerFormatId( ImageFormat imageType );

	inline const IID* FindDecoderIdFromPath( const TCHAR* pFilePath ) { return wic::GetDecoderId( wic::FindFileImageFormat( pFilePath ) ); }
	inline const IID* FindDecoderIdFromResource( const TCHAR* pResType ) { return wic::GetDecoderId( wic::FindResourceImageFormat( pResType ) ); }


	CComPtr<IWICBitmapSource> LoadBitmapFromFile( const TCHAR* pFilePath, UINT framePos = 0 );
	CComPtr<IWICBitmapSource> LoadBitmapFromStream( IStream* pImageStream, const IID* pDecoderId );		// (*) will keep the stream alive for the lifetime of the bitmap; same if we scale the bitmap


	// save bitmap: works with IWICBitmapSource*, HBITMAP, and other wic::cvt::ToWicBitmap defined conversions

	template< typename SrcBitmapType >
	bool SaveBitmapToFile( SrcBitmapType srcBitmap, const TCHAR* pDestFilePath )
	{
		CBitmapOrigin bitmapSource( cvt::ToWicBitmap( srcBitmap ) );
		return bitmapSource.SaveBitmapToFile( pDestFilePath );
	}

	template< typename SrcBitmapType >
	bool SaveBitmapToStream( SrcBitmapType srcBitmap, IStream* pDestStream, const GUID& containerFormatId )
	{
		CBitmapOrigin bitmapSource( cvt::ToWicBitmap( srcBitmap ) );
		return bitmapSource.SaveBitmapToStream( pDestStream, containerFormatId );
	}


#ifdef _DEBUG

	namespace dbg
	{
		// diagnostics
		bool InspectPixelFormatInfo( IWICBitmapSource* pBitmap );
	}

#endif

} //namespace wic


#endif // ImagingWic_h
