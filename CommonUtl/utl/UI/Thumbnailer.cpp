
#include "stdafx.h"
#include "Thumbnailer.h"
#include "EnumTags.h"
#include "FlagTags.h"
#include "FileSystem.h"
#include "ImagingWic.h"
#include "StreamStdTypes.h"
#include "StructuredStorage.h"
#include "GdiCoords.h"
#include "TimeUtils.h"
#include "BaseApp.h"
#include "FileObjectCache.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//#define TRACE_THUMBS TRACE
#define TRACE_THUMBS __noop


namespace thumb
{
	static size_t s_traceCount = 0;

	const CEnumTags& GetTags_StdBoundsSize( void )
	{
		static const CEnumTags s_tags( _T("16|32|48|64|96|128|256|512|1024") );
		return s_tags;
	}
}


// CShellThumbCache implementation

const CSize CShellThumbCache::s_defaultBoundsSize( thumb::DefaultBoundsSize, thumb::DefaultBoundsSize );

CShellThumbCache::CShellThumbCache( void )
	: m_boundsSize( s_defaultBoundsSize )
	, m_pThumbProducer( NULL )
	, m_thumbExtractFlags( SIIGBF_BIGGERSIZEOK )
{
	// (*) COM must be initialized by now
	m_pShellThumbCache.CoCreateInstance( CLSID_LocalThumbnailCache, NULL, CLSCTX_INPROC );

	ASSERT_PTR( m_pShellThumbCache );
}

void CShellThumbCache::ReleaseShellCache( void )
{
	m_pShellThumbCache = NULL;
}

bool CShellThumbCache::SetBoundsSize( const CSize& boundsSize )
{
	if ( m_boundsSize == boundsSize )
		return false;

	ASSERT( boundsSize.cx >= 16 && boundsSize.cy >= 16 );
	m_boundsSize = boundsSize;
	return true;
}

CComPtr< IShellItem > CShellThumbCache::FindShellItem( const fs::CFlexPath& filePath ) const
{
	if ( !filePath.IsComplexPath() )
		return m_shellExplorer.FindShellItem( filePath );
	return NULL;
}

CCachedThumbBitmap* CShellThumbCache::ExtractThumb( const ShellItemPair& imagePair )
{
	if ( m_pThumbProducer != NULL && m_pThumbProducer->ProducesThumbFor( imagePair.first ) )
		return m_pThumbProducer->ExtractThumb( imagePair.first );			// chain to external thumb producer

	if ( IsValidShellCache() && imagePair.second != NULL )		// valid physical shell item?
		switch ( ui::FindImageFileFormat( imagePair.first.GetPtr() ) )
		{
			case ui::IconFormat:				// better generate the icon for best fitting image (minimize icon scaling)
			case ui::UnknownImageFormat:		// don't bother to extract thumbnail
				break;
			default:
			{
				CComPtr< ISharedBitmap > pSharedThumb;
				WTS_CACHEFLAGS cacheFlags;
				CThumbKey thumbKey;

				// use IThumbnailCache to produce a usually larger thumb bitmap, which will be scaled
				if ( HR_OK( m_pShellThumbCache->GetThumbnail( imagePair.second, m_boundsSize.cx, WTS_EXTRACT, &pSharedThumb, &cacheFlags, &thumbKey ) ) )
				{
			        WTS_ALPHATYPE alphaType = WTSAT_UNKNOWN;
			        HR_AUDIT( pSharedThumb->GetFormat( &alphaType ) );

					HBITMAP hSharedBitmap = NULL;				// note: shared bitmap cannot be selected into a DC - must either Detach() or copy to a WIC bitmap
					if ( HR_OK( pSharedThumb->GetSharedBitmap( &hSharedBitmap ) ) )
						return NewScaledThumb( wic::cvt::ToWicBitmap( hSharedBitmap ), imagePair.first, &thumbKey );
				}
				else
					TRACE( _T("  for file: %s\n"), imagePair.first.GetPtr() );
			}
		}

	return NULL;
}

