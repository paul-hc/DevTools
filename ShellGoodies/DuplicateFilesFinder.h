#ifndef DuplicateFilesFinder_h
#define DuplicateFilesFinder_h
#pragma once

#include "utl/Timer.h"
#include "DuplicateFileItem.h"


struct CDupsOutcome
{
	CDupsOutcome( void ) : m_searchedDirCount( 0 ), m_foundFileCount( 0 ), m_ignoredCount( 0 ) {}
public:
	CTimer m_timer;
	size_t m_searchedDirCount;
	size_t m_foundFileCount;
	size_t m_ignoredCount;
};


class CDuplicateFilesFinder
{
public:
	CDuplicateFilesFinder( void );		// for testing
	CDuplicateFilesFinder( utl::IProgressService* pProgressSvc, fs::IEnumerator* pProgressEnum );

	void SetWildSpec( const std::tstring& wildSpec ) { m_wildSpec = wildSpec; }
	void SetMinFileSize( UINT64 minFileSize ) { m_minFileSize = minFileSize; }

	const CDupsOutcome& GetOutcome( void ) const { return m_outcome; }

	void FindDuplicates( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups,
						 const std::vector< fs::CPath >& searchPaths,
						 const std::vector< fs::CPath >& ignorePaths ) throws_( CUserAbortedException );
private:
	void SearchForFiles( std::vector< fs::CPath >& rFoundPaths,
						 const std::vector< fs::CPath >& searchPaths,
						 const std::vector< fs::CPath >& ignorePaths );
	void GroupByFileSize( CDuplicateGroupStore* pGroupsStore, const std::vector< fs::CPath >& foundPaths );
	void GroupByCrc32( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, CDuplicateGroupStore* pGroupsStore );

	void ProgSection_GroupByFileSize( size_t fileCount ) const;
	void ProgSection_GroupByCrc32( size_t itemCount ) const;
private:
	utl::IProgressService* m_pProgressSvc;
	fs::IEnumerator* m_pProgressEnum;

	std::tstring m_wildSpec;
	UINT64 m_minFileSize;

	CDupsOutcome m_outcome;
};


#endif // DuplicateFilesFinder_h
