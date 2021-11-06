#ifndef DuplicateFilesEnumerator_h
#define DuplicateFilesEnumerator_h
#pragma once

#include "DuplicateFileItem.h"
#include "FileEnumerator.h"
#include "IProgressService.h"
#include "Timer.h"


struct CDupsOutcome
{
	CDupsOutcome( void ) : m_foundSubDirCount( 0 ), m_foundFileCount( 0 ), m_ignoredCount( 0 ) {}
public:
	CTimer m_timer;
	size_t m_foundSubDirCount;
	size_t m_foundFileCount;
	size_t m_ignoredCount;
};


class CDuplicateFilesEnumerator : public fs::CBaseEnumerator
{
public:
	CDuplicateFilesEnumerator( fs::TEnumFlags enumFlags, IEnumerator* pChainEnum = NULL, utl::IProgressService* pProgressSvc = svc::CNoProgressService::Instance() );
	~CDuplicateFilesEnumerator() { Clear(); }

	void SearchDuplicates( const std::vector< fs::TPatternPath >& searchPaths );
	void SearchDuplicates( const fs::TPatternPath& searchPath ) { SearchDuplicates( std::vector< fs::TPatternPath >( 1, searchPath ) ); }

	const CDupsOutcome& GetOutcome( void ) const { return m_outcome; }

	// base overrides
	virtual void Clear( void );
	virtual size_t GetFileCount( void ) const { return m_outcome.m_foundFileCount; }
protected:
	// IEnumerator interface overrides
	virtual void OnAddFileInfo( const fs::CFileState& fileState );
	virtual void AddFoundFile( const TCHAR* pFilePath );
private:
	void GroupByCrc32( void );

	void ProgSection_GroupByCrc32( void ) const;
private:
	utl::IProgressService* m_pProgressSvc;
	CDupsOutcome m_outcome;

	// transient during search
	CDuplicateGroupStore* m_pGroupStore;
public:
	std::vector< CDuplicateFilesGroup* > m_dupGroupItems;		// results: groups sorted by path, each group's duplicate items sorted by path
};


#endif // DuplicateFilesEnumerator_h
