
#include "stdafx.h"
#include "GroupIcon.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace pred
{
	struct CompareIconStdSize
	{
		CompareResult operator()( const res::CGroupIconEntry& left, const res::CGroupIconEntry& right ) const
		{
			return Compare_Scalar( CIconSize::FindStdSize( left.GetSize() ), CIconSize::FindStdSize( right.GetSize() ) );
		}
	};
}


// CGroupIcon implementation

const std::pair<TBitsPerPixel, IconStdSize> CGroupIcon::m_nullBppStdSize( 0, DefaultSize );

CGroupIcon::CGroupIcon( UINT iconId )
	: m_resGroupIcon( MAKEINTRESOURCE( iconId ), RT_GROUP_ICON )
	, m_pGroupIconDir( m_resGroupIcon.GetResource< res::CGroupIconDir >() )
{
}

const res::CGroupIconEntry* CGroupIcon::FindMatch( TBitsPerPixel bitsPerPixel, IconStdSize iconStdSize ) const
{
	if ( IsValid() )
	{
		typedef std::reverse_iterator< const res::CGroupIconEntry* > const_reverse_iterator;

		for ( const_reverse_iterator it( m_pGroupIconDir->End() ), itEnd( m_pGroupIconDir->Begin() ); it != itEnd; ++it )
			if ( DefaultSize == iconStdSize || iconStdSize == CIconSize::FindStdSize( it->GetSize() ) )		// size match
				if ( AnyBpp == bitsPerPixel || bitsPerPixel == it->GetBitsPerPixel() )					// colour depth match
					return &*it;
	}

	return nullptr;
}

bool CGroupIcon::ContainsSize( IconStdSize iconStdSize, TBitsPerPixel* pBitsPerPixel /*= nullptr*/ ) const
{
	if ( const res::CGroupIconEntry* pFound = FindMatch( AnyBpp, iconStdSize ) )
	{
		if ( pBitsPerPixel != nullptr )
			*pBitsPerPixel = pFound->GetBitsPerPixel();
		return true;
	}
	return false;
}

bool CGroupIcon::ContainsBpp( TBitsPerPixel bitsPerPixel, IconStdSize* pIconStdSize /*= nullptr*/ ) const
{
	if ( const res::CGroupIconEntry* pFound = FindMatch( bitsPerPixel, static_cast< IconStdSize >( DefaultSize ) ) )
	{
		if ( pIconStdSize != nullptr )
			*pIconStdSize = CIconSize::FindStdSize( pFound->GetSize() );
		return true;
	}
	return false;
}

std::pair<TBitsPerPixel, IconStdSize> CGroupIcon::FindSmallest( void ) const
{
	if ( IsValid() )		// strictly minimum size
		return ToBppSize( std::min_element( m_pGroupIconDir->Begin(), m_pGroupIconDir->End(), pred::LessValue< pred::CompareIcon_Size >() ) );
	return m_nullBppStdSize;
}

std::pair<TBitsPerPixel, IconStdSize> CGroupIcon::FindLargest( void ) const
{
	if ( IsValid() )		// max colour maximum size (I think max colour takes precedence by icon scaler)
		return ToBppSize( std::max_element( m_pGroupIconDir->Begin(), m_pGroupIconDir->End(), pred::LessValue< pred::CompareIcon_BppSize >() ) );
	return m_nullBppStdSize;
}

void CGroupIcon::QueryAvailableSizes( std::vector< std::pair<TBitsPerPixel, IconStdSize> >& rIconPairs ) const
{
	rIconPairs.clear();

	if ( IsValid() )
	{
		for ( int i = 0; i != m_pGroupIconDir->m_count; ++i )
			rIconPairs.push_back( std::make_pair( m_pGroupIconDir->m_images[ i ].GetBitsPerPixel(), CIconSize::FindStdSize( m_pGroupIconDir->m_images[ i ].GetSize() ) ) );

		std::sort( rIconPairs.begin(), rIconPairs.end(), pred::LessValue< pred::CompareIcon_BppSize >() );		// BitsPerPixel | Width
	}
}

void CGroupIcon::QueryAvailableSizes( std::vector< std::pair<TBitsPerPixel, CSize> >& rIconPairs ) const
{
	rIconPairs.clear();

	if ( IsValid() )
	{
		for ( int i = 0; i != m_pGroupIconDir->m_count; ++i )
			rIconPairs.push_back( std::make_pair( m_pGroupIconDir->m_images[ i ].GetBitsPerPixel(), m_pGroupIconDir->m_images[ i ].GetSize() ) );

		std::sort( rIconPairs.begin(), rIconPairs.end(), pred::LessValue< pred::CompareIcon_BppSize >() );		// BitsPerPixel | Width
	}
}
