
#include "stdafx.h"
#include "SearchModel.h"
#include "SearchPattern.h"
#include "ModelSchema.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const Range< UINT > CSearchModel::s_anyFileSizeRange( 0, UINT_MAX );

CSearchModel::CSearchModel( void )
	: m_maxFileCount( UINT_MAX )
	, m_fileSizeRange( s_anyFileSizeRange )
{
}

CSearchModel::~CSearchModel()
{
	ClearPatterns();
}

CSearchModel& CSearchModel::operator=( const CSearchModel& right )
{
	if ( &right != this )
	{
		m_maxFileCount = right.m_maxFileCount;
		m_fileSizeRange = right.m_fileSizeRange;
		utl::CloneOwningContainerObjects( m_patterns, right.m_patterns );
	}
	return *this;
}

void CSearchModel::Stream( CArchive& archive )
{
	if ( archive.IsStoring() )
	{
		archive << m_maxFileCount;
		archive << m_fileSizeRange;
	}
	else
	{
		ASSERT( app::GetLoadingSchema( archive ) >= app::Slider_v4_2 );		// newer encapsulated doc version?

		archive >> m_maxFileCount;
		archive >> m_fileSizeRange;
	}

	serial::StreamOwningPtrs( archive, m_patterns );
}

void CSearchModel::AugmentStoragePaths( std::vector< fs::TStgDocPath >& rStoragePaths ) const
{
	for ( std::vector< CSearchPattern* >::const_iterator itPattern = m_patterns.begin(); itPattern != m_patterns.end(); ++itPattern )
		if ( ( *itPattern )->IsStorageAlbumFile() )
			utl::AddUnique( rStoragePaths, ( *itPattern )->GetFilePath() );		// unique augmentation to prevent pushing duplicates in CAlbumModel::m_storageHost
}

void CSearchModel::ClearPatterns( void )
{
	utl::ClearOwningContainer( m_patterns );
}

size_t CSearchModel::AddPattern( CSearchPattern* pPattern, size_t atPos /*= utl::npos*/ )
{
	ASSERT_PTR( pPattern );
	REQUIRE( utl::npos == FindPatternPos( pPattern->GetFilePath() ) );		// must be unique

	if ( utl::npos == atPos )
		atPos = m_patterns.size();

	m_patterns.insert( m_patterns.begin() + atPos, pPattern );
	return atPos;
}

std::pair< CSearchPattern*, bool > CSearchModel::AddSearchPath( const fs::CPath& searchPath, size_t pos /*= utl::npos*/ )
{
	size_t foundPos = FindPatternPos( searchPath );
	if ( foundPos != utl::npos )
		return std::pair< CSearchPattern*, bool >( m_patterns[ foundPos ], false );			// pattern with path key already exists

	CSearchPattern* pNewPattern = new CSearchPattern( searchPath );

	if ( !pNewPattern->IsValidPath() )
	{
		delete pNewPattern;
		return std::pair< CSearchPattern*, bool >( NULL, false );			// could be a stray file of incompatible type
	}

	AddPattern( pNewPattern, pos );
	return std::pair< CSearchPattern*, bool >( pNewPattern, true );							// new pattern
}

std::auto_ptr< CSearchPattern > CSearchModel::RemovePatternAt( size_t pos )
{
	ASSERT( pos < m_patterns.size() );

	std::auto_ptr< CSearchPattern > pPattern( m_patterns[ pos ] );

	m_patterns.erase( m_patterns.begin() + pos );
	return pPattern;
}

size_t CSearchModel::FindPatternPos( const fs::CPath& searchPath, size_t ignorePos /*= utl::npos*/ ) const
{
	for ( size_t pos = 0; pos != m_patterns.size(); ++pos )
		if ( m_patterns[ pos ]->GetFilePath() == searchPath )
			if ( pos != ignorePos )
				return pos;

	return utl::npos;
}
