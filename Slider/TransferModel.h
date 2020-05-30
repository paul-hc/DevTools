#ifndef TransferModel_h
#define TransferModel_h
#pragma once

#include "ArchivingModel_fwd.h"
#include "FileAttr.h"


class CTransferFileAttr;

namespace ui
{
	interface IProgressService;
	interface IUserReport;
}


class CTransferModel
{
public:
	CTransferModel( void ) {}
	~CTransferModel();

	void Build( const std::vector< TTransferPathPair >& xferPairs, ui::IProgressService* pProgressService, ui::IUserReport* pUserReport );

	bool IsEmpty( void ) const { return m_fileAttribs.empty(); }
	const std::vector< CTransferFileAttr* >& GetFileAttribs( void ) const { return m_fileAttribs; }
	std::vector< CTransferFileAttr* >& RefFileAttribs( void ) { return m_fileAttribs; }
private:
	std::vector< CTransferFileAttr* > m_fileAttribs;
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
	fs::CFlexPath m_srcImagePath;			// transfer SRC
};


#endif // TransferModel_h
