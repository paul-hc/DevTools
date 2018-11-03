#ifndef DuplicateFilesFinder_h
#define DuplicateFilesFinder_h
#pragma once

#include "utl/FileSystem_fwd.h"
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
	CDuplicateFilesFinder( const std::tstring& wildSpec, ULONGLONG minFileSize ) : m_wildSpec( wildSpec ), m_minFileSize( minFileSize ) {}

	const CDupsOutcome& GetOutcome( void ) const { return m_outcome; }

	void FindDuplicates( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, const std::vector< CSrcPathItem* >& srcPathItems, CWnd* pParent ) throws_( CUserAbortedException );
private:
	void SearchForFiles( std::vector< fs::CPath >& rFoundPaths, const std::vector< CSrcPathItem* >& srcPathItems, fs::IEnumerator* pProgressEnum );
	void GroupByFileSize( CDuplicateGroupStore* pGroupsStore, const std::vector< fs::CPath >& foundPaths, ui::IProgressCallback* pProgress );
	void GroupByCrc32( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, CDuplicateGroupStore* pGroupsStore, ui::IProgressCallback* pProgress );
private:
	std::tstring m_wildSpec;
	ULONGLONG m_minFileSize;

	CDupsOutcome m_outcome;
};


#include "utl/ProgressDialog.h"


class CDuplicatesProgress : private fs::IEnumerator
						  , private utl::noncopyable
{
public:
	CDuplicatesProgress( CWnd* pParent );
	~CDuplicatesProgress();

	ui::IProgressCallback* GetProgress( void ) { return &m_dlg; }
	fs::IEnumerator* GetProgressEnumerator( void ) { return this; }

	void Section_GroupByFileSize( size_t fileCount );
	void Section_GroupByCrc32( size_t itemCount );

	// file enumerator callback
	virtual void AddFoundFile( const TCHAR* pFilePath ) throws_( CUserAbortedException );
	virtual void AddFoundSubDir( const TCHAR* pSubDirPath ) throws_( CUserAbortedException );
private:
	CProgressDialog m_dlg;
};


#endif // DuplicateFilesFinder_h
