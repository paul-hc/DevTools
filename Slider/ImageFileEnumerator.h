#ifndef ImageFileEnumerator_h
#define ImageFileEnumerator_h
#pragma once

#include "utl/FileEnumerator.h"
#include "utl/Range.h"
#include "utl/UI/UserReport.h"
#include "ImagesModel.h"


class CSearchSpec;
class CFileAttr;


class CImageFileEnumerator : public fs::CEnumerator
{
public:
	CImageFileEnumerator( IEnumerator* pProgressEnum = NULL );
	~CImageFileEnumerator();

	void SetFileSizeFilter( const Range< size_t >& fileSizeRange ) { m_fileSizeRange = fileSizeRange; ENSURE( m_fileSizeRange.IsNormalized() ); }

	void Search( const std::vector< CSearchSpec* >& searchSpecs ) throws_( CException*, CUserAbortedException );
	void Search( const CSearchSpec& searchSpec ) throws_( CException*, CUserAbortedException );
	void SearchImageArchive( const fs::CPath& stgDocPath ) throws_( CException*, CUserAbortedException );

	// found files
	bool AnyFound( void ) const { return !m_foundImages.IsEmpty(); }
	void SwapFoundImages( CImagesModel& rImagesModel );

	const ui::CIssueStore& GetIssueStore( void ) const { return m_issueStore; }
private:
	// fs::IEnumerator interface (files only)
	virtual void AddFoundFile( const TCHAR* pFilePath );
	virtual void AddFile( const CFileFind& foundFile );
	virtual bool MustStop( void ) const;

	bool PassFilter( const CFileAttr& fileAttr ) const;
	void Push( CFileAttr* pFileAttr );
	void PushMany( const std::vector< CFileAttr* >& fileAttrs );		// transfer ownership
private:
	Range< size_t > m_fileSizeRange;
	ui::CIssueStore m_issueStore;
	const CSearchSpec* m_pCurrSpec;

	CImagesModel m_foundImages;
public:
	static const Range< size_t > s_allFileSizesRange;
};


#endif // ImageFileEnumerator_h
