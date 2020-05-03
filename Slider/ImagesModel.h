#ifndef ImagesModel_h
#define ImagesModel_h
#pragma once


class CFileAttr;


class CImagesModel : private utl::noncopyable
{
public:
	CImagesModel( void );
	~CImagesModel();

	void Stream( CArchive& archive );

	void Clear( void );
	void ReleaseStorages( void );
	void StoreBaselineSequence( void );

	bool IsEmpty( void ) const { return m_fileAttributes.empty(); }

	const std::vector< CFileAttr* >& GetFileAttrs( void ) const { return m_fileAttributes; }
	std::vector< CFileAttr* >& RefFileAttrs( void ) { return m_fileAttributes; }
	bool AddFileAttr( CFileAttr* pFileAttr );

	const CFileAttr* FindFileAttrWithPath( const fs::CPath& filePath ) const;

	const std::vector< fs::CPath >& GetStoragePaths( void ) const { return m_storagePaths; }
	bool AddStoragePath( const fs::CPath& storagePath );
private:
	persist std::vector< CFileAttr* > m_fileAttributes;			// owning container
	persist std::vector< fs::CPath > m_storagePaths;			// such as .ias files, storages found during search
};


#endif // ImagesModel_h
