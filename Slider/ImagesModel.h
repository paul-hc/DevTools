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

	void ClearFileAttrs( void );

	void StoreBaselineSequence( void );
private:
	persist std::vector< CFileAttr* > m_fileAttributes;			// owning container
	persist std::vector< fs::CPath > m_storagePaths;			// such as .ias files
};


#endif // ImagesModel_h
