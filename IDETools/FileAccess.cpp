
#include "pch.h"
#include "FileAccess.h"
#include "FileAssoc.h"
#include "IdeUtilities.h"
#include "OutputActivator.h"
#include "ModuleSession.h"
#include "Application.h"
#include "utl/FileSystem.h"
#include "utl/UI/ShellUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)


IMPLEMENT_DYNCREATE(FileAccess, CCmdTarget)


FileAccess::FileAccess()
{
	EnableAutomation();

	// To keep the application running as long as an OLE automation
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();
}

FileAccess::~FileAccess()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	AfxOleUnlockApp();
}

void FileAccess::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.
	__super::OnFinalRelease();
}

BEGIN_MESSAGE_MAP(FileAccess, CCmdTarget)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(FileAccess, CCmdTarget)
	DISP_FUNCTION_ID(FileAccess, "FileExist", dispidFileExist, FileExist, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(FileAccess, "GetFileAttributes", dispidGetFileAttributes, GetFileAttributes, VT_I4, VTS_BSTR)
	DISP_FUNCTION_ID(FileAccess, "IsDirectoryFile", dispidIsDirectoryFile, IsDirectoryFile, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(FileAccess, "IsCompressedFile", dispidIsCompressedFile, IsCompressedFile, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(FileAccess, "IsReadOnlyFile", dispidIsReadOnlyFile, IsReadOnlyFile, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(FileAccess, "SetReadOnlyFile", dispidSetReadOnlyFile, SetReadOnlyFile, VT_BOOL, VTS_BSTR VTS_BOOL)
	DISP_FUNCTION_ID(FileAccess, "Execute", dispidExecute, Execute, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(FileAccess, "ShellOpen", dispidShellOpen, ShellOpen, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(FileAccess, "ExploreAndSelectFile", dispidExploreAndSelectFile, ExploreAndSelectFile, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(FileAccess, "GetNextAssocDoc", dispidGetNextAssocDoc, GetNextAssocDoc, VT_BSTR, VTS_BSTR VTS_BOOL)
	DISP_FUNCTION_ID(FileAccess, "GetNextVariationDoc", dispidGetNextVariationDoc, GetNextVariationDoc, VT_BSTR, VTS_BSTR VTS_BOOL)
	DISP_FUNCTION_ID(FileAccess, "GetComplementaryDoc", dispidGetComplementaryDoc, GetComplementaryDoc, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION_ID(FileAccess, "OutputWndActivateTab", dispidOutputWndActivateTab, OutputWndActivateTab, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(FileAccess, "GetIDECurrentBrowseFile", dispidGetIDECurrentBrowseFile, GetIDECurrentBrowseFile, VT_BSTR, VTS_NONE)
	DISP_FUNCTION_ID(FileAccess, "UpdateIDECurrentBrowseFile", dispidUpdateIDECurrentBrowseFile, UpdateIDECurrentBrowseFile, VT_BOOL, VTS_BOOL)
END_DISPATCH_MAP()

// Note: we add support for IID_IFileAccess to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .IDL file.

// {1556FB24-22DB-11D2-A278-006097B8DD84}
static const IID IID_IFileAccess =
{ 0x1556fb24, 0x22db, 0x11d2, { 0xa2, 0x78, 0x0, 0x60, 0x97, 0xb8, 0xdd, 0x84 } };

BEGIN_INTERFACE_MAP(FileAccess, CCmdTarget)
	INTERFACE_PART(FileAccess, IID_IFileAccess, Dispatch)
END_INTERFACE_MAP()

// {1556FB25-22DB-11D2-A278-006097B8DD84}
IMPLEMENT_OLECREATE(FileAccess, "IDETools.FileAccess", 0x1556fb25, 0x22db, 0x11d2, 0xa2, 0x78, 0x0, 0x60, 0x97, 0xb8, 0xdd, 0x84)


// message handlers

BOOL FileAccess::FileExist( LPCTSTR fullPath )
{
	return fs::IsValidFile( fullPath );
}

long FileAccess::GetFileAttributes( LPCTSTR fullPath )
{
	return ::GetFileAttributes( fullPath );
}

BOOL FileAccess::IsDirectoryFile( LPCTSTR fullPath )
{
	DWORD fileAttributes = ::GetFileAttributes( fullPath );

	return fileAttributes != INVALID_FILE_ATTRIBUTES && ( fileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0;
}

BOOL FileAccess::IsCompressedFile( LPCTSTR fullPath )
{
	DWORD fileAttributes = ::GetFileAttributes( fullPath );

	return fileAttributes != INVALID_FILE_ATTRIBUTES && ( fileAttributes & FILE_ATTRIBUTE_COMPRESSED ) != 0;
}

BOOL FileAccess::IsReadOnlyFile( LPCTSTR fullPath )
{
	DWORD fileAttributes = ::GetFileAttributes( fullPath );

	return fileAttributes != INVALID_FILE_ATTRIBUTES && ( fileAttributes & FILE_ATTRIBUTE_READONLY ) != 0;
}

BOOL FileAccess::SetReadOnlyFile( LPCTSTR fullPath, BOOL readOnly )
{
	DWORD fileAttributes = ::GetFileAttributes( fullPath );

	if ( fileAttributes == INVALID_FILE_ATTRIBUTES )
		return FALSE;

	if ( readOnly )
		fileAttributes |= FILE_ATTRIBUTE_READONLY;
	else
		fileAttributes &= ~FILE_ATTRIBUTE_READONLY;

	return SetFileAttributes( fullPath, fileAttributes ) != FALSE;
}

BOOL FileAccess::Execute( LPCTSTR fullPath )
{
	HINSTANCE hInstExec = (HINSTANCE)(UINT_PTR)WinExec( str::ToUtf8( fullPath ).c_str(), SW_SHOWNORMAL );

	if ( (UINT_PTR)hInstExec >= HINSTANCE_ERROR )
		return TRUE;
	AfxMessageBox( shell::GetExecErrorMessage( fullPath, hInstExec ).c_str() );
	return FALSE;
}

BOOL FileAccess::ShellOpen( LPCTSTR docFullPath )
{
	HINSTANCE hInstExec = shell::Execute( nullptr, docFullPath );

	return (UINT_PTR)hInstExec >= HINSTANCE_ERROR;
}

BOOL FileAccess::ExploreAndSelectFile( LPCTSTR fileFullPath )
{
	return shell::ExploreAndSelectFile( fileFullPath );
}

BSTR FileAccess::GetNextAssocDoc( LPCTSTR docFullPath, BOOL forward )
{
	CFileAssoc fileAssoc( docFullPath );
	CString nextDocFullPath;

	if ( fileAssoc.IsValidKnownAssoc() )
		nextDocFullPath = fileAssoc.GetNextAssoc( forward != FALSE ).GetPtr();
	return nextDocFullPath.AllocSysString();
}

BSTR FileAccess::GetNextVariationDoc( LPCTSTR docFullPath, BOOL forward )
{
	CFileAssoc fileAssoc( docFullPath );
	CString nextDocFullPath;

	if ( fileAssoc.IsValidKnownAssoc() )
		nextDocFullPath = fileAssoc.GetNextFileNameVariationEx( forward != FALSE ).GetPtr();
	return nextDocFullPath.AllocSysString();
}

BSTR FileAccess::GetComplementaryDoc( LPCTSTR docFullPath )
{
	CFileAssoc fileAssoc( docFullPath );
	CString complDocFullPath;

	if ( fileAssoc.IsValidKnownAssoc() )
		complDocFullPath = fileAssoc.GetComplementaryAssoc().GetPtr();
	return complDocFullPath.AllocSysString();
}

BOOL FileAccess::OutputWndActivateTab( LPCTSTR tabCaption )
{
	HWND hWndOutput =::GetFocus(), hWndTab = nullptr;

	if ( hWndOutput == nullptr )
		return FALSE;
	hWndTab = ::GetWindow( hWndOutput, GW_HWNDFIRST );
	if ( hWndTab != nullptr )
		hWndTab = ::GetWindow( hWndTab, GW_HWNDNEXT );
	if ( hWndTab == nullptr )
		return FALSE;

	return ( new OutputActivator( hWndOutput, hWndTab, tabCaption ) )->CreateThread();
}


#define	CM_IDE_CLOSEBROWSEFILE			0x8502
#define	IDE_CLOSEBROWSEFILE_PREFIX		_T("Close Source Browser &File ")


BSTR FileAccess::GetIDECurrentBrowseFile( void )
{
	std::pair<HMENU, int> foundPopup = ide::FindPopupMenuWithCommand( ide::GetMainWindow()->GetSafeHwnd(), CM_IDE_CLOSEBROWSEFILE );

	if ( foundPopup.first == nullptr )
		return nullptr;

	// Found CM_IDE_CLOSEBROWSEFILE popup item:
	UINT itemState = GetMenuState( foundPopup.first, CM_IDE_CLOSEBROWSEFILE, MF_BYCOMMAND );

	ASSERT( itemState != UINT( -1 ) );
	if ( itemState & ( MF_DISABLED | MF_GRAYED ) )
		return nullptr;		// Menu item is disabled !

	CString itemText, currBrowseFN;

	GetMenuString( foundPopup.first, CM_IDE_CLOSEBROWSEFILE,
				   itemText.GetBuffer( MAX_PATH ), MAX_PATH,
				   MF_BYCOMMAND );
	itemText.ReleaseBuffer();

	if ( itemText.IsEmpty() || itemText.Find( IDE_CLOSEBROWSEFILE_PREFIX ) == -1 )
		return nullptr;

	// found "Close Source Browser &File file.ext" popup item -> extract filename
	int start = (int)_tcslen( IDE_CLOSEBROWSEFILE_PREFIX );
	int end = itemText.FindOneOf( _T("\t\a") );

	currBrowseFN = ( end != -1 ? itemText.Mid( start, end - start ) : itemText.Mid( start ) );
	return currBrowseFN.AllocSysString();
}

BOOL FileAccess::UpdateIDECurrentBrowseFile( BOOL doItNow )
{
	if ( app::GetModuleSession().m_vsUseStandardWindowsMenu )
	{	// This feature is supported only for Visual C++ using "screen reader compatible menus".
		HWND hWndIde = ide::GetMainWindow()->GetSafeHwnd();
		std::pair<HMENU, int> foundPopup = ide::FindPopupMenuWithCommand( hWndIde, CM_IDE_CLOSEBROWSEFILE );

		if ( foundPopup.first != nullptr )
		{
			if ( doItNow )
				::SendMessage( hWndIde, WM_INITMENUPOPUP, (WPARAM)foundPopup.first, MAKELPARAM( foundPopup.second, FALSE ) );
			else
				::PostMessage( hWndIde, WM_INITMENUPOPUP, (WPARAM)foundPopup.first, MAKELPARAM( foundPopup.second, FALSE ) );

			return TRUE;
		}
	}

	return FALSE;
}
