#ifndef ArchiveImagesContext_h
#define ArchiveImagesContext_h
#pragma once

#include "utl/FlexPath.h"
#include "ModelSchema.h"
#include <set>


class CEnumTags;
class CAlbumModel;
class CFileAttr;


// (*) don't change constant values
enum FileOp { FOP_FileCopy, FOP_FileMove, FOP_Reorder };
enum DestType { ToDirectory, ToArchiveStg };

const CEnumTags& GetTags_FileOp( void );


class CArchiveImagesContext
{
public:
	CArchiveImagesContext( void );
	~CArchiveImagesContext();

	void Stream( CArchive& archive );

	void StorePassword( const std::tstring& password ) { m_password = password; }

	// simple archive stg creation
	bool CreateArchiveStgFile( CAlbumModel* pModel, const fs::CPath& destStgPath );

	// piece-meal generation
	const std::vector< std::pair< fs::CFlexPath, fs::CFlexPath > >& GetPathPairs( void ) const { return m_pathPairs; }
	void SetupSourcePaths( const std::vector< CFileAttr* >& srcFiles );
	void ResetDestPaths( void );

	static bool IsValidFormat( const std::tstring& format );
	bool GenerateDestPaths( const fs::CPath& destPath, const std::tstring& format, UINT* pSeqCount, bool forceShallowStreamNames = false );

	bool BuildArchiveStorageFile( const fs::CPath& destStgPath, FileOp fileOp, CWnd* pParentWnd = AfxGetMainWnd() ) const;

	bool CanCommitOperations( std::vector< std::pair< fs::CFlexPath, fs::CFlexPath > >& rErrorPairs, FileOp fileOp, bool isUndoOp ) const;
	bool CommitOperations( FileOp fileOp, bool isUndoOp = false ) const;
private:
	static void CommitOperation( FileOp fileOp, const std::pair< fs::CFlexPath, fs::CFlexPath >& filePair, bool isUndoOp = false ) throws_( CException* );
private:
	persist std::vector< std::pair< fs::CFlexPath, fs::CFlexPath > > m_pathPairs;
	std::tstring m_password;		// encrypted
};


#endif // ArchiveImagesContext_h
