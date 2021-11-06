
#include "stdafx.h"
#include "FileSearch.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


struct FileFind_Access : public CFileFind
{
	WIN32_FIND_DATA* getFindData( void ) const	{ return (WIN32_FIND_DATA*)m_pFoundInfo; }
	WIN32_FIND_DATA* getNextData( void ) const	{ return (WIN32_FIND_DATA*)m_pNextInfo; }
};


IMPLEMENT_DYNCREATE(FileSearch, CCmdTarget)

FileSearch::FileSearch()
{
	nextFound = FALSE;

	fileAttrFilterStrict = 0;
	fileAttrFilterStrictNot = 0;
	fileAttrFilterOr = 0;
	excludeDirDots = true;

	EnableAutomation();
	// To keep the application running as long as an OLE automation
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();
}

FileSearch::~FileSearch()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	AfxOleUnlockApp();
}

void FileSearch::OnFinalRelease( void )
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.
	CCmdTarget::OnFinalRelease();
}

WIN32_FIND_DATA* FileSearch::getFindData( void ) const
{
	return ( (FileFind_Access&)m_fileFind ).getFindData();
}

WIN32_FIND_DATA* FileSearch::getNextData( void ) const
{
	return ( (FileFind_Access&)m_fileFind ).getNextData();
}

bool FileSearch::isFiltered( void ) const
{
	return fileAttrFilterStrict != 0 || fileAttrFilterStrictNot != 0 || fileAttrFilterOr != 0 || excludeDirDots;
}

bool FileSearch::isTargetFile( void ) const
{
	WIN32_FIND_DATA* findData = getFindData();

	VERIFY( findData != NULL );

	DWORD			attr = findData->dwFileAttributes;

	if ( fileAttrFilterStrict != 0 && ( attr & fileAttrFilterStrict ) != fileAttrFilterStrict )
		return false;		// File doesn't have mandatory specified attribute

	if ( fileAttrFilterStrictNot != 0 && ( attr & fileAttrFilterStrictNot ) != 0 )
		return false;		// File has mandatory excluded attribute

	if ( fileAttrFilterOr != 0 && ( attr & fileAttrFilterOr ) == 0 )
		return false;		// File has none of the OR specified attributes

	if ( excludeDirDots && ( attr & FILE_ATTRIBUTE_DIRECTORY ) != 0 && m_fileFind.IsDots() )
		return false;		// Reject dots directory file

	return true;
}

bool FileSearch::findNextTargetFile( void )
{
	// Take a step forward and actually setup m_fileFind with the first found file
	nextFound = m_fileFind.FindNextFile();
	if ( !isFiltered() )
		return TRUE;

	// File attribute filter specified -> while not matching attribute filter search for the next file
	BOOL			moreFiles;

	do
	{
		moreFiles = nextFound;
		if ( isTargetFile() )
			return TRUE;
		nextFound = m_fileFind.FindNextFile();
	} while ( moreFiles );

	return FALSE;
}

CString FileSearch::doFindAllFiles( LPCTSTR filePattern, LPCTSTR separator, long& outFileCount,
									BOOL recurseSubDirs /*= FALSE*/ )
{
	CString			foundPaths;
	BOOL			found = FindFile( filePattern );

	ASSERT( separator != NULL && *separator != _T('\0') );
	for ( ; found; ++outFileCount )
	{
		if ( !foundPaths.IsEmpty() )
			foundPaths += separator;
		foundPaths += m_fileFind.GetFilePath();
		found = FindNextFile();
	}

	if ( recurseSubDirs )
	{
		FileSearch		subDirFinder;
		CString			subDirFilePattern = subDirFinder.SetupForSubDirSearch( filePattern );

		// Do sub-directory search
		found = subDirFinder.FindFile( subDirFilePattern );
		while ( found )
		{	// Recurse to the found sub-directory
			foundPaths += doFindAllFiles( CString( subDirFinder.BuildSubDirFilePattern( filePattern ) ),
										  separator, outFileCount, recurseSubDirs );
			found = subDirFinder.FindNextFile();
		}
	}

	return foundPaths;
}


// message handlers

