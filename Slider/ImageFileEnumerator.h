#ifndef ImageFileEnumerator_h
#define ImageFileEnumerator_h
#pragma once

#include "utl/FileSystem.h"
#include "utl/Range.h"
#include "utl/UI/UserReport.h"
#include "Application_fwd.h"


class CFileAttr;
class CSearchSpec;


class CImageFileEnumerator : private fs::IEnumerator, private utl::noncopyable
{
public:
	CImageFileEnumerator( const Range< size_t >& fileSizeRange = Range< size_t >( 0, UINT_MAX ) );
	~CImageFileEnumerator();

	void Search( const std::vector< CSearchSpec >& searchSpecs ) throws_( CException* );
	void Search( const CSearchSpec& searchSpec ) throws_( CException* );
	void SearchImageArchive( const fs::CPath& stgDocPath ) throws_( CException* );

	// found files
	bool AnyFound( void ) const { return !m_fileAttrs.empty(); }
	const std::vector< CFileAttr >& GetFileAttrs( void ) const { return m_fileAttrs; }
	const std::vector< fs::CPath >& GetArchiveStgPaths( void ) const { return m_archiveStgPaths; }
	void SwapResults( std::vector< CFileAttr >& rFileAttrs, std::vector< fs::CPath >* pArchiveStgPaths = NULL );

	const ui::CIssueStore& GetIssueStore( void ) const { return m_issueStore; }
private:
	// fs::IEnumerator interface (files only)
	virtual void AddFoundFile( const TCHAR* pFilePath );
	virtual void AddFoundSubDir( const TCHAR* pSubDirPath );
	virtual void AddFile( const CFileFind& foundFile );

	bool PassFilter( const CFileFind& foundFile ) const;
	void Push( const CFileAttr& fileAttr );
	void PushMany( const std::vector< CFileAttr >& fileAttrs );
private:
	Range< size_t > m_fileSizeRange;
	ui::CIssueStore m_issueStore;
	std::auto_ptr< app::CScopedProgress > m_pProgress;
	const CSearchSpec* m_pCurrSpec;

	// found
	std::vector< CFileAttr > m_fileAttrs;
	std::vector< fs::CPath > m_archiveStgPaths;
};


#endif // ImageFileEnumerator_h