CCachedThumbBitmap* CShellThumbCache::GenerateThumb( const ShellItemPair& imagePair )
{
	if ( m_pThumbProducer != NULL && m_pThumbProducer->ProducesThumbFor( imagePair.first ) )
		return m_pThumbProducer->GenerateThumb( imagePair.first );			// chain to external thumb producer

	if ( imagePair.second != NULL )		// valid physical shell item?
		// use IShellItemImageFactory to produce a usually larger thumb bitmap, which will be scaled
		if ( HBITMAP hThumbBitmap = m_shellExplorer.ExtractThumbnail( imagePair.second, m_boundsSize, m_thumbExtractFlags ) )
			if ( CComPtr< IWICBitmapSource > pUnscaledBitmap = wic::cvt::ToWicBitmap( hThumbBitmap ) )
				return NewScaledThumb( pUnscaledBitmap, imagePair.first, NULL );

	return NULL;
}

CCachedThumbBitmap* CShellThumbCache::NewScaledThumb( IWICBitmapSource* pUnscaledBitmap, const fs::CFlexPath& srcImagePath, const CThumbKey* pCachedKey /*= NULL*/ ) const
{
	if ( pUnscaledBitmap != NULL )
		if ( CComPtr< IWICBitmapSource > pScaledBitmap = ScaleToThumbBitmap( pUnscaledBitmap ) )			// scale to m_boundsSize
			return new CCachedThumbBitmap( pUnscaledBitmap, pScaledBitmap, srcImagePath, pCachedKey );

	return NULL;
}

fs::FileExpireStatus CShellThumbCache::CheckThumbExpired( const CCachedThumbBitmap* pThumb ) const
{
	ASSERT_PTR( pThumb );

#if 0		// disable this code since it doesn't work: GetThumbnailByID() returns an old cached image even when image has changed
	if ( pThumb->IsCachedByShell() && IsValidShellCache() )
	{
		CComPtr< ISharedBitmap > pSharedThumb;
		WTS_CACHEFLAGS cacheFlags;

		if ( HR_OK( m_pShellThumbCache->GetThumbnailByID( pThumb->GetKey(), m_boundsSize.cx, &pSharedThumb, &cacheFlags ) ) )
			return fs::FileNotExpired;		// found with same key, not expired? - that's not really the case, it keeps a cached bitmap of the old image
	}
#endif

	fs::FileExpireStatus expired = pThumb->CheckExpired();
	if ( expired != fs::FileNotExpired )
		TRACE_THUMBS( _T("<%d> Expired thumb in cache for image: %s - Reason: %s  %s\n"), thumb::s_traceCount++,
			pThumb->GetSrcImagePath().GetPtr(), fs::GetTags_FileExpireStatus().GetUiTags()[ expired ].c_str(), pThumb->FormatDbg().c_str() );

	return expired;							// thumb has expired
}

CComPtr< IWICBitmapSource > CShellThumbCache::ScaleToThumbBitmap( IWICBitmapSource* pSrcBitmap ) const
{
	ASSERT_PTR( pSrcBitmap );

	const CSize srcSize = wic::GetBitmapSize( pSrcBitmap );

	if ( ui::FitsExactly( m_boundsSize, srcSize ) )
		return pSrcBitmap;									// optimization: it's got the right fit -> no need to scale
	else if ( ui::FitsInside( m_boundsSize, srcSize ) )
		return pSrcBitmap;									// optimization: image smaller than the thumb size -> avoid scaling, since the thumb looks "smeared"

	// convert to WIC bitmap and scale to m_boundsSize
	CComPtr< IWICBitmapSource > pScaledThumbBitmap = wic::ScaleBitmapToBounds( pSrcBitmap, m_boundsSize, WICBitmapInterpolationModeFant );		// or WICBitmapInterpolationModeHighQualityCubic
	return pScaledThumbBitmap;
}


// CThumbnailer implementation

CThumbnailer::CThumbnailer( size_t cacheMaxSize /*= MaxSize*/ )
	: CShellThumbCache()
	, m_thumbsCache( cacheMaxSize )
	, m_flags( 0 )
{
}

