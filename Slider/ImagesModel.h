#ifndef ImagesModel_h
#define ImagesModel_h
#pragma once

#include "FileAttr_fwd.h"


namespace utl { interface IProgressService; }
namespace fs
{
	class CFlexPath;
	template< typename PathT > class CPathIndex;
}


class CImagesModel : private utl::noncopyable
{
public:
	CImagesModel( void );
	CImagesModel( const CImagesModel& right );
	~CImagesModel();

	CImagesModel& operator=( const CImagesModel& right );

	void Stream( CArchive& archive );

	void Clear( void );
	void StoreBaselineSequence( void );
	void SetUseIndexing( bool useIndexing = true );

	void Swap( CImagesModel& rImagesModel );

	bool IsEmpty( void ) const { return m_fileAttributes.empty(); }

	const std::vector< CFileAttr* >& GetFileAttrs( void ) const { return m_fileAttributes; }
	std::vector< CFileAttr* >& RefFileAttrs( void ) { return m_fileAttributes; }
	const CFileAttr* GetFileAttrAt( size_t pos ) const { ASSERT( pos < m_fileAttributes.size() ); return m_fileAttributes[ pos ]; }
	bool AddFileAttr( CFileAttr* pFileAttr );
	std::auto_ptr< CFileAttr > RemoveFileAttrAt( size_t pos );

	// file-path key lookup
	bool ContainsFileAttr( const fs::CFlexPath& filePath ) const;
	size_t FindPosFileAttr( const fs::CFlexPath& filePath ) const;
	const CFileAttr* FindFileAttr( const fs::CFlexPath& filePath ) const;

	// embedded storages
	const std::vector< fs::CPath >& GetStoragePaths( void ) const { return m_storagePaths; }
	std::vector< fs::CPath >& RefStoragePaths( void ) { return m_storagePaths; }
	bool AddStoragePath( const fs::CPath& storagePath );
	void ClearInvalidStoragePaths( void );
public:
	void OrderFileAttrs( fattr::Order fileOrder, utl::IProgressService* pProgressSvc );
private:
	void FilterFileDuplicates( fattr::Order fileOrder, utl::IProgressService* pProgressSvc, bool compareImageDim = false );
	void FilterCorruptFiles( utl::IProgressService* pProgressSvc );
private:
	persist std::vector< CFileAttr* > m_fileAttributes;			// owning container
	persist std::vector< fs::CPath > m_storagePaths;			// such as .ias files, storages found during search - the catalogs are managed by parent album model

	// transient
	std::auto_ptr< fs::CPathIndex< fs::CFlexPath > > m_pIndexer;
};


#endif // ImagesModel_h
