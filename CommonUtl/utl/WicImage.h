#ifndef WicImage_h
#define WicImage_h
#pragma once

#include "ImagePathKey.h"
#include "WicBitmap.h"


// defines a WIC bitmap frame from a multi-frame image file or embedded file
//
class CWicImage : public CWicBitmap
{
public:
	CWicImage( const WICPixelFormatGUID* pCvtPixelFormat = &GUID_WICPixelFormat32bppPBGRA );
	virtual ~CWicImage();

	// image factory
	static std::auto_ptr< CWicImage > CreateFromFile( const fs::ImagePathKey& imageKey, bool throwMode = false );

	void Clear( void );

	const fs::ImagePathKey& GetKey( void ) const { return m_key; }

	bool LoadFromFile( const fs::ImagePathKey& imageKey );
	inline bool LoadFromFile( const fs::CFlexPath& imagePath, UINT framePos ) { return LoadFromFile( fs::ImagePathKey( imagePath, framePos ) ); }
	bool LoadFrame( UINT framePos );

	const fs::CFlexPath& GetImagePath( void ) const { return m_key.first; }
	UINT GetFramePos( void ) const { return m_key.second; }
	UINT GetFrameCount( void ) const { return m_frameCount; }

	bool IsValidFramePos( UINT framePos ) const { return framePos < m_frameCount; }

	bool IsValidFile( fs::AccessMode accessMode = fs::Exist ) const { return IsValid() && IsValidFramePos( m_key.second ) && GetImagePath().FileExist( accessMode ); }
	bool IsValidPhysicalFile( fs::AccessMode accessMode = fs::Exist ) const { return !GetImagePath().IsComplexPath() && IsValidFile( accessMode ); }

	static bool IsCorruptFile( const fs::CFlexPath& imagePath );
	static bool IsCorruptFrame( const fs::ImagePathKey& imageKey );

	std::tstring FormatDbg( void ) const;

	// overridables
	virtual bool IsAnimated( void ) const;
	virtual const CSize& GetBmpSize( void ) const;
protected:
	virtual bool StoreFrame( IWICBitmapSource* pFrameBitmap, UINT framePos );

	bool LoadDecoderFrame( wic::CBitmapDecoder& decoder, const fs::ImagePathKey& imageKey );
private:
	// hidden base methods
	using CWicBitmap::SetWicBitmap;
private:
	fs::ImagePathKey m_key;								// path and frame pos
	UINT m_frameCount;									// count of frames in a multiple image format
	const WICPixelFormatGUID* m_pCvtPixelFormat;		// format to which the loaded frame bitmap is converted to (NULL for auto)
public:
	static const fs::ImagePathKey s_nullKey;
};


#include "FilterStore.h"


namespace fs
{
	inline const CFlexPath& CastFlexPath( const fs::ImagePathKey& imageKey ) { return CastFlexPath( imageKey.first ); }


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
	bool BrowseImageFile( std::tstring& rFilePath, BrowseMode browseMode = FileOpen, DWORD flags = 0, CWnd* pParentWnd = NULL );
}


#endif // WicImage_h