BEGIN_MESSAGE_MAP(FileSearch, CCmdTarget)
	//{{AFX_MSG_MAP(FileSearch)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(FileSearch, CCmdTarget)
	//{{AFX_DISPATCH_MAP(FileSearch)
	DISP_PROPERTY_EX(FileSearch, "FileAttrFilterStrict", GetFileAttrFilterStrict, SetFileAttrFilterStrict, VT_I4)
	DISP_PROPERTY_EX(FileSearch, "FileAttrFilterStrictNot", GetFileAttrFilterStrictNot, SetFileAttrFilterStrictNot, VT_I4)
	DISP_PROPERTY_EX(FileSearch, "FileAttrFilterOr", GetFileAttrFilterOr, SetFileAttrFilterOr, VT_I4)
	DISP_PROPERTY_EX(FileSearch, "ExcludeDirDots", GetExcludeDirDots, SetExcludeDirDots, VT_BOOL)
	DISP_PROPERTY_EX(FileSearch, "FileAttributes", GetFileAttributes, SetNotSupported, VT_I4)
	DISP_PROPERTY_EX(FileSearch, "FileName", GetFileName, SetNotSupported, VT_BSTR)
	DISP_PROPERTY_EX(FileSearch, "FilePath", GetFilePath, SetNotSupported, VT_BSTR)
	DISP_PROPERTY_EX(FileSearch, "FileTitle", GetFileTitle, SetNotSupported, VT_BSTR)
	DISP_PROPERTY_EX(FileSearch, "FileURL", GetFileURL, SetNotSupported, VT_BSTR)
	DISP_PROPERTY_EX(FileSearch, "Root", GetRoot, SetNotSupported, VT_BSTR)
	DISP_PROPERTY_EX(FileSearch, "Length", GetLength, SetNotSupported, VT_I4)
	DISP_PROPERTY_EX(FileSearch, "IsDots", GetIsDots, SetNotSupported, VT_BOOL)
	DISP_PROPERTY_EX(FileSearch, "IsReadOnly", GetIsReadOnly, SetNotSupported, VT_BOOL)
	DISP_PROPERTY_EX(FileSearch, "IsDirectory", GetIsDirectory, SetNotSupported, VT_BOOL)
	DISP_PROPERTY_EX(FileSearch, "IsCompressed", GetIsCompressed, SetNotSupported, VT_BOOL)
	DISP_PROPERTY_EX(FileSearch, "IsSystem", GetIsSystem, SetNotSupported, VT_BOOL)
	DISP_PROPERTY_EX(FileSearch, "IsHidden", GetIsHidden, SetNotSupported, VT_BOOL)
	DISP_PROPERTY_EX(FileSearch, "IsTemporary", GetIsTemporary, SetNotSupported, VT_BOOL)
	DISP_PROPERTY_EX(FileSearch, "IsNormal", GetIsNormal, SetNotSupported, VT_BOOL)
	DISP_PROPERTY_EX(FileSearch, "IsArchived", GetIsArchived, SetNotSupported, VT_BOOL)
	DISP_FUNCTION(FileSearch, "FindFile", FindFile, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION(FileSearch, "FindNextFile", FindNextFile, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(FileSearch, "FindAllFiles", FindAllFiles, VT_BSTR, VTS_BSTR VTS_BSTR VTS_PI4 VTS_BOOL)
	DISP_FUNCTION(FileSearch, "Close", Close, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(FileSearch, "MatchesMask", MatchesMask, VT_BOOL, VTS_I4)
	DISP_FUNCTION(FileSearch, "BuildSubDirFilePattern", BuildSubDirFilePattern, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION(FileSearch, "SetupForSubDirSearch", SetupForSubDirSearch, VT_BSTR, VTS_BSTR)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IFileSearch to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .ODL file.

// {C722D0B5-1E2D-11D5-B59B-00D0B74ECB52}
static const IID IID_IFileSearch =
{ 0xc722d0b5, 0x1e2d, 0x11d5, { 0xb5, 0x9b, 0x0, 0xd0, 0xb7, 0x4e, 0xcb, 0x52 } };

BEGIN_INTERFACE_MAP(FileSearch, CCmdTarget)
	INTERFACE_PART(FileSearch, IID_IFileSearch, Dispatch)
END_INTERFACE_MAP()

// {C722D0B6-1E2D-11D5-B59B-00D0B74ECB52}
IMPLEMENT_OLECREATE(FileSearch, "IDETools.FileSearch", 0xc722d0b6, 0x1e2d, 0x11d5, 0xb5, 0x9b, 0x0, 0xd0, 0xb7, 0x4e, 0xcb, 0x52)

/////////////////////////////////////////////////////////////////////////////
// FileSearch message handlers

long FileSearch::GetFileAttrFilterStrict()
{
	return fileAttrFilterStrict;
}

void FileSearch::SetFileAttrFilterStrict( long nNewValue )
{
	fileAttrFilterStrict = nNewValue;
}

long FileSearch::GetFileAttrFilterStrictNot()
{
	return fileAttrFilterStrictNot;
}

void FileSearch::SetFileAttrFilterStrictNot( long nNewValue )
{
	fileAttrFilterStrictNot = nNewValue;
}

long FileSearch::GetFileAttrFilterOr()
{
	return fileAttrFilterOr;
}

void FileSearch::SetFileAttrFilterOr( long nNewValue )
{
	fileAttrFilterOr = nNewValue;
}

BOOL FileSearch::GetExcludeDirDots()
{
	return excludeDirDots;
}

void FileSearch::SetExcludeDirDots( BOOL bNewValue )
{
	excludeDirDots = ( bNewValue != FALSE );
}

long FileSearch::GetFileAttributes()
{
	WIN32_FIND_DATA* findData = getFindData();

	return findData != NULL ? findData->dwFileAttributes : 0;
}

BSTR FileSearch::GetFileName()
{
	return m_fileFind.GetFileName().AllocSysString();
}

BSTR FileSearch::GetFilePath()
{
	return m_fileFind.GetFilePath().AllocSysString();
}

BSTR FileSearch::GetFileTitle()
{
	return m_fileFind.GetFileTitle().AllocSysString();
}

BSTR FileSearch::GetFileURL()
{
	return m_fileFind.GetFileURL().AllocSysString();
}

BSTR FileSearch::GetRoot()
{
	return m_fileFind.GetRoot().AllocSysString();
}

long FileSearch::GetLength()
{
	return static_cast<long>( m_fileFind.GetLength() );
}

BOOL FileSearch::GetIsDots()
{
	return m_fileFind.IsDots();
}

BOOL FileSearch::GetIsReadOnly()
{
	return m_fileFind.IsReadOnly();
}

BOOL FileSearch::GetIsDirectory()
{
	return m_fileFind.IsDirectory();
}

BOOL FileSearch::GetIsCompressed()
{
	return m_fileFind.IsCompressed();
}

BOOL FileSearch::GetIsSystem()
{
	return m_fileFind.IsSystem();
}

BOOL FileSearch::GetIsHidden()
{
	return m_fileFind.IsHidden();
}

BOOL FileSearch::GetIsTemporary()
{
	return m_fileFind.IsTemporary();
}

BOOL FileSearch::GetIsNormal()
{
	return m_fileFind.IsNormal();
}

BOOL FileSearch::GetIsArchived()
{
	return m_fileFind.IsArchived();
}

BOOL FileSearch::FindFile( LPCTSTR filePattern )
{
	nextFound = FALSE;
	if ( !m_fileFind.FindFile( filePattern ) )
		return FALSE;
	return findNextTargetFile();
}

BOOL FileSearch::FindNextFile()
{
	if ( !nextFound )
		return FALSE;
	return findNextTargetFile();
}

// Returns a string containing all the file paths concatenated and separated by the specified separator string.
// If the separator is null or empty, the default ";" will be used.
BSTR FileSearch::FindAllFiles( LPCTSTR filePattern, LPCTSTR separator, long FAR* outFileCount, BOOL recurseSubDirs )
{
	CString foundPaths;
	long fileCount = 0;

	if ( separator == NULL || *separator == _T('\0') )
		separator = _T(";");
	foundPaths = doFindAllFiles( filePattern, separator, fileCount, recurseSubDirs );

	if ( outFileCount != NULL )
		*outFileCount = fileCount;

	return foundPaths.AllocSysString();
}

void FileSearch::Close()
{
	nextFound = FALSE;
	m_fileFind.Close();
}

BOOL FileSearch::MatchesMask(long mask)
{
	return m_fileFind.MatchesMask( mask );
}

// Assuming the current found file is a directory, appends the filePattern filename and extension
// to the full path of the folder
BSTR FileSearch::BuildSubDirFilePattern( LPCTSTR filePattern )
{
	CString subDirFilePattern;

	if ( m_fileFind.IsDirectory() && !m_fileFind.IsDots() )
	{
		CString subDirPath = m_fileFind.GetFilePath();
		int len = subDirPath.GetLength();
		TCHAR drive[ _MAX_DRIVE ], dir[ _MAX_DIR ], fname[ _MAX_FNAME ], ext[ _MAX_EXT ];

		if ( filePattern == NULL || *filePattern == _T('\0') )
			filePattern = _T("*.*");
		// Append a trailing backslash if not already
		if ( len > 3 )
			if ( subDirPath[ len - 1 ] != _T('\\') && subDirPath[ len - 1 ] != _T('/') )
				subDirPath += _T("\\");
		// Get the subdir full path
		_tsplitpath( subDirPath, drive, dir, NULL, NULL );
		// Get the pattern filename and extension
		_tsplitpath( filePattern, NULL, NULL, fname, ext );
		// Mix together the subdir path and filter filename and extension into the resulting subdir pattern
		_tmakepath( subDirFilePattern.GetBuffer( _MAX_PATH ), drive, dir, fname, ext );
		subDirFilePattern.ReleaseBuffer();
	}
	return subDirFilePattern.AllocSysString();
}

// Prepares this object for sub-directory searching.
// Returns the pattern to be used for subdirectory searching.
BSTR FileSearch::SetupForSubDirSearch( LPCTSTR parentFilePattern )
{
	CString subDirFilePattern;

	fileAttrFilterStrict = FILE_ATTRIBUTE_DIRECTORY;
	fileAttrFilterStrictNot = fileAttrFilterOr = 0;
	excludeDirDots = true;

	TCHAR drive[ _MAX_DRIVE ], dir[ _MAX_DIR ];

	_tsplitpath( parentFilePattern, drive, dir, NULL, NULL );
	_tmakepath( subDirFilePattern.GetBuffer( _MAX_PATH ), drive, dir, _T("*"), _T(".*") );
	subDirFilePattern.ReleaseBuffer();

	Close();
	return subDirFilePattern.AllocSysString();
}
