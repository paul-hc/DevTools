
#include "stdafx.h"
#include "SearchModel.h"
#include "SearchSpec.h"
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
	ClearSpecs();
}

CSearchModel& CSearchModel::operator=( const CSearchModel& right )
{
	if ( &right != this )
	{
		m_maxFileCount = right.m_maxFileCount;
		m_fileSizeRange = right.m_fileSizeRange;
		utl::CloneOwningContainerObjects( m_searchSpecs, right.m_searchSpecs );
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

	serial::StreamOwningPtrs( archive, m_searchSpecs );
}

void CSearchModel::ClearSpecs( void )
{
	utl::ClearOwningContainer( m_searchSpecs );
}

void CSearchModel::AddSpec( CSearchSpec* pSearchSpec, size_t pos /*= utl::npos*/ )
{
	m_searchSpecs.insert( m_searchSpecs.begin() + ( pos != utl::npos ? pos : m_searchSpecs.size() ), pSearchSpec );
}

void CSearchModel::AddSearchPath( const fs::CPath& searchPath, size_t pos /*= utl::npos*/ )
{
	AddSpec( new CSearchSpec( searchPath ), pos );
}

std::auto_ptr< CSearchSpec > CSearchModel::RemoveSpecAt( size_t pos )
{
	ASSERT( pos < m_searchSpecs.size() );

	std::auto_ptr< CSearchSpec > pSearchSpec( m_searchSpecs[ pos ] );

	m_searchSpecs.erase( m_searchSpecs.begin() + pos );
	return pSearchSpec;
}

int CSearchModel::FindSpecPos( const fs::CPath& searchPath ) const
{
	for ( std::vector< CSearchSpec* >::const_iterator itSearch = m_searchSpecs.begin(); itSearch != m_searchSpecs.end(); ++itSearch )
		if ( ( *itSearch )->GetFilePath() == searchPath )
			return static_cast< int >( std::distance( m_searchSpecs.begin(), itSearch ) );

	return -1;
}