size_t CThumbnailer::GetCachedCount( void ) const
{
	return m_thumbsCache.GetCount();
}

bool CThumbnailer::DiscardThumbnail( const fs::CFlexPath& srcImagePath )
{
	return m_thumbsCache.Remove( srcImagePath );
}

void CThumbnailer::Clear( void )
{
	thumb::s_traceCount = 0;
	TRACE_THUMBS( _T("<%d> (--) Clear all thumbnails: %d\n"), thumb::s_traceCount++, m_thumbsCache.GetCount() );
	m_thumbsCache.Clear();
}

const CFlagTags& CThumbnailer::GetTags_CacheStatusFlags( void )
{
	static const CFlagTags::FlagDef flagDefs[] =
	{
		{ CacheHit, _T("") },				// silent on cache hits (not the interesting case)
		{ CacheRemoveExpired, _T("(-) remove expired") },
		{ CacheExtract, _T("(+) extract from cache") },
		{ Generate, _T("(++) generate") }
	};
	static const CFlagTags tags( flagDefs, COUNT_OF( flagDefs ) );
	return tags;
}

bool CThumbnailer::SetBoundsSize( const CSize& boundsSize )
{
	if ( !CShellThumbCache::SetBoundsSize( boundsSize ) )
		return false;			// not changed

	Clear();
	return true;
}

CCachedThumbBitmap* CThumbnailer::AcquireThumbnail( const fs::CFlexPath& srcImagePath, int* pCacheStatusFlags /*= NULL*/ )
{
	int cacheStatus = 0;

	CCachedThumbBitmap* pThumb = m_thumbsCache.Find( srcImagePath );
	if ( pThumb != NULL )
		if ( fs::FileNotExpired == CheckThumbExpired( pThumb ) )
			SetFlag( cacheStatus, CacheHit );
		else
		{
			m_thumbsCache.Remove( srcImagePath );				// delete expired entry
			pThumb = NULL;
			SetFlag( cacheStatus, CacheRemoveExpired );
		}

	ShellItemPair imagePair( srcImagePath, NULL );

	if ( NULL == pThumb )
	{
		imagePair.second = FindShellItem( srcImagePath );		// bind to the actual shell item
		pThumb = ExtractThumb( imagePair );

		if ( pThumb != NULL && NULL == imagePair.second )		// externally produced?
			if ( HasFlag( m_flags, AutoRegenSmallStgThumbs ) )
				if ( pThumb->IsTooSmall( GetBoundsSize() ) )
				{	// archived thumb is too small -> force generation; this will slow down thumb access, but thumbs will look better.
					TRACE_THUMBS( _T("<%d> Regenerating cached thumb for image: %s - Reason: resolution is too small\n"), thumb::s_traceCount++, pThumb->GetSrcImagePath().GetPtr() );

					delete pThumb;
					pThumb = NULL;
				}

		SetFlag( cacheStatus, CacheExtract, pThumb != NULL );
	}

	if ( NULL == pThumb )
	{
		pThumb = GenerateThumb( imagePair );
		SetFlag( cacheStatus, Generate, pThumb != NULL );
	}

	if ( pThumb != NULL && !HasFlag( cacheStatus, CacheHit ) )
		m_thumbsCache.Add( pThumb->GetSrcImagePath(), pThumb );

#ifdef _DEBUG
	std::tstring flagsText = GetTags_CacheStatusFlags().FormatUi( cacheStatus, _T(",") );
	if ( !flagsText.empty() )
		TRACE_THUMBS( _T("<%d> Thumb '%s' for %s: %s\n"), thumb::s_traceCount++,
			flagsText.c_str(),
			srcImagePath.IsComplexPath() ? _T("EMBEDDED image") : _T("image"),
			pThumb != NULL ? pThumb->FormatDbg().c_str() : _T("NULL") );
#endif

	if ( pCacheStatusFlags != NULL )
		*pCacheStatusFlags = cacheStatus;
	return pThumb;
}

CCachedThumbBitmap* CThumbnailer::AcquireThumbnailNoThrow( const fs::CFlexPath& srcImagePath ) throws_()
{
	try
	{
		return AcquireThumbnail( srcImagePath );
	}
	catch ( CException* pExc )
	{
		pExc->Delete();
		return NULL;
	}
}

