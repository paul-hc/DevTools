#ifndef ImageFileEnumerator_h
#define ImageFileEnumerator_h
#pragma once

#include "utl/FileSystem.h"
#include "utl/Range.h"
#include "utl/UI/UserReport.h"
#include "ModelSchema.h"


class CSearchSpec;
class CFileAttr;


class CImageFileEnumerator : public fs::CEnumerator
{
public:
	CImageFileEnumerator( IEnumerator* pProgressCallback = NULL );
	~CImageFileEnumerator();

	void SetFileSizeFilter( const Range< size_t >& fileSizeRange ) { m_fileSizeRange = fileSizeRange; ENSURE( m_fileSizeRange.IsNormalized() ); }

	void Search( const std::vector< CSearchSpec* >& searchSpecs ) throws_( CException*, CUserAbortedException );
	void Search( const CSearchSpec& searchSpec ) throws_( CException*, CUserAbortedException );
	void SearchImageArchive( const fs::CPath& stgDocPath ) throws_( CException*, CUserAbortedException );

	// found files
	bool AnyFound( void ) const { return !m_fileAttrs.empty(); }
	const std::vector< CFileAttr >& GetFileAttrs( void ) const { return m_fileAttrs; }
	const std::vector< fs::CPath >& GetArchiveStgPaths( void ) const { return m_archiveStgPaths; }
	void SwapResults( std::vector< CFileAttr >& rFileAttrs, std::vector< fs::CPath >* pArchiveStgPaths = NULL );

	const ui::CIssueStore& GetIssueStore( void ) const { return m_issueStore; }
private:
	// fs::IEnumerator interface (files only)
	virtual void AddFoundFile( const TCHAR* pFilePath );
	virtual void AddFile( const CFileFind& foundFile );
	virtual bool MustStop( void ) const;

	bool PassFilter( const CFileAttr& fileAttr ) const;
	void Push( const CFileAttr& fileAttr );
	void PushMany( const std::vector< CFileAttr >& fileAttrs );
private:
	Range< size_t > m_fileSizeRange;
	ui::CIssueStore m_issueStore;
	const CSearchSpec* m_pCurrSpec;

	// found
	std::vector< CFileAttr > m_fileAttrs;
	std::vector< fs::CPath > m_archiveStgPaths;
public:
	static const Range< size_t > s_allFileSizesRange;
};


#include "utl/UI/ProgressDialog.h"


class CImagesProgressCallback : private fs::IEnumerator
							  , private utl::noncopyable
{
public:
	CImagesProgressCallback( CWnd* pParent, const std::tstring& operationLabel = s_searching );
	~CImagesProgressCallback();

	ui::IProgressCallback* GetCallback( void ) { return &m_dlg; }
	fs::IEnumerator* GetProgressEnumerator( void ) { return this; }

	bool IsValidDialog( void ) const { return m_dlg.IsRunning(); }		// i.e. not aborted by user?
	void Section_OrderImageFiles( size_t fileCount );
private:
	// file enumerator callbacks
	virtual void AddFoundFile( const TCHAR* pFilePath ) throws_( CUserAbortedException );
	virtual bool AddFoundSubDir( const TCHAR* pSubDirPath ) throws_( CUserAbortedException );
private:
	CProgressDialog m_dlg;
public:
	static const std::tstring s_searching;
};


#endif // ImageFileEnumerator_h
