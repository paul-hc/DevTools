#ifndef WicImageCache_h
#define WicImageCache_h
#pragma once

#include "CacheLoader.h"
#include "FlexPath.h"
#include "ThrowMode.h"
#include "WicImage.h"
#include <deque>


namespace fs { enum FileExpireStatus; }


class CWicImageCache : public CThrowMode
					 , private fs::ICacheOwner< fs::ImagePathKey, CWicImage >
					 , private utl::noncopyable
{
	friend class CWicImageTests;
private:
	CWicImageCache( size_t maxSize );
	~CWicImageCache();
public:
	static CWicImageCache& Instance( void );

	size_t GetCount( void ) const;

	void Clear( void );
	std::pair< CWicImage*, int > Acquire( const fs::ImagePathKey& imageKey );
	bool Discard( const fs::ImagePathKey& imageKey );
	size_t DiscardFrames( const fs::CFlexPath& imagePath );

	fs::cache::EnqueueResult Enqueue( const fs::ImagePathKey& imageKey );
	void Enqueue( const std::vector< fs::ImagePathKey >& imageKeys );

	enum { MaxSize = 30u };

	typedef fs::CCacheLoader< fs::ImagePathKey, CWicImage > TCacheLoader;

	TCacheLoader* GetCache( void ) { return &m_imageCache; }

	CComPtr< IWICBitmapSource > LookupBitmapSource( const fs::ImagePathKey& imageKey ) const;		// fast load if not cached, no bitmap copy
	CSize LookupImageDim( const fs::ImagePathKey& imageKey ) const;

	size_t DiscardWithPrefix( const TCHAR* pDirPrefix );
private:
	// fs::ICacheOwner< fs::ImagePathKey, CWicImage > interface
	virtual CWicImage* LoadObject( const fs::ImagePathKey& imageKey );
	virtual void TraceObject( const fs::ImagePathKey& imageKey, CWicImage* pImage, int cacheFlags );
private:
	TCacheLoader m_imageCache;
	static size_t s_traceCount;
};


#endif // WicImageCache_h
