
#include "stdafx.h"
#include "WicImageCache.h"
#include "WicImage.h"
#include "FlagTags.h"
#include "MfcUtilities.h"
#include "CacheLoader.hxx"
#include <functional>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define TRACE_CACHE TRACE
//#define TRACE_CACHE __noop


size_t CWicImageCache::s_traceCount = 0;

CWicImageCache::CWicImageCache( size_t maxSize )
	: CThrowMode( false )
	, m_imageCache( maxSize, this )
{
}

CWicImageCache::~CWicImageCache()
{
}

CWicImageCache& CWicImageCache::Instance( void )
{
	static CWicImageCache imageCache( MaxSize );
	return imageCache;
}

CWicImage* CWicImageCache::LoadObject( const fs::ImagePathKey& imageKey )
{
	return CWicImage::CreateFromFile( imageKey, IsThrowMode() ).release();
}

void CWicImageCache::TraceObject( const fs::ImagePathKey& imageKey, CWicImage* pImage, fs::cache::TStatusFlags cacheFlags )
{
#ifdef _DEBUG
	std::tstring flagsText = fs::cache::GetTags_StatusFlags().FormatUi( cacheFlags, _T(",") );
	if ( !flagsText.empty() )
		TRACE_CACHE( _T("{%d} '%s' %s: %s\n"), s_traceCount++,
			flagsText.c_str(),
			imageKey.first.IsComplexPath() ? _T("EMBEDDED image") : _T("image"),
			pImage != NULL ? pImage->FormatDbg().c_str() : _T("NULL") );
#else
	imageKey, pImage, cacheFlags;
#endif
}

void CWicImageCache::Clear( void )
{
	TRACE( _T(" (--) Clear all cached images: %d\n"), m_imageCache.GetCount() );
	s_traceCount = 0;
	m_imageCache.Clear();
}

size_t CWicImageCache::GetCount( void ) const
{
	return m_imageCache.GetCount();
}

std::pair< CWicImage*, fs::cache::TStatusFlags > CWicImageCache::Acquire( const fs::ImagePathKey& imageKey )
{
	return m_imageCache.Acquire( imageKey );
}

bool CWicImageCache::Discard( const fs::ImagePathKey& imageKey )
{
	return m_imageCache.Remove( imageKey );
}

size_t CWicImageCache::DiscardFrames( const fs::CFlexPath& imagePath )
{
	std::vector< fs::ImagePathKey > toDiscardKeys;

	const std::deque< fs::ImagePathKey >& pathKeys = m_imageCache.GetPathKeys();
	for ( std::deque< fs::ImagePathKey >::const_iterator itPathKey = pathKeys.begin(); itPathKey != pathKeys.end(); ++itPathKey )
		if ( imagePath == itPathKey->first )
			toDiscardKeys.push_back( *itPathKey );

	m_imageCache.Remove( toDiscardKeys.begin(), toDiscardKeys.end() );
	return toDiscardKeys.size();
}

fs::cache::EnqueueResult CWicImageCache::Enqueue( const fs::ImagePathKey& imageKey )
{
	return m_imageCache.Enqueue( imageKey );
}

void CWicImageCache::Enqueue( const std::vector< fs::ImagePathKey >& imageKeys )
{
	m_imageCache.Enqueue( imageKeys );
}

CComPtr< IWICBitmapSource > CWicImageCache::LookupBitmapSource( const fs::ImagePathKey& imageKey ) const
{
	CComPtr< IWICBitmapSource > pBitmap;

	if ( const CWicImage* pFoundImage = m_imageCache.Find( imageKey, true ) )		// already cached?
		pBitmap = pFoundImage->GetWicBitmap();
	else
	{
		wic::CBitmapDecoder decoder( imageKey.first );
		if ( decoder.IsValid() )
			pBitmap = decoder.GetFrameAt( imageKey.second );						// no conversion
	}

	return pBitmap;
}

CSize CWicImageCache::LookupImageDim( const fs::ImagePathKey& imageKey ) const
{
	return wic::GetBitmapSize( LookupBitmapSource( imageKey ) );					// get bitmap size efficiently, with no additional overhead
}

size_t CWicImageCache::DiscardWithPrefix( const TCHAR* pDirPrefix )
{
	std::vector< fs::ImagePathKey > toDiscardKeys;

	const std::deque< fs::ImagePathKey >& pathKeys = m_imageCache.GetPathKeys();
	for ( std::deque< fs::ImagePathKey >::const_iterator itPathKey = pathKeys.begin(); itPathKey != pathKeys.end(); ++itPathKey )
		if ( path::MatchPrefix( itPathKey->first.GetPtr(), pDirPrefix ) )
			toDiscardKeys.push_back( *itPathKey );

	m_imageCache.Remove( toDiscardKeys.begin(), toDiscardKeys.end() );
	return toDiscardKeys.size();
}
