#ifndef ImageStorageService_h
#define ImageStorageService_h
#pragma once

#include "ArchivingModel_fwd.h"
#include "FileAttr.h"


class CTransferFileAttr;

namespace ui
{
	interface IProgressService;
	interface IUserReport;
}


class CImageStorageService : private utl::noncopyable
{
public:
	CImageStorageService( void );			// using null-pattern, for testing
	CImageStorageService( ui::IProgressService* pProgressSvc, ui::IUserReport* pUserReport );
	~CImageStorageService();

	void Build( const std::vector< TTransferPathPair >& xferPairs );

	// for testing
	void BuildFromSrcPaths( const std::vector< fs::CPath >& srcImagePaths );
	void MakeAlbumFileAttrs( std::vector< CFileAttr* >& rFileAttrs ) const;

	bool IsEmpty( void ) const { return m_transferAttrs.empty(); }
	const std::vector< CTransferFileAttr* >& GetTransferAttrs( void ) const { return m_transferAttrs; }
	std::vector< CTransferFileAttr* >& RefTransferAttrs( void ) { return m_transferAttrs; }

	const std::tstring& GetPassword( void ) const { return m_password; }
	void SetPassword( const std::tstring& password ) { m_password = password; }

	ui::IProgressService* GetProgress( void ) const { return m_pProgressSvc; }
	ui::IUserReport* GetReport( void ) const { return m_pUserReport; }
private:
	ui::IProgressService* m_pProgressSvc;
	ui::IUserReport* m_pUserReport;

	std::vector< CTransferFileAttr* > m_transferAttrs;
	std::vector< fs::CPath > m_srcStorages;
	std::tstring m_password;
};


class CTransferFileAttr : public CFileAttr
{
public:
	CTransferFileAttr( const TTransferPathPair& pathPair )
		: CFileAttr( pathPair.first )		// inherit SRC file attribute
		, m_srcImagePath( pathPair.first )
	{
		GetImageDim();						// cache dimensions via image loading
		SetPathKey( fs::ImagePathKey( pathPair.second, 0 ) );		// change to DEST path for transfer
	}

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


#endif // ImageStorageService_h
