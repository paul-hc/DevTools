#ifndef SearchModel_h
#define SearchModel_h
#pragma once

#include "utl/Path_fwd.h"
#include "utl/Range.h"


class CSearchPattern;
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

	void AugmentStoragePaths( std::vector<fs::TStgDocPath>& rStoragePaths ) const;

	UINT GetMaxFileCount( void ) const { return m_maxFileCount; }
	void SetMaxFileCount( UINT maxFileCount ) { m_maxFileCount = maxFileCount; }

	const Range<UINT>& GetFileSizeRange( void ) const { return m_fileSizeRange; }
	void SetFileSizeRange( const Range<UINT>& fileSizeRange ) { m_fileSizeRange = fileSizeRange; }

	const std::vector<CSearchPattern*>& GetPatterns( void ) const { return m_patterns; }
	std::vector<CSearchPattern*>& RefPatterns( void ) { return m_patterns; }

	CSearchPattern* GetPatternAt( size_t pos ) const { ASSERT( pos < m_patterns.size() ); return m_patterns[ pos ]; }
	size_t AddPattern( CSearchPattern* pPattern, size_t pos = utl::npos );
	std::pair<CSearchPattern*, bool> AddSearchPath( const fs::CPath& searchPath, size_t pos = utl::npos );		// returns the existing pattern if already exists
	std::auto_ptr<CSearchPattern> RemovePatternAt( size_t pos );
	void ClearPatterns( void );
	size_t FindPatternPos( const fs::CPath& searchPath, size_t ignorePos = utl::npos ) const;

	bool IsEmpty( void ) const { return m_patterns.empty(); }
	bool IsSinglePattern( void ) const { return 1 == m_patterns.size(); }
	const CSearchPattern* GetSinglePattern( void ) const { return IsSinglePattern() ? m_patterns.front() : nullptr; }
	CSearchPattern* RefSinglePattern( void ) { ASSERT( 1 == m_patterns.size() ); return m_patterns.front(); }
private:
	persist UINT m_maxFileCount;							// max count filter of found files (Slider_v4_2 +)
	persist Range<UINT> m_fileSizeRange;					// size filter for found files (UINT for 32/64 bit portability)
	persist std::vector<CSearchPattern*> m_patterns;		// search patterns used for searching the file list
public:
	static const Range<UINT> s_anyFileSizeRange;			// no file size filtering
};


#endif // SearchModel_h
