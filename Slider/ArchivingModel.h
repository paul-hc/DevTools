#ifndef ArchivingModel_h
#define ArchivingModel_h
#pragma once

#include "ArchivingModel_fwd.h"


class CAlbumModel;


class CArchivingModel
{
public:
	CArchivingModel( void );
	~CArchivingModel();

	void Stream( CArchive& archive );

	void StorePassword( const std::tstring& password ) { m_password = password; }

	// piece-meal generation
	const std::vector< TTransferPathPair >& GetPathPairs( void ) const { return m_pathPairs; }
	void SetupSourcePaths( const std::vector< CFileAttr* >& srcFiles );
	void ResetDestPaths( void );

	static bool IsValidFormat( const std::tstring& format );
	bool GenerateDestPaths( const fs::CPath& destPath, const std::tstring& format, UINT* pSeqCount, bool forceShallowStreamNames = false );

	// service
	bool CreateArchiveStgFile( CAlbumModel* pModel, const fs::CPath& destStgPath );		// simple archive storage creation
	bool BuildArchiveStorageFile( const fs::CPath& destStgPath, FileOp fileOp, CWnd* pParentWnd = AfxGetMainWnd() ) const;

	bool CanCommitOperations( std::vector< TTransferPathPair >& rErrorPairs, FileOp fileOp, bool isUndoOp ) const;
	bool CommitOperations( FileOp fileOp, bool isUndoOp = false ) const;
private:
	static void CommitOperation( FileOp fileOp, const TTransferPathPair& xferPair, bool isUndoOp = false ) throws_( CException* );
private:
	persist std::vector< TTransferPathPair > m_pathPairs;

	// transient
	std::tstring m_password;		// encrypted
};


#endif // ArchivingModel_h
