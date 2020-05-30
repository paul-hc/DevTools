#ifndef ImageStorageModel_h
#define ImageStorageModel_h
#pragma once

#include "ArchivingModel_fwd.h"
#include "FileAttr.h"


class CTransferFileAttr;

namespace ui
{
	interface IProgressService;
	interface IUserReport;
}


class CImageStorageModel
{
public:
	CImageStorageModel( void ) {}
	~CImageStorageModel();

	void Build( const std::vector< TTransferPathPair >& xferPairs, ui::IProgressService* pProgressService, ui::IUserReport* pUserReport );

	bool IsEmpty( void ) const { return m_fileAttribs.empty(); }
	const std::vector< CTransferFileAttr* >& GetFileAttribs( void ) const { return m_fileAttribs; }
	std::vector< CTransferFileAttr* >& RefFileAttribs( void ) { return m_fileAttribs; }
private:
	std::vector< CTransferFileAttr* > m_fileAttribs;
	std::vector< fs::CPath > m_srcStorages;
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


#endif // ImageStorageModel_h
