#ifndef ImagesModel_h
#define ImagesModel_h
#pragma once

#include "FileAttr_fwd.h"


namespace ui { interface IProgressService; }


class CImagesModel : private utl::noncopyable
{
public:
	CImagesModel( void );
	CImagesModel( const CImagesModel& right ) { operator=( right ); }
	~CImagesModel();

	CImagesModel& operator=( const CImagesModel& right );

	void Stream( CArchive& archive );

	void Clear( void );
	void Swap( CImagesModel& rImagesModel );
	void StoreBaselineSequence( void );

	void ReleaseStorages( void );
	void AcquireStorages( void ) const;		// to enable image caching

	bool IsEmpty( void ) const { return m_fileAttributes.empty(); }

	const std::vector< CFileAttr* >& GetFileAttrs( void ) const { return m_fileAttributes; }
	std::vector< CFileAttr* >& RefFileAttrs( void ) { return m_fileAttributes; }
	const CFileAttr* GetFileAttrAt( size_t pos ) const { ASSERT( pos < m_fileAttributes.size() ); return m_fileAttributes[ pos ]; }
	bool AddFileAttr( CFileAttr* pFileAttr );

	const CFileAttr* FindFileAttrWithPath( const fs::CPath& filePath ) const;		// file-path key lookup

	std::vector< fs::CPath >& RefStoragePaths( void ) { return m_storagePaths; }
	bool AddStoragePath( const fs::CPath& storagePath );
public:
	void OrderFileAttrs( fattr::Order fileOrder, ui::IProgressService* pProgressSvc );
private:
	void FilterFileDuplicates( fattr::Order fileOrder, ui::IProgressService* pProgressSvc, bool compareImageDim = false );
	void FilterCorruptFiles( ui::IProgressService* pProgressSvc );
private:
	persist std::vector< CFileAttr* > m_fileAttributes;			// owning container
	persist std::vector< fs::CPath > m_storagePaths;			// such as .ias files, storages found during search
};


#endif // ImagesModel_h
