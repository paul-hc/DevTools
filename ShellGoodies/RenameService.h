#ifndef RenameService_h
#define RenameService_h
#pragma once

#include "utl/Path.h"
#include "AppCommands.h"


class CRenameItem;
class CPickDataset;
class CPathFormatter;


class CRenameService
{
public:
	CRenameService( const std::vector< CRenameItem* >& renameItems ) { StoreRenameItems( renameItems ); }

	void StoreRenameItems( const std::vector< CRenameItem* >& renameItems );

	UINT FindNextAvailSeqCount( const CPathFormatter& formatter ) const;

	bool CheckPathCollisions( cmd::IErrorObserver* pErrorObserver );

	std::auto_ptr< CPickDataset > MakeFnamePickDataset( void ) const;
	std::auto_ptr< CPickDataset > MakeDirPickDataset( void ) const;
public:
	void QueryDestFilenames( std::vector< std::tstring >& rDestFnames ) const;
	bool FileExistOutsideWorkingSet( const fs::CPath& filePath ) const;		// collision with an existing file/dir outside working set (selected files)

	static fs::CPath GetDestPath( fs::TPathPairMap::const_iterator itPair );
	static std::tstring GetDestFname( fs::TPathPairMap::const_iterator itPair );

	// text tools
	static std::tstring ApplyTextTool( UINT menuId, const std::tstring& text );
private:
	fs::TPathPairMap m_renamePairs;
};


enum
{
	PickFilenameMaxCount = 10000,
	IDC_PICK_FILENAME_BASE = 11300,
	IDC_PICK_FILENAME_MAX = IDC_PICK_FILENAME_BASE + PickFilenameMaxCount - 1,

	PickDirPathMaxCount = 256,
	IDC_PICK_DIR_PATH_BASE = 11000,
	IDC_PICK_DIR_PATH_MAX = IDC_PICK_DIR_PATH_BASE + PickDirPathMaxCount - 1
};


class CPickDataset
{
public:
	CPickDataset( std::vector< std::tstring >* pDestFnames );		// for fnames
	CPickDataset( const fs::CPath& firstDestPath );					// for parent subdirs

	// filenames
	enum BestMatch { Empty, CommonPrefix, CommonSubstring };

	bool IsSingleFile( void ) const { return 1 == m_destFnames.size(); }
	BestMatch GetBestMatch( void ) const { return m_bestMatch; }
	bool HasCommonSequence( void ) const { return m_commonSequence.size() > 1; }

	const std::vector< std::tstring >& GetDestFnames( void ) const { ASSERT( !m_destFnames.empty() ); return m_destFnames; }
	const std::tstring& GetCommonSequence( void ) const { return m_commonSequence; }

	void MakePickFnameMenu( CMenu* pPopupMenu, const TCHAR* pSelFname = NULL ) const;
	std::tstring GetPickedFname( UINT cmdId ) const;

	// parent subdirs
	bool HasSubDirs( void ) const { return !m_subDirs.empty(); }
	const std::vector< std::tstring >& GetSubDirs( void ) const { ASSERT( !m_subDirs.empty() ); return m_subDirs; }

	void MakePickDirMenu( CMenu* pPopupMenu ) const;								// single command or pick menu
	std::tstring GetPickedDirectory( UINT cmdId ) const;
private:
	std::tstring ExtractLongestCommonPrefix( void ) const;
	bool AllHavePrefix( size_t prefixLen ) const;

	static const TCHAR* EscapeAmpersand( const std::tstring& text );
private:
	// filenames
	std::vector< std::tstring > m_destFnames;
	BestMatch m_bestMatch;
	std::tstring m_commonSequence;

	// parent subdirs
	std::vector< std::tstring > m_subDirs;
};


namespace func
{
	struct AssignFname
	{
		AssignFname( std::vector< std::tstring >::const_iterator itFnames ) : m_itFnames( itFnames ) {}

		void operator()( fs::CPathParts& rDestParts ) const
		{
			rDestParts.m_fname = *m_itFnames++;
		}
	private:
		mutable std::vector< std::tstring >::const_iterator m_itFnames;
	};
}


#endif // RenameService_h
