
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

void CSearchModel::ClearPatterns( void )
{
	utl::ClearOwningContainer( m_patterns );
}

void CSearchModel::AddPattern( CSearchPattern* pPattern, size_t pos /*= utl::npos*/ )
{
	m_patterns.insert( m_patterns.begin() + ( pos != utl::npos ? pos : m_patterns.size() ), pPattern );
}

void CSearchModel::AddSearchPath( const fs::CPath& searchPath, size_t pos /*= utl::npos*/ )
{
	AddPattern( new CSearchPattern( searchPath ), pos );
}

std::auto_ptr< CSearchPattern > CSearchModel::RemovePatternAt( size_t pos )
{
	ASSERT( pos < m_patterns.size() );

	std::auto_ptr< CSearchPattern > pPattern( m_patterns[ pos ] );

	m_patterns.erase( m_patterns.begin() + pos );
	return pPattern;
}

int CSearchModel::FindPatternPos( const fs::CPath& searchPath ) const
{
	for ( std::vector< CSearchPattern* >::const_iterator itSearch = m_patterns.begin(); itSearch != m_patterns.end(); ++itSearch )
		if ( ( *itSearch )->GetFilePath() == searchPath )
			return static_cast< int >( std::distance( m_patterns.begin(), itSearch ) );

	return -1;
}
