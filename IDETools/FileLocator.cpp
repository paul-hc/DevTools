
#include "stdafx.h"
#include "IdeUtilities.h"
#include "FileLocatorDialog.h"
#include "FileLocator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE(FileLocator, CCmdTarget)


FileLocator::FileLocator( void )
{
	EnableAutomation();

	// To keep the application running as long as an OLE automation
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();
}

FileLocator::~FileLocator()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	AfxOleUnlockApp();
}

void FileLocator::OnFinalRelease( void )
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.
	CCmdTarget::OnFinalRelease();
}


// message handlers

BEGIN_MESSAGE_MAP(FileLocator, CCmdTarget)
	//{{AFX_MSG_MAP(FileLocator)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BEGIN_DISPATCH_MAP(FileLocator, CCmdTarget)
	//{{AFX_DISPATCH_MAP(FileLocator)
	DISP_PROPERTY_EX(FileLocator, "LocalDirPath", GetLocalDirPath, SetLocalDirPath, VT_BSTR)
	DISP_PROPERTY_EX(FileLocator, "LocalCurrentFile", GetLocalCurrentFile, SetLocalCurrentFile, VT_BSTR)
	DISP_PROPERTY_EX(FileLocator, "AssociatedProjectFile", GetAssociatedProjectFile, SetAssociatedProjectFile, VT_BSTR)
	DISP_PROPERTY_EX(FileLocator, "ProjectActiveConfiguration", GetProjectActiveConfiguration, SetProjectActiveConfiguration, VT_BSTR)
	DISP_PROPERTY_EX(FileLocator, "ProjectAdditionalIncludePath", GetProjectAdditionalIncludePath, SetProjectAdditionalIncludePath, VT_BSTR)
	DISP_PROPERTY_EX(FileLocator, "SelectedFiles", GetSelectedFiles, SetNotSupported, VT_BSTR)
	DISP_PROPERTY_EX(FileLocator, "SelectedCount", GetSelectedCount, SetNotSupported, VT_I4)
	DISP_FUNCTION(FileLocator, "GetSelectedFile", GetSelectedFile, VT_BSTR, VTS_I4)
	DISP_FUNCTION(FileLocator, "LocateFile", LocateFile, VT_BOOL, VTS_NONE)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IFileLocator to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .ODL file.

// {A0580B95-3350-11D5-B5A4-00D0B74ECB52}
static const IID IID_IFileLocator =
{ 0xa0580b95, 0x3350, 0x11d5, { 0xb5, 0xa4, 0x0, 0xd0, 0xb7, 0x4e, 0xcb, 0x52 } };

BEGIN_INTERFACE_MAP(FileLocator, CCmdTarget)
	INTERFACE_PART( FileLocator, IID_IFileLocator, Dispatch )
END_INTERFACE_MAP()

// {A0580B96-3350-11D5-B5A4-00D0B74ECB52}
IMPLEMENT_OLECREATE( FileLocator, "IDETools.FileLocator", 0xa0580b96, 0x3350, 0x11d5, 0xb5, 0xa4, 0x0, 0xd0, 0xb7, 0x4e, 0xcb, 0x52 )


// message handlers

BSTR FileLocator::GetLocalDirPath( void )
{
	return str::AllocSysString( m_projCtx.GetLocalDirPath() );
}

void FileLocator::SetLocalDirPath( LPCTSTR lpszNewValue )
{
	m_projCtx.SetLocalDirPath( lpszNewValue );
}

BSTR FileLocator::GetLocalCurrentFile( void )
{
	return str::AllocSysString( m_projCtx.GetLocalCurrentFile() );
}

void FileLocator::SetLocalCurrentFile( LPCTSTR lpszNewValue )
{
	m_projCtx.SetLocalCurrentFile( lpszNewValue );
}

BSTR FileLocator::GetAssociatedProjectFile( void )
{
	return str::AllocSysString( m_projCtx.GetAssociatedProjectFile() );
}

void FileLocator::SetAssociatedProjectFile( LPCTSTR lpszNewValue )
{
	m_projCtx.SetAssociatedProjectFile( lpszNewValue );
}

BSTR FileLocator::GetProjectActiveConfiguration( void )
{
	return str::AllocSysString( m_projCtx.GetProjectActiveConfiguration() );
}

void FileLocator::SetProjectActiveConfiguration( LPCTSTR lpszNewValue )
{
	m_projCtx.SetProjectActiveConfiguration( lpszNewValue );
}

BSTR FileLocator::GetProjectAdditionalIncludePath( void )
{
	// OBSOLETE:
	return str::AllocSysString( std::tstring() );
}

void FileLocator::SetProjectAdditionalIncludePath( LPCTSTR lpszNewValue )
{
	lpszNewValue;
	// OBSOLETE:
//	m_projCtx.SetProjectAdditionalIncludePath( lpszNewValue );
}

BSTR FileLocator::GetSelectedFiles( void )
{
	return str::AllocSysString( m_selectedFilesFlat );
}

long FileLocator::GetSelectedCount( void )
{
	return (long)m_selectedFiles.size();
}

BSTR FileLocator::GetSelectedFile( long index )
{
	CString result;
	if ( index >= 0 && index < m_selectedFiles.size() )
		result = m_selectedFiles[ index ].first.GetPtr();
	else
		TRACE( _T("FileLocator::GetSelectedFile(): invalid index: %d from valid range [0, %d]\n"), index, m_selectedFiles.size() );
	return result.AllocSysString();
}

BOOL FileLocator::LocateFile( void )
{
	CFileLocatorDialog dlg( ide::getRootWindow() );
	dlg.SetLocalCurrentFile( m_projCtx.GetLocalCurrentFile() );

	if ( dlg.DoModal() != IDOK )
		return FALSE;

	m_selectedFilesFlat = dlg.m_selectedFilePath;
	m_selectedFiles = dlg.m_selectedFiles;
	return TRUE;
}
