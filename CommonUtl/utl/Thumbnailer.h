#ifndef Thumbnailer_h
#define Thumbnailer_h
#pragma once

#include "FileObjectCache.h"
#include "Image_fwd.h"
#include "Thumbnailer_fwd.h"
#include "WicDibSection.h"
#include "WinExplorer.h"


class CEnumTags;
class CFlagTags;
namespace fs { enum FileExpireStatus; }


typedef std::pair< fs::CFlexPath, CComPtr< IShellItem > > ShellItemPair;		// source image path with its shell item


namespace thumb
{
	enum { DefaultBoundsSize = 96, MinBoundsSize = 16, MaxBoundsSize = 1024 };
	const CEnumTags& GetTags_StdBoundsSize( void );
}


// access to system local cached thumbs using IThumbnailCache

class CShellThumbCache
{
public:
	CShellThumbCache( void );

	const CSize& GetBoundsSize( void ) const { return m_boundsSize; }
	bool SetBoundsSize( const CSize& boundsSize );

	void SetThumbExtractFlags( SIIGBF thumbExtractFlags ) { m_thumbExtractFlags = thumbExtractFlags; }
	void SetOptimizeExtractIcons( bool optimizeExtractIcons = true ) { SetThumbExtractFlags( optimizeExtractIcons ? SIIGBF_ICONONLY : SIIGBF_BIGGERSIZEOK ); }

	// called to chain thumbnail production externally
	bool IsExternallyProduced( const fs::CFlexPath& filePath ) const { return m_pThumbProducer != NULL && m_pThumbProducer->ProducesThumbFor( filePath ); }
	void SetExternalProducer( fs::IThumbProducer* pThumbProducer ) { m_pThumbProducer = pThumbProducer; }

	bool IsValidShellCache( void ) const { return m_pShellThumbCache != NULL; }
	void ReleaseShellCache( void );

	CComPtr< IShellItem > FindShellItem( const fs::CFlexPath& filePath ) const;

	CCachedThumbBitmap* ExtractThumb( const ShellItemPair& imagePair );
	CCachedThumbBitmap* GenerateThumb( const ShellItemPair& imagePair );

	fs::FileExpireStatus CheckThumbExpired( const CCachedThumbBitmap* pThumb ) const;

	// create new thumbnail scale to m_boundsSize
	CCachedThumbBitmap* NewScaledThumb( IWICBitmapSource* pUnscaledBitmap, const fs::CFlexPath& srcImagePath, const CThumbKey* pCachedKey = NULL ) const;
	CComPtr< IWICBitmapSource > ScaleToThumbBitmap( IWICBitmapSource* pSrcBitmap ) const;
private:
	CSize m_boundsSize;
	CComPtr< IThumbnailCache > m_pShellThumbCache;		// provides access to Windows system-wide thumbnail cache
	fs::IThumbProducer* m_pThumbProducer;				// chains to external thumb producer
	shell::CWinExplorer m_shellExplorer;
	SIIGBF m_thumbExtractFlags;							// optimize for thumbnails or icons extraction (default for thumbnails)
public:
	static const CSize s_defaultBoundsSize;
};


class CThumbnailer : public CShellThumbCache, public ui::ICustomImageDraw, private utl::noncopyable
{
public:
	CThumbnailer( void );
	~CThumbnailer() { Clear(); }

	size_t GetCachedCount( void ) const;

	void Clear( void );
	bool SetBoundsSize( const CSize& boundsSize );

	CCachedThumbBitmap* AcquireThumbnail( const fs::CFlexPath& srcImagePath, int* pCacheStatusFlags = NULL );
	CCachedThumbBitmap* AcquireThumbnailNoThrow( const fs::CFlexPath& srcImagePath ) throws_();

	bool DiscardThumbnail( const fs::CFlexPath& srcImagePath );			// force discard a cached thumbnail, should't really be used
private:
	// hidden base methods
	using CShellThumbCache::ExtractThumb;
public:
	enum CacheStatusFlags
	{
		CacheHit			= BIT_FLAG( 0 ),
		CacheRemoveExpired	= BIT_FLAG( 1 ),
		CacheExtract		= BIT_FLAG( 2 ),
		Generate			= BIT_FLAG( 3 )
	};

	const CFlagTags& GetTags_CacheStatusFlags( void );

	// ui::ICustomImageDraw interface
	virtual CSize GetItemImageSize( ui::ICustomImageDraw::ImageType imageType = ui::ICustomImageDraw::SmallImage ) const;
	virtual bool SetItemImageSize( const CSize& imageBoundsSize );
	virtual bool DrawItemImage( CDC* pDC, const utl::ISubject* pSubject, const CRect& itemImageRect );
private:
	enum { MaxSize = 500 };

	fs::CFileObjectCache< fs::CFlexPath, CCachedThumbBitmap > m_thumbsCache;
public:
	int m_flags;
	static const GUID& s_containerFormatId;			// GUID_ContainerFormatJpeg

	enum Flags
	{
		AutoRegenSmallStgThumbs	= BIT_FLAG( 0 )		// for archive stg thumbs: regenerate if to small - this will slow down the process
	};
};


class CCachedThumbBitmap : public CWicDibSection
{
	friend class CShellThumbCache;

	CCachedThumbBitmap( IWICBitmapSource* pUnscaledBitmap, IWICBitmapSource* pScaledBitmap, const fs::CFlexPath& srcImagePath, const CThumbKey* pCachedKey = NULL );
public:
	const CThumbKey& GetKey( void ) const { return m_key; }
	const fs::CFlexPath& GetSrcImagePath( void ) const { return m_srcImagePath; }
	std::tstring FormatDbg( void ) const;

	const CSize& GetUnscaledBmpSize( void ) const { return m_unscaledBmpSize; }

	bool IsCachedByShell( void ) const { return m_key != m_nullKey; }

	fs::FileExpireStatus CheckExpired( void ) const;		// src image timestamp and boundsSize enlargement
	bool IsTooSmall( const CSize& boundsSize ) const { return m_unscaledBmpSize.cx < boundsSize.cx && m_unscaledBmpSize.cy < boundsSize.cy; }
private:
	fs::CFlexPath m_srcImagePath;			// path of the source image (large)
	CThumbKey m_key;						// only for shell cache extraction: a hashed key of the image content, sort of CRC: it depends on image contents
	CTime m_lastModifTime;					// refers to source image
	const CSize m_unscaledBmpSize;			// original size of the bitmap source when the thumb was extracted, from which it was scaled to bounds
public:
	static const CThumbKey m_nullKey;
};


namespace thumb
{
	class CPushBoundsSize
	{
	public:
		CPushBoundsSize( CThumbnailer* pThumbnailer, int boundsSize )
			: m_pThumbnailer( pThumbnailer )
			, m_oldBoundsSize( pThumbnailer->GetBoundsSize() )
		{
			m_pThumbnailer->SetBoundsSize( CSize( boundsSize, boundsSize ) );
		}

		~CPushBoundsSize() { m_pThumbnailer->SetBoundsSize( m_oldBoundsSize ); }
	private:
		CThumbnailer* m_pThumbnailer;
		CSize m_oldBoundsSize;
	};
}


#endif // Thumbnailer_h
