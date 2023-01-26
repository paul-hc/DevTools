#ifndef Thumbnailer_fwd_h
#define Thumbnailer_fwd_h
#pragma once

#include "ComparePredicates.h"
#include "StdHashValue.h"
#include "FlexPath.h"
#include <thumbcache.h>				// IThumbnailCache, CLSID_ThumbnailCache


class CCachedThumbBitmap;


namespace fs
{
	// used to plug-in a thumbnail producer for structured storage container; it uses storage-friendly flex paths

	interface IThumbProducer
	{
		virtual bool ProducesThumbFor( const fs::CFlexPath& srcImagePath ) const = 0;

		virtual CCachedThumbBitmap* ExtractThumb( const fs::CFlexPath& srcImagePath ) = 0;
		virtual CCachedThumbBitmap* GenerateThumb( const fs::CFlexPath& srcImagePath ) = 0;
	};
}


namespace pred
{
	struct CompareThumbId
	{
		CompareResult operator()( const WTS_THUMBNAILID& left, const WTS_THUMBNAILID& right ) const
		{
			return ToCompareResult( std::memcmp( left.rgbKey, right.rgbKey, sizeof( WTS_THUMBNAILID ) ) );
		}
	};
}


struct CThumbKey : public WTS_THUMBNAILID
{
	CThumbKey( bool nullKey = false ) { if ( nullKey ) memset( rgbKey, 0, sizeof( WTS_THUMBNAILID ) ); }

	bool operator==( const WTS_THUMBNAILID& right ) const { return pred::Equal == pred::CompareThumbId()( *this, right ); }
	bool operator!=( const WTS_THUMBNAILID& right ) const { return !operator==( right ); }

	bool operator<( const WTS_THUMBNAILID& right ) const { return pred::Less == pred::CompareThumbId()( *this, right ); }

	std::tstring Format( void ) const { std::pair<UINT64, UINT64> idPair = ToPair( *this ); return str::Format( _T("{%llx,%llx}"), idPair.first, idPair.second ); }

	static std::pair<UINT64, UINT64> ToPair( const WTS_THUMBNAILID& id ) { return std::make_pair( *(const UINT64*)&id.rgbKey[ 0 ], *(const UINT64*)&id.rgbKey[ 8 ] ); }
};


template<>
struct std::hash<WTS_THUMBNAILID>
{
	inline std::size_t operator()( const WTS_THUMBNAILID& id ) const /*noexcept*/
    {
        return utl::GetPairHashValue( CThumbKey::ToPair( id ) );
    }
};


#endif // Thumbnailer_fwd_h
