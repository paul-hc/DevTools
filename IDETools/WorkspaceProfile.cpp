
#include "stdafx.h"
#include "IdeUtilities.h"
#include "WkspSaveDialog.h"
#include "WkspLoadDialog.h"
#include "WorkspaceProfile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


LPCTSTR defaulWkspSection = _T("WorkspaceProfiles");
LPCTSTR defaulProjectName = _T("<Default>");
LPCTSTR sectionWorkspaceDialogs = _T("WorkspaceDialogs");


IMPLEMENT_DYNCREATE( WorkspaceProfile, CCmdTarget )


WorkspaceProfile::WorkspaceProfile()
	: CCmdTarget()
	, m_options( NULL )		// No profile IO !
	, projectNameArray()
	, fileArray()
	, m_mustCloseAll( TRUE )
{
	EnableAutomation();
	// To keep the application running as long as an OLE automation
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();
}

WorkspaceProfile::~WorkspaceProfile()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	AfxOleUnlockApp();
}

void WorkspaceProfile::OnFinalRelease( void )
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.
	CCmdTarget::OnFinalRelease();
}

int WorkspaceProfile::findProjectName( LPCTSTR projectName ) const
{
	if ( projectName != NULL && projectName[ 0 ] != _T('\0') )
		for ( unsigned int i = 0; i != projectNameArray.size(); ++i )
			if ( path::EquivalentPtr( projectNameArray[ i ], projectName ) )
				return i;

	return -1;
}

int WorkspaceProfile::findFile( LPCTSTR fileFullPath ) const
{
	if ( fileFullPath != NULL && fileFullPath[ 0 ] != _T('\0') )
		for ( unsigned int i = 0; i != fileArray.size(); ++i )
			if ( path::EquivalentPtr( fileArray[ i ], fileFullPath ) )
				return i;

	return -1;
}

CString WorkspaceProfile::getFileEntryName( int fileIndex ) const
{
	CString entryName;

	entryName.Format( _T("File%02d"), fileIndex + 1 );
	return entryName;
}


// message and dispatch maps

BEGIN_MESSAGE_MAP(WorkspaceProfile, CCmdTarget)
	//{{AFX_MSG_MAP(WorkspaceProfile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(WorkspaceProfile, CCmdTarget)
	//{{AFX_DISPATCH_MAP(WorkspaceProfile)
	DISP_PROPERTY_NOTIFY(WorkspaceProfile, "MustCloseAll", m_mustCloseAll, OnMustCloseAllChanged, VT_BOOL)
	DISP_FUNCTION(WorkspaceProfile, "AddFile", AddFile, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION(WorkspaceProfile, "AddProjectName", AddProjectName, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION(WorkspaceProfile, "GetFileCount", GetFileCount, VT_I4, VTS_NONE)
	DISP_FUNCTION(WorkspaceProfile, "GetFileName", GetFileName, VT_BSTR, VTS_I4)
	DISP_FUNCTION(WorkspaceProfile, "Save", Save, VT_BOOL, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(WorkspaceProfile, "Load", Load, VT_BOOL, VTS_BSTR VTS_BSTR)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IWorkspaceProfile to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .ODL file.

// {0E44AB06-90E1-11D2-A2C9-006097B8DD84}
static const IID IID_IWorkspaceProfile =
{ 0xe44ab06, 0x90e1, 0x11d2, { 0xa2, 0xc9, 0x0, 0x60, 0x97, 0xb8, 0xdd, 0x84 } };

BEGIN_INTERFACE_MAP(WorkspaceProfile, CCmdTarget)
	INTERFACE_PART(WorkspaceProfile, IID_IWorkspaceProfile, Dispatch)
END_INTERFACE_MAP()

// {0E44AB07-90E1-11D2-A2C9-006097B8DD84}
IMPLEMENT_OLECREATE(WorkspaceProfile, "IDETools.WorkspaceProfile", 0xe44ab07, 0x90e1, 0x11d2, 0xa2, 0xc9, 0x0, 0x60, 0x97, 0xb8, 0xdd, 0x84)


// WorkspaceProfile automation properties

void WorkspaceProfile::OnMustCloseAllChanged( void )
{
}


// automation methods

BOOL WorkspaceProfile::AddFile( LPCTSTR fileFullPath )
{
	if ( findFile( fileFullPath ) != -1 )
		return FALSE;

	fileArray.push_back( fileFullPath );
	return TRUE;
}

BOOL WorkspaceProfile::AddProjectName( LPCTSTR projectName )
{
	if ( findProjectName( projectName ) != -1 )
		return FALSE;

	projectNameArray.push_back( projectName );
	return TRUE;
}

long WorkspaceProfile::GetFileCount( void )
{
	return (long)fileArray.size();
}

BSTR WorkspaceProfile::GetFileName( long index )
{
	if ( index >= 0 && index < GetFileCount() )
		return fileArray[ index ].AllocSysString();
	else
		return CString().AllocSysString();
}

BOOL WorkspaceProfile::Save( LPCTSTR section, LPCTSTR currProjectName )
{
	CWkspSaveDialog dialog( *this, section, currProjectName, ide::getRootWindow() );

	return dialog.DoModal() != IDCANCEL;
}

BOOL WorkspaceProfile::Load( LPCTSTR section, LPCTSTR currProjectName )
{
	UNUSED_ALWAYS( section );
	UNUSED_ALWAYS( currProjectName );

	CWkspLoadDialog dialog( *this, section, currProjectName, ide::getRootWindow() );

	if ( dialog.DoModal() == IDCANCEL )
		return FALSE;

	return TRUE;
}
