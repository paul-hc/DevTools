#ifndef WicImage_h
#define WicImage_h
#pragma once

#include "utl/PathMap.h"
#include "ImagePathKey.h"
#include "WicBitmap.h"


// defines a WIC bitmap frame from a multi-frame image file or embedded file
//
class CWicImage : public CWicBitmap
{
public:
	CWicImage( const WICPixelFormatGUID* pCvtPixelFormat = &GUID_WICPixelFormat32bppPBGRA );
	virtual ~CWicImage();

	void Clear( void );

	const fs::TImagePathKey& GetKey( void ) const { return m_key; }

	bool LoadFromFile( const fs::TImagePathKey& imageKey );
	inline bool LoadFromFile( const fs::CFlexPath& imagePath, UINT framePos ) { return LoadFromFile( fs::TImagePathKey( imagePath, framePos ) ); }
	bool LoadFrame( UINT framePos );

	const fs::CFlexPath& GetImagePath( void ) const { return m_key.first; }
	UINT GetFramePos( void ) const { return m_key.second; }
	UINT GetFrameCount( void ) const { return m_frameCount; }

	bool IsValidFramePos( UINT framePos ) const { return framePos < m_frameCount; }

	bool IsValidFile( fs::AccessMode accessMode = fs::Exist ) const { return IsValid() && IsValidFramePos( m_key.second ) && GetImagePath().FileExist( accessMode ); }
	bool IsValidPhysicalFile( fs::AccessMode accessMode = fs::Exist ) const { return !GetImagePath().IsComplexPath() && IsValidFile( accessMode ); }

	std::tstring FormatDbg( void ) const;

	bool IsMultiFrameStatic( void ) const { return m_frameCount > 1 && !IsAnimated(); }

	// overridables
	virtual bool IsAnimated( void ) const;
	virtual const CSize& GetBmpSize( void ) const;
protected:
	virtual bool StoreFrame( IWICBitmapSource* pFrameBitmap, UINT framePos );
private:
	// hidden base methods
	using CWicBitmap::SetWicBitmap;

public:
	// image factory methods
	static std::auto_ptr<CWicImage> CreateFromFile( const fs::TImagePathKey& imageKey, utl::ErrorHandling handlingMode = utl::CheckMode );
	static std::pair<UINT, wic::TDecoderFlags> LookupImageFileFrameCount( const fs::CFlexPath& imagePath );

	static bool IsCorruptFile( const fs::CFlexPath& imagePath );
	static bool IsCorruptFrame( const fs::TImagePathKey& imageKey );

	static wic::CBitmapDecoder AcquireDecoder( const fs::CFlexPath& imagePath, utl::ErrorHandling handlingMode = utl::CheckMode );
		// used in static methods that don't create a CWicImage object - wic::CBitmapDecoder is efficient to copy
private:
	// Shared bitmap decoder that keeps track of static (not animated) multi-frame images with same image path (typically from a .tif stream).
	// These are shared resources since for multi-frame images we may need subsequent loading of other frames. The CWicAnimatedImage class manages its own shared decoder.
	// This object destroys itself when none of its frames reference it.
	class CMultiFrameDecoder
	{
	public:
		CMultiFrameDecoder( void ) {}
		CMultiFrameDecoder( const wic::CBitmapDecoder& decoder );
		~CMultiFrameDecoder();

		const wic::CBitmapDecoder& GetDecoder( void ) const { return m_decoder; }
		bool AnyFramesLoaded( void ) const { return !m_loadedFrames.empty(); }

		std::auto_ptr<CWicImage> LoadFrame( const fs::TImagePathKey& imageKey );
		bool UnloadFrame( CWicImage* pFrameImage );
	private:
		size_t FindPosLoaded( UINT framePos ) const;
		bool IsLoaded( UINT framePos ) const { return FindPosLoaded( framePos ) != utl::npos; }
	private:
		wic::CBitmapDecoder m_decoder;					// share the same decoder for frame access (keep it alive to avoid sharing violations on embedded storage IStream-s)
		std::vector< CWicImage* > m_loadedFrames;
	};
	friend class CMultiFrameDecoder;

	typedef fs::CPathMap< fs::CFlexPath, CMultiFrameDecoder > TMultiFrameDecoderMap;

	static TMultiFrameDecoderMap& SharedMultiFrameDecoders( void );

	void SetSharedDecoder( CMultiFrameDecoder* pSharedDecoder );
	bool LoadDecoderFrame( wic::CBitmapDecoder& decoder, const fs::TImagePathKey& imageKey );
private:
	fs::TImagePathKey m_key;							// path and frame pos
	UINT m_frameCount;									// count of frames in a multiple image format
	const WICPixelFormatGUID* m_pCvtPixelFormat;		// format to which the loaded frame bitmap is converted to (NULL for auto)
	CMultiFrameDecoder* m_pSharedDecoder;				// only for multi-frame images, otherwise NULL
public:
	static const fs::TImagePathKey s_nullKey;
};


namespace ui
{
	struct CImageFileDetails
	{
		CImageFileDetails( void ) { Reset(); }

		void Reset( const CWicImage* pImage = NULL );

		bool IsValid( void ) const { return !m_filePath.IsEmpty(); }
		bool IsMultiFrameImage( void ) const { return !m_isAnimated && m_frameCount > 1; }
		bool HasNavigInfo( void ) const { return m_navigCount > 1; }
		double GetMegaPixels( void ) const;

		bool operator==( const CImageFileDetails& right ) const;
	public:
		fs::CFlexPath m_filePath;
		bool m_isAnimated;
		UINT m_fileSize;

		UINT m_framePos;
		UINT m_frameCount;
		CSize m_dimensions;

		UINT m_navigPos;
		UINT m_navigCount;
	};
}


#include "FilterStore.h"


namespace fs
{
	inline const CFlexPath& CastFlexPath( const fs::TImagePathKey& imageKey ) { return CastFlexPath( imageKey.first ); }


	class CImageFilterStore : public CFilterStore
	{
		CImageFilterStore( WICComponentType codec = WICDecoder );

		void Enumerate( WICComponentType codec );
	public:
		static CImageFilterStore& Instance( shell::BrowseMode browseMode );		// distinct filters for Open and Save
	public:
		static const std::tstring s_classTag;
	};
}


namespace shell
{
	bool BrowseImageFile( fs::CPath& rFilePath, BrowseMode browseMode = FileOpen, DWORD flags = 0, CWnd* pParentWnd = NULL );
}


#endif // WicImage_h
