#ifndef FileSetUi_h
#define FileSetUi_h
#pragma once

#include "FileWorkingSet.h"


enum
{
	PickFilenameMaxCount = 10000,
	IDC_PICK_FILENAME_BASE = 11300,
	IDC_PICK_FILENAME_MAX = IDC_PICK_FILENAME_BASE + PickFilenameMaxCount - 1,

	PickDirPathMaxCount = 256,
	IDC_PICK_DIR_PATH_BASE = 11000,
	IDC_PICK_DIR_PATH_MAX = IDC_PICK_DIR_PATH_BASE + PickDirPathMaxCount - 1
};


class CFileSetUi
{
public:
	CFileSetUi( CFileWorkingSet* pFileWorkingSet );

	void QueryDestFilenames( std::vector< std::tstring >& rDestFnames ) const;
	void QuerySubDirs( std::vector< std::tstring >& rSubDirs ) const;

	std::tstring ExtractLongestCommonPrefix( const std::vector< std::tstring >& destFnames ) const;

	void MakePickFnamePatternMenu( std::tstring* pSinglePattern, CMenu* pPopupMenu, const TCHAR* pSelFname = NULL ) const;		// single pattern or pick menu
	bool MakePickDirPathMenu( UINT* pSingleCmdId, CMenu* pPopupMenu ) const;													// single command or pick menu

	std::tstring GetPickedFname( UINT cmdId, std::vector< std::tstring >* pDestFnames = NULL ) const;
	std::tstring GetPickedDirectory( UINT cmdId ) const;

	static std::tstring GetDestPath( fs::PathPairMap::const_iterator itPair );
	static std::tstring GetDestFname( fs::PathPairMap::const_iterator itPair );
	static std::tstring& EscapeAmpersand( std::tstring& rText );

	// text tools
	static std::tstring ApplyTextTool( UINT cmdId, const std::tstring& text );
private:
	bool AllHavePrefix( const std::vector< std::tstring >& destFnames, size_t prefixLen ) const;
private:
	const fs::PathPairMap& m_renamePairs;
};


#endif // FileSetUi_h
