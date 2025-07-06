
#include "pch.h"
#include "IconGroup.h"
#include "Icon.h"
#include "GroupIconRes.h"
#include "utl/ContainerOwnership.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CIconGroup implementation

void CIconGroup::Clear( void )
{
	for ( size_t framePos = 0; framePos != m_frames.size(); ++framePos )
		m_frames[ framePos ].second->ResetIconGroup();		// reset the back pointers to this

	utl::ClearOwningMapValues( m_frames );
}

size_t CIconGroup::LoadAllIcons( UINT iconResId )
{
	Clear();

	m_iconResId = iconResId;
	ASSERT( m_iconResId != 0 );

	const CGroupIconRes groupIcon( m_iconResId );

	//TRACE( _T("CIconGroup::LoadAllIcons( m_iconResId=%d ): %d frames\n"), m_iconResId, groupIcon.GetSize() );

	if ( groupIcon.IsValid() )
	{
		m_frames.reserve( groupIcon.GetSize() );

		for ( size_t framePos = 0, frameCount = groupIcon.GetSize(); framePos != frameCount; ++framePos )
		{
			const res::CGroupIconEntry& srcEntry = groupIcon.GetIconEntryAt( framePos );

			//TRACE( _T("\tEntry: framePos=%d,\t%s\n"), framePos, dbg::FormatGroupIconEntry( srcEntry ).c_str() );

			if ( srcEntry.GetBitsPerPixel() <= ILC_COLOR32 )			// not a weird entry?
				if ( CIcon* pIcon = CIcon::LoadNewIcon( srcEntry ) )	// load from resources this exact icon frame
				{
					ui::CIconEntry iconEntry( srcEntry.GetBitsPerPixel(), srcEntry.GetDimension() );

					if ( 0 == iconEntry.m_dimension )					// for unusual sizes (e.g. 256x256), pIcon->GetDimension() returns (0, 0)
						iconEntry.m_dimension = pIcon->GetDimension();	// re-evaluate icon's dimension

					m_frames.push_back( TIconPair( iconEntry, pIcon ) );
				}
				else
					ASSERT( false );		// error loading the icon entry fro resources?
			else
				TRACE( _T("\t\tIgnoring ill-formed group icon frame!\n") );
		}

		// sort by: BPP (desc) | Size
		std::sort( m_frames.begin(), m_frames.end(), pred::LessValue<pred::TCompareIconEntry>() );

		// store the back pointers to this icon group
		for ( size_t framePos = 0; framePos != m_frames.size(); ++framePos )
			m_frames[ framePos ].second->ResetIconGroup( this, framePos );
	}

	return GetSize();
}

size_t CIconGroup::AddIcon( const ui::CIconEntry& entry, CIcon* pIcon )
{
	ASSERT_PTR( pIcon );

	std::vector<TIconPair>::iterator itInsert = std::lower_bound( m_frames.begin(), m_frames.end(), TIconPair( entry, nullptr ), pred::LessValue<pred::TCompareIconEntry>() );

	if ( itInsert != m_frames.end() && itInsert->first == entry )		// replace an existing entry?
	{
		if ( itInsert->second != pIcon )
			delete itInsert->second;

		itInsert->second = pIcon;
	}
	else
		itInsert = m_frames.insert( itInsert, TIconPair( entry, pIcon ) );			// insert pair in sort order

	return std::distance( m_frames.begin(), itInsert );
}

bool CIconGroup::AugmentIcon( const ui::CIconEntry& entry, CIcon* pIcon )
{
	size_t frameCount = GetSize();

	AddIcon( entry, pIcon );
	return GetSize() != frameCount;		// true if inserted, false if replaced
}

void CIconGroup::DeleteFrameAt( size_t framePos )
{
	ASSERT( framePos < GetSize() );

	delete m_frames[ framePos ].second;
	m_frames.erase( m_frames.begin() + framePos );
}

size_t CIconGroup::FindPos( const ui::CIconEntry& entry ) const
{
	std::vector<TIconPair>::const_iterator itFound = std::lower_bound( m_frames.begin(), m_frames.end(), TIconPair( entry, nullptr ), pred::LessValue<pred::TCompareIconEntry>() );

	if ( itFound != m_frames.end() && itFound->first == entry )
		return std::distance( m_frames.begin(), itFound );

	return utl::npos;
}

CIconGroup::TIconPair* CIconGroup::FindEntry( const ui::CIconEntry& entry ) const
{
	size_t foundPos = FindPos( entry );
	return foundPos != utl::npos ? const_cast<CIconGroup::TIconPair*>( &m_frames[ foundPos ] ) : nullptr;
}

CIcon* CIconGroup::FindIcon( const ui::CIconEntry& entry ) const
{
	TIconPair* pFoundPair = FindEntry( entry );
	return pFoundPair != nullptr ? pFoundPair->second : nullptr;
}

size_t CIconGroup::FindBestMatchingPos( IconStdSize iconStdSize ) const
{
	const ui::CIconEntry entry( ILC_COLOR32, iconStdSize );
	std::vector<TIconPair>::const_iterator itFound = std::lower_bound( m_frames.begin(), m_frames.end(), TIconPair( entry, nullptr ), pred::LessValue<pred::TCompareIconEntry>());

	if ( itFound != m_frames.end() )		// first match is the best match (same iconStdSize, maximum BitsPerPixel)
		return std::distance( m_frames.begin(), itFound );

	return utl::npos;
}

CIcon* CIconGroup::FindBestMatchingIcon( IconStdSize iconStdSize ) const
{
	size_t foundPos = FindBestMatchingPos( iconStdSize );
	return foundPos != utl::npos ? m_frames[foundPos].second : nullptr;
}

const CIconGroup::TIconPair* CIconGroup::FindBestMatchingPair( IconStdSize iconStdSize ) const
{
	size_t foundPos = FindBestMatchingPos( iconStdSize );
	return foundPos != utl::npos ? &m_frames[ foundPos ] : nullptr;
}
