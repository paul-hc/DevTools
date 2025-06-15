
#include "pch.h"
#include "GroupIconRes.h"
#include "Icon.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CGroupIconRes implementation

const std::pair<TBitsPerPixel, IconStdSize> CGroupIconRes::s_nullBppStdSize( 0, DefaultSize );

CGroupIconRes::CGroupIconRes( UINT iconId )
	: m_resGroupIcon( MAKEINTRESOURCE( iconId ), RT_GROUP_ICON )				// Group Icon resource
	, m_pGroupIconDir( m_resGroupIcon.GetResource<res::CGroupIconDir>() )
{
}

CIcon* CGroupIconRes::LoadIconAt( size_t framePos ) const
{
	return CIcon::LoadNewIcon( GetIconEntryAt( framePos ) );
}

HICON CGroupIconRes::LoadIconEntry( const res::CGroupIconEntry& iconEntry )
{
	CResourceData resFrameIcon( MAKEINTRESOURCE( iconEntry.m_id ), RT_ICON );	// Icon Frame resource

	if ( BYTE* pIconBytes = resFrameIcon.GetResource<BYTE>() )
		if ( HICON hIcon = ::CreateIconFromResourceEx( pIconBytes, resFrameIcon.GetSize(), TRUE, IconVersionNumberFormat, iconEntry.GetWidth(), iconEntry.GetHeight(), LR_DEFAULTCOLOR ) )
		{
		#ifdef _DEBUG
			CIconInfo iconInfo( hIcon ); iconInfo;			// inspect unusual icon formats
			bool hasAlpha = iconInfo.HasAlpha(); hasAlpha;
		#endif

			return hIcon;
		}

	return nullptr;
}

const res::CGroupIconEntry* CGroupIconRes::FindMatch( TBitsPerPixel bitsPerPixel, IconStdSize iconStdSize ) const
{
	if ( IsValid() )
	{
		typedef std::reverse_iterator<const res::CGroupIconEntry*> const_reverse_iterator;

		for ( const_reverse_iterator it( m_pGroupIconDir->End() ), itEnd( m_pGroupIconDir->Begin() ); it != itEnd; ++it )
			if ( DefaultSize == iconStdSize || iconStdSize == CIconSize::FindStdSize( it->GetSize() ) )		// size match
				if ( AnyBpp == bitsPerPixel || bitsPerPixel == it->GetBitsPerPixel() )						// colour depth match
					return &*it;
	}

	return nullptr;
}

bool CGroupIconRes::ContainsSize( IconStdSize iconStdSize, TBitsPerPixel* pOutBitsPerPixel /*= nullptr*/ ) const
{
	if ( const res::CGroupIconEntry* pFound = FindMatch( AnyBpp, iconStdSize ) )
	{
		if ( pOutBitsPerPixel != nullptr )
			*pOutBitsPerPixel = pFound->GetBitsPerPixel();
		return true;
	}
	return false;
}

bool CGroupIconRes::ContainsBpp( TBitsPerPixel bitsPerPixel, IconStdSize* pOutIconStdSize /*= nullptr*/ ) const
{
	if ( const res::CGroupIconEntry* pFound = FindMatch( bitsPerPixel, static_cast<IconStdSize>( DefaultSize ) ) )
	{
		if ( pOutIconStdSize != nullptr )
			*pOutIconStdSize = CIconSize::FindStdSize( pFound->GetSize() );

		return true;
	}
	return false;
}

std::pair<TBitsPerPixel, IconStdSize> CGroupIconRes::FindSmallest( void ) const
{
	if ( IsValid() )		// strictly minimum size
		return ToBppSize( std::min_element( m_pGroupIconDir->Begin(), m_pGroupIconDir->End(), pred::LessValue<pred::CompareIcon_Size>() ) );

	return s_nullBppStdSize;
}

std::pair<TBitsPerPixel, IconStdSize> CGroupIconRes::FindLargest( void ) const
{
	if ( IsValid() )		// max colour maximum size (I think max colour takes precedence by icon scaler)
		return ToBppSize( std::max_element( m_pGroupIconDir->Begin(), m_pGroupIconDir->End(), pred::LessValue<pred::CompareIcon_BppSize>() ) );

	return s_nullBppStdSize;
}

void CGroupIconRes::QueryAvailableSizes( std::vector< std::pair<TBitsPerPixel, IconStdSize> >& rIconPairs ) const
{
	rIconPairs.clear();

	if ( IsValid() )
	{
		for ( int i = 0; i != m_pGroupIconDir->m_count; ++i )
			rIconPairs.push_back( std::make_pair( m_pGroupIconDir->m_images[ i ].GetBitsPerPixel(), CIconSize::FindStdSize( m_pGroupIconDir->m_images[ i ].GetSize() ) ) );

		std::sort( rIconPairs.begin(), rIconPairs.end(), pred::LessValue<pred::CompareIcon_BppSize>() );		// BitsPerPixel | Width
	}
}

void CGroupIconRes::QueryAvailableSizes( std::vector< std::pair<TBitsPerPixel, CSize> >& rIconPairs ) const
{
	rIconPairs.clear();

	if ( IsValid() )
	{
		for ( int i = 0; i != m_pGroupIconDir->m_count; ++i )
			rIconPairs.push_back( std::make_pair( m_pGroupIconDir->m_images[ i ].GetBitsPerPixel(), m_pGroupIconDir->m_images[ i ].GetSize() ) );

		std::sort( rIconPairs.begin(), rIconPairs.end(), pred::LessValue<pred::CompareIcon_BppSize>() );		// BitsPerPixel | Width
	}
}


namespace dbg
{
	std::tstring dbg::FormatGroupIconEntry( const res::CGroupIconEntry& entry )
	{
	#ifdef _DEBUG
		return str::Format( _T("EntryId=%d\tBPP=%d\tSize=(%d, %d)\tBytesInRes=%d"), entry.m_id, entry.GetBitsPerPixel(), entry.GetWidth(), entry.GetHeight(), entry.m_bytesInRes );
	#else
		entry;
		return str::GetEmpty();
	#endif
	}
}
