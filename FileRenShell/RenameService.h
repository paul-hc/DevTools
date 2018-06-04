#ifndef RenameService_h
#define RenameService_h
#pragma once

#include "utl/Path.h"
#include "FileCommands_fwd.h"


class CRenameItem;


enum
{
	PickFilenameMaxCount = 10000,
	IDC_PICK_FILENAME_BASE = 11300,
	IDC_PICK_FILENAME_MAX = IDC_PICK_FILENAME_BASE + PickFilenameMaxCount - 1,

	PickDirPathMaxCount = 256,
	IDC_PICK_DIR_PATH_BASE = 11000,
	IDC_PICK_DIR_PATH_MAX = IDC_PICK_DIR_PATH_BASE + PickDirPathMaxCount - 1
};


class CRenameService
{
public:
	CRenameService( const std::vector< CRenameItem* >& renameItems );

	UINT FindNextAvailSeqCount( const std::tstring& format ) const;

	bool CheckPathCollisions( cmd::IErrorObserver* pErrorObserver );

	void QueryDestFilenames( std::vector< std::tstring >& rDestFnames ) const;
	void QuerySubDirs( std::vector< std::tstring >& rSubDirs ) const;

	void MakePickFnamePatternMenu( std::tstring* pSinglePattern, CMenu* pPopupMenu, const TCHAR* pSelFname = NULL ) const;		// single pattern or pick menu
	bool MakePickDirPathMenu( UINT* pSingleCmdId, CMenu* pPopupMenu ) const;													// single command or pick menu

	std::tstring GetPickedFname( UINT cmdId, std::vector< std::tstring >* pDestFnames = NULL ) const;
	std::tstring GetPickedDirectory( UINT cmdId ) const;
public:
	static std::tstring GetDestPath( fs::TPathPairMap::const_iterator itPair );
	static std::tstring GetDestFname( fs::TPathPairMap::const_iterator itPair );
	static std::tstring& EscapeAmpersand( std::tstring& rText );

	// text tools
	static std::tstring ApplyTextTool( UINT cmdId, const std::tstring& text );

	static std::tstring ExtractLongestCommonPrefix( const std::vector< std::tstring >& destFnames );
private:
	static bool AllHavePrefix( const std::vector< std::tstring >& destFnames, size_t prefixLen );

	bool FileExistOutsideWorkingSet( const fs::CPath& filePath ) const;		// collision with an existing file/dir outside working set (selected files)
private:
	fs::TPathPairMap m_renamePairs;
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
