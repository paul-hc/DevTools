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
	CDuplicateFilesFinder( const std::tstring& wildSpec, UINT64 minFileSize ) : m_wildSpec( wildSpec ), m_minFileSize( minFileSize ) {}

	const CDupsOutcome& GetOutcome( void ) const { return m_outcome; }

	void FindDuplicates( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups,
						 const std::vector< CPathItem* >& srcPathItems,
						 const std::vector< CPathItem* >& ignorePathItems,
						 CWnd* pParent ) throws_( CUserAbortedException );
private:
	void SearchForFiles( std::vector< fs::CPath >& rFoundPaths,
						 const std::vector< CPathItem* >& srcPathItems,
						 const std::vector< CPathItem* >& ignorePathItems,
						 fs::IEnumerator* pProgressEnum );
	void GroupByFileSize( CDuplicateGroupStore* pGroupsStore, const std::vector< fs::CPath >& foundPaths, ui::IProgressService* pProgress );
	void GroupByCrc32( std::vector< CDuplicateFilesGroup* >& rDuplicateGroups, CDuplicateGroupStore* pGroupsStore, ui::IProgressService* pProgress );
private:
	std::tstring m_wildSpec;
	UINT64 m_minFileSize;

	CDupsOutcome m_outcome;
};


#include "utl/UI/ProgressDialog.h"


class CDuplicatesProgressService : private fs::IEnumerator
								 , private utl::noncopyable
{
public:
	CDuplicatesProgressService( CWnd* pParent );
	~CDuplicatesProgressService();

	ui::IProgressService* GetService( void ) { return m_dlg.GetService(); }
	ui::IProgressHeader* GetHeader( void ) { return GetService()->GetHeader(); }
	fs::IEnumerator* GetProgressEnumerator( void ) { return this; }

	void Section_GroupByFileSize( size_t fileCount );
	void Section_GroupByCrc32( size_t itemCount );
private:
	// file enumerator callbacks
	virtual void OnAddFileInfo( const CFileFind& foundFile ) throws_( CUserAbortedException );
	virtual void AddFoundFile( const TCHAR* pFilePath ) throws_( CUserAbortedException );
	virtual bool AddFoundSubDir( const TCHAR* pSubDirPath ) throws_( CUserAbortedException );
private:
	CProgressDialog m_dlg;
	fs::CPath m_lastFilePath;
};


#endif // DuplicateFilesFinder_h
