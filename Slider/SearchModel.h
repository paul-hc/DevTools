#ifndef SearchModel_h
#define SearchModel_h
#pragma once

#include "utl/Range.h"


class CSearchSpec;
namespace app { enum ModelSchema; }


class CSearchModel : private utl::noncopyable
{
	friend class CAlbumModel;		// for backwards compatible data-member loading
public:
	CSearchModel( void );
	CSearchModel( const CSearchModel& right ) { operator=( right ); }
	~CSearchModel();

	CSearchModel& operator=( const CSearchModel& right );

	void Stream( CArchive& archive );

	UINT GetMaxFileCount( void ) const { return m_maxFileCount; }
	void SetMaxFileCount( UINT maxFileCount ) { m_maxFileCount = maxFileCount; }

	const Range< UINT >& GetFileSizeRange( void ) const { return m_fileSizeRange; }
	void SetFileSizeRange( const Range< UINT >& fileSizeRange ) { m_fileSizeRange = fileSizeRange; }

	const std::vector< CSearchSpec* >& GetSpecs( void ) const { return m_searchSpecs; }
	std::vector< CSearchSpec* >& RefSpecs( void ) { return m_searchSpecs; }

	CSearchSpec* GetSpecAt( size_t pos ) const { ASSERT( pos < m_searchSpecs.size() ); return m_searchSpecs[ pos ]; }
	void AddSpec( CSearchSpec* pSearchSpec, size_t pos = utl::npos );
	void AddSearchPath( const fs::CPath& searchPath, size_t pos = utl::npos );
	std::auto_ptr< CSearchSpec > RemoveSpecAt( size_t pos );
	void ClearSpecs( void );
	int FindSpecPos( const fs::CPath& searchPath ) const;

	bool IsSingleSpec( void ) const { return 1 == m_searchSpecs.size(); }
	const CSearchSpec* GetSingleSpec( void ) const { return IsSingleSpec() ? m_searchSpecs.front() : NULL; }
	CSearchSpec* RefSingleSpec( void ) { ASSERT( 1 == m_searchSpecs.size() ); return m_searchSpecs.front(); }
private:
	persist UINT m_maxFileCount;							// max count filter of found files (Slider_v4_2 +)
	persist Range< UINT > m_fileSizeRange;					// size filter for found files (UINT for 32/64 bit portability)
	persist std::vector< CSearchSpec* > m_searchSpecs;		// search specifiers used for searching the file list
public:
	static const Range< UINT > s_anyFileSizeRange;			// no file size filtering
};


#endif // SearchModel_h
