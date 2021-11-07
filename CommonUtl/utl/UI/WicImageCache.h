#ifndef WicImageCache_h
#define WicImageCache_h
#pragma once

#include "CacheLoader.h"
#include "FlexPath.h"
#include "ErrorHandler.h"
#include "WicImage.h"
#include <deque>


namespace fs { enum FileExpireStatus; }


class CWicImageCache : public CErrorHandler
					 , private fs::ICacheOwner< fs::TImagePathKey, CWicImage >
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
	std::pair<CWicImage*, fs::cache::TStatusFlags> Acquire( const fs::TImagePathKey& imageKey );
	bool Discard( const fs::TImagePathKey& imageKey );
	size_t DiscardFrames( const fs::CFlexPath& imagePath );

	fs::cache::EnqueueResult Enqueue( const fs::TImagePathKey& imageKey );
	void Enqueue( const std::vector< fs::TImagePathKey >& imageKeys );

	enum { MaxSize = 30u };

	typedef fs::CCacheLoader< fs::TImagePathKey, CWicImage > TCacheLoader;

	TCacheLoader* GetCache( void ) { return &m_imageCache; }

	CComPtr<IWICBitmapSource> LookupBitmapSource( const fs::TImagePathKey& imageKey ) const;		// fast load if not cached, no bitmap copy
	CSize LookupImageDim( const fs::TImagePathKey& imageKey ) const;

	size_t DiscardWithPrefix( const TCHAR* pDirPrefix );
private:
	// fs::ICacheOwner< fs::TImagePathKey, CWicImage > interface
	virtual CWicImage* LoadObject( const fs::TImagePathKey& imageKey );
	virtual void TraceObject( const fs::TImagePathKey& imageKey, CWicImage* pImage, fs::cache::TStatusFlags cacheFlags );
private:
	TCacheLoader m_imageCache;
	static size_t s_traceCount;
};


#endif // WicImageCache_h