CSize CThumbnailer::GetItemImageSize( ui::GlyphGauge glyphGauge /*= ui::SmallGlyph*/ ) const
{
	switch ( glyphGauge )
	{
		default: ASSERT( false );
		case ui::SmallGlyph:	return CIconId::GetStdSize( SmallIcon );
		case ui::LargeGlyph:	return GetBoundsSize();
	}
}

bool CThumbnailer::SetItemImageSize( const CSize& imageBoundsSize )
{
	return SetBoundsSize( imageBoundsSize );
}

bool CThumbnailer::DrawItemImage( CDC* pDC, const utl::ISubject* pSubject, const CRect& itemImageRect )
{
	ASSERT_PTR( pDC );
	if ( pSubject != NULL )
	{
		fs::CFlexPath srcImagePath( pSubject->GetCode() );
		if ( srcImagePath.FileExist() )
			if ( CCachedThumbBitmap* pThumbnail = AcquireThumbnail( srcImagePath ) )
			{
				pThumbnail->Draw( pDC, itemImageRect, ui::ShrinkFit );
				return true;
			}
	}
	return false;
}


// CGlyphThumbnailer implementation

CGlyphThumbnailer::CGlyphThumbnailer( int glyphDimension )
	: CThumbnailer( 2500 )
{
	SetGlyphDimension( glyphDimension );
	SetOptimizeExtractIcons();					// for more accurate icon scaling that favours the best fitting image size present
}

bool CGlyphThumbnailer::SetGlyphDimension( int glyphDimension )
{
	return SetBoundsSize( CSize( glyphDimension, glyphDimension ) );
}

CSize CGlyphThumbnailer::GetItemImageSize( ui::GlyphGauge glyphGauge /*= ui::SmallGlyph*/ ) const
{
	glyphGauge;
	return GetBoundsSize();			// fixed gauge, regardless of glyphGauge
}

bool CGlyphThumbnailer::SetItemImageSize( const CSize& imageBoundsSize )
{
	imageBoundsSize;
	ASSERT( false );				// should never be called
	return false;
}


// CCachedThumbBitmap implementation

const CThumbKey CCachedThumbBitmap::m_nullKey( true );

CCachedThumbBitmap::CCachedThumbBitmap( IWICBitmapSource* pUnscaledBitmap, IWICBitmapSource* pScaledBitmap, const fs::CFlexPath& srcImagePath, const CThumbKey* pCachedKey /*= NULL*/ )
	: CWicDibSection( pScaledBitmap )
	, m_srcImagePath( srcImagePath )
	, m_key( pCachedKey != NULL ? *pCachedKey : m_nullKey )
	, m_lastModifTime( fs::ReadLastModifyTime( m_srcImagePath ) )
	, m_unscaledBmpSize( pUnscaledBitmap != NULL ? wic::GetBitmapSize( pUnscaledBitmap ) : GetBmpFmt().m_size )
{
}

fs::FileExpireStatus CCachedThumbBitmap::CheckExpired( void ) const
{
	return fs::CheckExpireStatus( m_srcImagePath, m_lastModifTime );
}


#ifdef _DEBUG


std::tstring CCachedThumbBitmap::FormatDbg( void ) const
{
	using namespace stream;
	static const TCHAR sep[] = _T("  ");

	std::tostringstream oss;
	oss << m_srcImagePath.Get();
	oss << sep << _T("src_dims=") << m_unscaledBmpSize;
	if ( GetBmpFmt().m_size != m_unscaledBmpSize )
		oss << sep << _T("size=") << GetBmpFmt().FormatDbg( sep );

	if ( time_utl::IsValid( m_lastModifTime ) != 0 )
		oss << _T("  modif_time=") << m_lastModifTime.Format( _T("%d/%m/%Y %H:%M:%S") ).GetString();

	if ( IsCachedByShell() )
		oss << _T("  key=") << m_key.Format();

	return oss.str();
}


#endif // _DEBUG
