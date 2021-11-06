#ifndef CatalogStorageService_h
#define CatalogStorageService_h
#pragma once

#include "ArchivingModel_fwd.h"
#include "FileAttr.h"
#include "CatalogStorageHost.h"


class CAlbumDoc;
class CAlbumModel;
class CImagesModel;

namespace utl { interface IProgressService; }
namespace ui { interface IUserReport; }


namespace svc
{
	// decouple the dependency to AlbumDoc.h

	CAlbumModel* ToAlbumModel( CObject* pAlbumDoc );
	CImagesModel& ToImagesModel( CObject* pAlbumDoc );
}


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


abstract class CTransferAlbumService : private utl::noncopyable
{
protected:
	CTransferAlbumService( void );			// uses null-pattern, for testing
	CTransferAlbumService( utl::IProgressService* pProgressSvc, ui::IUserReport* pUserReport );
	~CTransferAlbumService();

	// overridables
	virtual void BuildTransferAttrs( const CImagesModel* pImagesModel, bool useDeepStreamPaths = true ) = 0;
	virtual void CloneDestAlbumDoc( const CAlbumDoc* pSrcAlbumDoc );
public:
	void BuildFromAlbumSaveAs( const CAlbumDoc* pSrcAlbumDoc );				// preserve SRC album, create a new image storage doc file

	bool IsEmpty( void ) const { return m_transferAttrs.empty(); }
	const std::vector< CTransferFileAttr* >& GetTransferAttrs( void ) const { return m_transferAttrs; }
	std::vector< CTransferFileAttr* >& RefTransferAttrs( void ) { return m_transferAttrs; }

	utl::IProgressService* GetProgress( void ) const { return m_pProgressSvc; }
	ui::IUserReport* GetReport( void ) const { return m_pUserReport; }
protected:
	utl::IProgressService* m_pProgressSvc;
	ui::IUserReport* m_pUserReport;

	std::vector< CTransferFileAttr* > m_transferAttrs;
	std::auto_ptr< CAlbumDoc > m_pDestAlbumDoc;		// created ad-hoc as destination model conversion album
	std::tstring m_password;						// used in catalog storages only

	static const TCHAR s_buildTag[];
};


// supports the creation of an image catalog storage (compound document)
class CCatalogStorageService : public CTransferAlbumService
{
public:
	CCatalogStorageService( void ) {}			// uses null-pattern, for testing
	CCatalogStorageService( utl::IProgressService* pProgressSvc, ui::IUserReport* pUserReport ) : CTransferAlbumService( pProgressSvc, pUserReport ) {}

	void BuildFromSrcPaths( const std::vector< fs::CPath >& srcImagePaths );				// for testing
	void BuildFromTransferPairs( const std::vector< TTransferPathPair >& xferPairs );		// legacy archiving model

	const std::tstring& GetPassword( void ) const { return m_password; }
	void SetPassword( const std::tstring& password ) { m_password = password; }

	CObject* GetAlbumDoc( void ) const { return (CObject*)m_pDestAlbumDoc.get(); }
protected:
	// base overrides
	virtual void BuildTransferAttrs( const CImagesModel* pImagesModel, bool useDeepStreamPaths = true );
private:
	std::vector< fs::TStgDocPath > m_srcDocStgPaths;
	CCatalogStorageHost m_srcStorageHost;			// open for reading during image archive construction
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
