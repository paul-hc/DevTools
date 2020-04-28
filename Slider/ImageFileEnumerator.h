#ifndef ImageFileEnumerator_h
#define ImageFileEnumerator_h
#pragma once

#include "utl/FileSystem.h"
#include "utl/Range.h"
#include "utl/UI/UserReport.h"
#include "ModelSchema.h"

#include "Application_fwd.h"		// TEMP: for app::CScopedProgress


class CSearchSpec;
class CFileAttr;


class CImageFileEnumerator : private fs::IEnumerator
						   , private utl::noncopyable
{
public:
	CImageFileEnumerator( void );
	~CImageFileEnumerator();

	void SetMaxFileCountFilter( size_t maxFileCount ) { m_maxFileCount = maxFileCount; }
	void SetFileSizeFilter( const Range< size_t >& fileSizeRange ) { m_fileSizeRange = fileSizeRange; ENSURE( m_fileSizeRange.IsNormalized() ); }

	void Search( const CSearchSpec& searchSpec ) throws_( CException* );

//	private:
	void Search( const std::vector< CSearchSpec* >& searchSpecs ) throws_( CException* );
	public:
//	private:
	void SearchImageArchive( const fs::CPath& stgDocPath ) throws_( CException* );
	public:

	// found files
	bool AnyFound( void ) const { return !m_fileAttrs.empty(); }
	const std::vector< CFileAttr >& GetFileAttrs( void ) const { return m_fileAttrs; }
	const std::vector< fs::CPath >& GetArchiveStgPaths( void ) const { return m_archiveStgPaths; }
	void SwapResults( std::vector< CFileAttr >& rFileAttrs, std::vector< fs::CPath >* pArchiveStgPaths = NULL );

	const ui::CIssueStore& GetIssueStore( void ) const { return m_issueStore; }
private:
	// fs::IEnumerator interface (files only)
	virtual void AddFoundFile( const TCHAR* pFilePath );
	virtual bool AddFoundSubDir( const TCHAR* pSubDirPath );
	virtual void AddFile( const CFileFind& foundFile );

	bool PassFilter( const CFileFind& foundFile ) const;
	void Push( const CFileAttr& fileAttr );
	void PushMany( const std::vector< CFileAttr >& fileAttrs );
private:
	size_t m_maxFileCount;
	Range< size_t > m_fileSizeRange;
	ui::CIssueStore m_issueStore;
	std::auto_ptr< app::CScopedProgress > m_pProgress;
	const CSearchSpec* m_pCurrSpec;

	// found
	std::vector< CFileAttr > m_fileAttrs;
	std::vector< fs::CPath > m_archiveStgPaths;
public:
	static const Range< size_t > s_allFileSizesRange;
};


class CImageDiscoverer
{
public:
	CImageDiscoverer( void );
	~CImageDiscoverer();
private:
};


#endif // ImageFileEnumerator_h
