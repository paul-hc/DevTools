#ifndef CatalogStorageService_h
#define CatalogStorageService_h
#pragma once

#include "ArchivingModel_fwd.h"
#include "FileAttr.h"
#include "CatalogStorageHost.h"


class CAlbumDoc;
class CImagesModel;
class CTransferFileAttr;

namespace ui
{
	interface IProgressService;
	interface IUserReport;
}


// supports the creation of an image archive compound document
class CCatalogStorageService : private utl::noncopyable
{
public:
	CCatalogStorageService( void );			// using null-pattern, for testing
	CCatalogStorageService( ui::IProgressService* pProgressSvc, ui::IUserReport* pUserReport );
	~CCatalogStorageService();

	void BuildFromAlbumSaveAs( const CAlbumDoc* pSrcAlbumDoc );								// preserve SRC album, create a new image storage doc file
	void BuildFromSrcPaths( const std::vector< fs::CPath >& srcImagePaths );				// for testing
	void BuildFromTransferPairs( const std::vector< TTransferPathPair >& xferPairs );		// legacy archiving model

	bool IsEmpty( void ) const { return m_transferAttrs.empty(); }
	const std::vector< CTransferFileAttr* >& GetTransferAttrs( void ) const { return m_transferAttrs; }
	std::vector< CTransferFileAttr* >& RefTransferAttrs( void ) { return m_transferAttrs; }

	const std::tstring& GetPassword( void ) const { return m_password; }
	void SetPassword( const std::tstring& password ) { m_password = password; }

	CObject* GetAlbumDoc( void ) const { return m_pAlbumDoc; }

	ui::IProgressService* GetProgress( void ) const { return m_pProgressSvc; }
	ui::IUserReport* GetReport( void ) const { return m_pUserReport; }
private:
	void BuildTransferAttrs( const CImagesModel* pImagesModel, bool useDeepStreamPaths = true );
	CAlbumDoc* CloneDestAlbumDoc( const CAlbumDoc* pSrcAlbumDoc );
private:
	ui::IProgressService* m_pProgressSvc;
	ui::IUserReport* m_pUserReport;

	std::tstring m_password;
	std::vector< CTransferFileAttr* > m_transferAttrs;

	std::vector< fs::CPath > m_srcDocStgPaths;
	CCatalogStorageHost m_srcStorageHost;			// open for reading during image archive construction

	CObject* m_pAlbumDoc;
	bool m_isManagedAlbum;							// created internally when CREATING the image archive; passed externally when SAVING the image archive

	static const TCHAR s_buildTag[];
};


class CTransferFileAttr : public CFileAttr
{
public:
	CTransferFileAttr( const TTransferPathPair& pathPair );
	CTransferFileAttr( const CFileAttr& srcFileAttr, const fs::TEmbeddedPath& destImagePath );
	virtual ~CTransferFileAttr();

	const fs::CFlexPath& GetSrcImagePath( void ) const { return m_srcImagePath; }
private:
	fs::CFlexPath m_srcImagePath;			// SRC image path in transfer
};


namespace pwd
{
	std::tstring ToEncrypted( const std::tstring& displayPassword );
	std::tstring ToDecrypted( const std::tstring& encryptedPassword );


	inline std::tstring ToString( const char* pAnsiPwd )
	{
		// straight conversion through unsigned char - not using MultiByteToWideChar()
		const unsigned char* pAnsi = (const unsigned char*)pAnsiPwd;			// (!) cast to unsigned char for proper negative sign chars
		return std::tstring( pAnsi, pAnsi + str::GetLength( pAnsiPwd ) );
	}

	inline std::tstring ToString( const wchar_t* pWidePwd )
	{
		return std::tstring( pWidePwd );
	}
}


#endif // CatalogStorageService_h
