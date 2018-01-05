
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
			return Compare_Scalar( CIconId::FindStdSize( left.GetSize() ), CIconId::FindStdSize( right.GetSize() ) );
		}
	};
}


// CGroupIcon implementation

const std::pair< BitsPerPixel, IconStdSize > CGroupIcon::m_nullBppStdSize( 0, DefaultSize );

CGroupIcon::CGroupIcon( UINT iconId )
	: m_resGroupIcon( MAKEINTRESOURCE( iconId ), RT_GROUP_ICON )
	, m_pGroupIconDir( m_resGroupIcon.GetResource< res::CGroupIconDir >() )
{
}

const res::CGroupIconEntry* CGroupIcon::FindMatch( BitsPerPixel bitsPerPixel, IconStdSize iconStdSize ) const
{
	if ( IsValid() )
	{
		typedef std::reverse_iterator< const res::CGroupIconEntry* > const_reverse_iterator;

		for ( const_reverse_iterator it( m_pGroupIconDir->End() ), itEnd( m_pGroupIconDir->Begin() ); it != itEnd; ++it )
			if ( DefaultSize == iconStdSize || iconStdSize == CIconId::FindStdSize( it->GetSize() ) )		// size match
				if ( AnyBpp == bitsPerPixel || bitsPerPixel == it->GetBitsPerPixel() )					// colour depth match
					return &*it;
	}

	return NULL;
}

bool CGroupIcon::ContainsSize( IconStdSize iconStdSize, BitsPerPixel* pBitsPerPixel /*= NULL*/ ) const
{
	if ( const res::CGroupIconEntry* pFound = FindMatch( AnyBpp, iconStdSize ) )
	{
		if ( pBitsPerPixel != NULL )
			*pBitsPerPixel = pFound->GetBitsPerPixel();
		return true;
	}
	return false;
}

bool CGroupIcon::ContainsBpp( BitsPerPixel bitsPerPixel, IconStdSize* pIconStdSize /*= NULL*/ ) const
{
	if ( const res::CGroupIconEntry* pFound = FindMatch( bitsPerPixel, static_cast< IconStdSize >( DefaultSize ) ) )
	{
		if ( pIconStdSize != NULL )
			*pIconStdSize = CIconId::FindStdSize( pFound->GetSize() );
		return true;
	}
	return false;
}

std::pair< BitsPerPixel, IconStdSize > CGroupIcon::FindSmallest( void ) const
{
	if ( IsValid() )		// strictly minimum size
		return ToBppSize( std::min_element( m_pGroupIconDir->Begin(), m_pGroupIconDir->End(), pred::LessBy< pred::CompareIcon_Size >() ) );
	return m_nullBppStdSize;
}

std::pair< BitsPerPixel, IconStdSize > CGroupIcon::FindLargest( void ) const
{
	if ( IsValid() )		// max colour maximum size (I think max colour takes precedence by icon scaler)
		return ToBppSize( std::max_element( m_pGroupIconDir->Begin(), m_pGroupIconDir->End(), pred::LessBy< pred::CompareIcon_BppSize >() ) );
	return m_nullBppStdSize;
}

void CGroupIcon::QueryAvailableSizes( std::vector< std::pair< BitsPerPixel, IconStdSize > >& rIconPairs ) const
{
	rIconPairs.clear();

	if ( IsValid() )
	{
		for ( int i = 0; i != m_pGroupIconDir->m_count; ++i )
			rIconPairs.push_back( std::make_pair( m_pGroupIconDir->m_images[ i ].GetBitsPerPixel(), CIconId::FindStdSize( m_pGroupIconDir->m_images[ i ].GetSize() ) ) );

		std::sort( rIconPairs.begin(), rIconPairs.end(), pred::LessBy< pred::CompareIcon_BppSize >() );		// BitsPerPixel | Width
	}
}

void CGroupIcon::QueryAvailableSizes( std::vector< std::pair< BitsPerPixel, CSize > >& rIconPairs ) const
{
	rIconPairs.clear();

	if ( IsValid() )
	{
		for ( int i = 0; i != m_pGroupIconDir->m_count; ++i )
			rIconPairs.push_back( std::make_pair( m_pGroupIconDir->m_images[ i ].GetBitsPerPixel(), m_pGroupIconDir->m_images[ i ].GetSize() ) );

		std::sort( rIconPairs.begin(), rIconPairs.end(), pred::LessBy< pred::CompareIcon_BppSize >() );		// BitsPerPixel | Width
	}
}
