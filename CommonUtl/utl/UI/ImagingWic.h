#ifndef ImagingWic_h
#define ImagingWic_h
#pragma once

#include "FlexPath.h"
#include "Image_fwd.h"
#include "ThrowMode.h"
#include "ImagingWic_fwd.h"


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
		CComPtr< IWICImagingFactory > m_pFactory;
	};


	// wrapper for IWICBitmapDecoder: works with file streams and embedded file streams
	//
	class CBitmapDecoder : public CThrowMode
	{
	public:
		CBitmapDecoder( bool throwMode = false ) : CThrowMode( throwMode ) {}
		CBitmapDecoder( const fs::CFlexPath& imagePath, bool throwMode = false ) : CThrowMode( throwMode ), m_frameCount( 0 ) { CreateFromFile( imagePath ); }
		CBitmapDecoder( const CBitmapDecoder& right ) : CThrowMode( false ), m_frameCount( right.m_frameCount ), m_pDecoder( right.m_pDecoder ) {}

		bool IsValid( void ) const { return m_frameCount != 0; }
		UINT GetFrameCount( void ) const { return m_frameCount; }

		IWICBitmapDecoder* GetDecoder( void ) const { return m_pDecoder; }

		bool CreateFromFile( const fs::CFlexPath& imagePath );
		bool CreateFromStream( IStream* pStream, const IID* pDecoderId = NULL );

		bool HasContainerFormat( const GUID& containerFormatId ) const;
		CComPtr< IWICBitmapFrameDecode > GetFrameAt( UINT framePos ) const;
		CComPtr< IWICBitmapSource > ConvertFrameAt( UINT framePos, const WICPixelFormatGUID* pPixelFormat = &GUID_WICPixelFormat32bppPBGRA ) const;		// releases any source dependencies (IStream, HFILE, etc)

		// metadata
		CComPtr< IWICMetadataQueryReader > GetDecoderMetadata( void ) const;				// global
		CComPtr< IWICMetadataQueryReader > GetFrameMetadataAt( UINT framePos ) const;		// per frame

		enum DecoderFlag
		{
			MultiFrame	= BIT_FLAG( 0 ),
			Animation	= BIT_FLAG( 1 ),
			Lossless	= BIT_FLAG( 2 )
		};

		int GetDecoderFlags( void ) const;
	private:
		CComPtr< IWICBitmapDecoder > m_pDecoder;
		UINT m_frameCount;
	};


	struct CBitmapFormat
	{
		CBitmapFormat( IWICBitmapSource* pBitmap = NULL ) { Construct( pBitmap ); }

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
	class CBitmapOrigin : public CThrowMode
	{
	public:
		CBitmapOrigin( IWICBitmapSource* pSrcBitmap, bool throwMode = false );

		bool IsSourceTrueBitmap( void ) const;			// WICBitmap vs IWICBitmapFrameDecode, IWICBitmapScaler, etc

		// converts IWICBitmapSource to a IWICBitmap, releasing any bitmap source dependencies (IStream, HFILE, etc)
		bool DetachSourceToBitmap( WICBitmapCreateCacheOption detachOption = WICBitmapCacheOnLoad );

		void ReleaseSource( void ) { m_pSrcBitmap = NULL; }

		bool SaveBitmapToFile( const TCHAR* pDestFilePath, const GUID* pContainerFormatId = NULL );		// e.g. GUID_ContainerFormatJpeg
		bool SaveBitmapToStream( IStream* pDestStream, const GUID& containerFormatId );

		bool IsValid( void ) const { return m_pSrcBitmap != NULL && m_bmpFmt.IsValid(); }

		IWICBitmapSource* GetSourceBitmap( void ) const { return m_pSrcBitmap; }
		const CBitmapFormat& GetBmpFmt( void ) const { return m_bmpFmt; }

		const fs::CFlexPath& GetOriginalPath( void ) const { return m_originalPath; }
		void SetOriginalPath( const fs::CFlexPath& originalPath ) { m_originalPath = originalPath; }

		static const GUID& FindContainerFormatId( const TCHAR* pDestFilePath );
	private:
		bool SaveBitmapToWicStream( IWICStream* pDestWicStream, const GUID& containerFormatId );
		CComPtr< IWICBitmapFrameEncode > CreateDestFrameEncode( IWICBitmapEncoder* pEncoder );
		bool ConvertToFrameEncode( IWICBitmapFrameEncode* pDestFrameEncode ) const;

		CComPtr< IWICPalette > ClonePalette( void ) const;

		static const GUID& ResolveContainerFormatId( const GUID* pFormatId, const TCHAR* pDestFilePath );
	private:
		CComPtr< IWICBitmapSource > m_pSrcBitmap;		// could be source frame (scaled, etc) or a detached IWICBitmap copy
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

	CComPtr< IWICBitmapSource > LoadBitmapFromStream( CDibMeta& rDibMeta, IStream* pImageStream, const IID& decoderId );
	UINT GetFrameCount( IWICBitmapDecoder* pDecoder );
	CComPtr< IWICBitmapSource > ExtractFrameBitmap( CDibMeta& rDibMeta, IWICBitmapDecoder* pDecoder, UINT framePos = 0 );

	// create a 24/32 bpp DIB (top-down) from the specified WIC bitmap
	HBITMAP CreateDibSection( IWICBitmapSource* pBitmap );
	bool CreateDibSection( CDibMeta& rDibMeta, IWICBitmapSource* pBitmap );

	// WIC info
	CSize GetBitmapSize( IWICBitmapSource* pBitmap );

	CComPtr< IWICPixelFormatInfo > GetPixelFormatInfo( IWICBitmapSource* pBitmap );
	bool GetPixelFormat( UINT& rBitsPerPixel, UINT& rChannelCount, IWICBitmapSource* pBitmap );
	bool HasAlphaChannel( IWICBitmapSource* pBitmap );


	// transformations
	CComPtr< IWICBitmap > CreateBitmapFromSource( IWICBitmapSource* pBitmapSource, WICBitmapCreateCacheOption createOption = WICBitmapCacheOnDemand );		// true copy on-demand

	CComPtr< IWICBitmapScaler > ScaleBitmap( IWICBitmapSource* pWicBitmap, const CSize& newSize, WICBitmapInterpolationMode interpolationMode = WICBitmapInterpolationModeFant );
	CComPtr< IWICBitmapScaler > ScaleBitmapToBounds( IWICBitmapSource* pWicBitmap, const CSize& boundsSize, WICBitmapInterpolationMode interpolationMode = WICBitmapInterpolationModeFant );


	namespace cvt
	{
		inline CComPtr< IWICBitmapSource > ToWicBitmap( IWICBitmapSource* pWicBitmap ) { return pWicBitmap; }
		CComPtr< IWICBitmapSource > ToWicBitmap( HBITMAP hBitmap, WICBitmapAlphaChannelOption alphaChannel = WICBitmapUsePremultipliedAlpha );
	}


	// WIC bitmap persistence: load/save to file/stream

	const IID* FindDecoderIdFromPath( const TCHAR* pFilePath );
	const IID* FindDecoderIdFromResource( const TCHAR* pResType );

	CComPtr< IWICBitmapSource > LoadBitmapFromFile( const TCHAR* pFilePath, UINT framePos = 0 );
	CComPtr< IWICBitmapSource > LoadBitmapFromStream( IStream* pImageStream, const IID& decoderId );		// (*) will keep the stream alive for the lifetime of the bitmap; same if we scale the bitmap


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