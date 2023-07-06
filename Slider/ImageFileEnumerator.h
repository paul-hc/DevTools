#ifndef ImageFileEnumerator_h
#define ImageFileEnumerator_h
#pragma once

#include "utl/FileEnumerator.h"
#include "utl/UI/UserReport.h"
#include "ImagesModel.h"


class CSearchPattern;
class CFileAttr;


class CImageFileEnumerator : public fs::CPathEnumerator
{
public:
	CImageFileEnumerator( IEnumerator* pProgressEnum = nullptr );
	~CImageFileEnumerator();

	void Search( const std::vector<CSearchPattern*>& searchPatterns ) throws_( CException*, CUserAbortedException );
	void Search( const CSearchPattern& searchPattern ) throws_( CException*, CUserAbortedException );
	void SearchCatalogStorage( const fs::TStgDocPath& docStgPath ) throws_( CException*, CUserAbortedException );

	// found files
	bool AnyFound( void ) const { return !m_foundImages.IsEmpty(); }
	void SwapFoundImages( CImagesModel& rImagesModel );

	const ui::CIssueStore& GetIssueStore( void ) const { return m_issueStore; }

	// base overrides
	virtual size_t GetFileCount( void ) const override { return m_foundImages.GetFileAttrs().size(); }
	virtual void Clear( void ) override;
private:
	// fs::IEnumerator interface (files only)
	virtual void OnAddFileInfo( const fs::CFileState& fileState ) override;
	virtual void AddFoundFile( const fs::CPath& filePath ) override;

	bool PassFilter( const CFileAttr& fileAttr ) const;
	bool Push( CFileAttr* pFileAttr );
	void PushMany( const std::vector<CFileAttr*>& fileAttrs );		// transfer ownership
private:
	ui::CIssueStore m_issueStore;
	const CSearchPattern* m_pCurrPattern;

	CImagesModel m_foundImages;
public:
	static const Range<size_t> s_allFileSizesRange;
};


#endif // ImageFileEnumerator_h
