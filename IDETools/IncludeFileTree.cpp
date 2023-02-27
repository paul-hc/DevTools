
#include "pch.h"
#include "IncludeFileTree.h"
#include "IdeUtilities.h"
#include "FileTreeDialog.h"
#include "utl/Path.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE(IncludeFileTree, CCmdTarget)


IncludeFileTree::IncludeFileTree( void )
	: CCmdTarget()
{
	EnableAutomation();
	// To keep the application running as long as an OLE automation
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();
}

IncludeFileTree::~IncludeFileTree()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	AfxOleUnlockApp();
}

void IncludeFileTree::OnFinalRelease( void )
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.
	__super::OnFinalRelease();
}


// message handlers

BEGIN_MESSAGE_MAP(IncludeFileTree, CCmdTarget)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(IncludeFileTree, CCmdTarget)
	DISP_PROPERTY_NOTIFY_ID(IncludeFileTree, "PickedIncludeFile", dispidPickedIncludeFile, m_pickedIncludeFile, OnPickedIncludeFileChanged, VT_BSTR)
	DISP_PROPERTY_NOTIFY_ID(IncludeFileTree, "PromptLineNo", dispidPromptLineNo, m_promptLineNo, OnPromptLineNoChanged, VT_I4)
	DISP_FUNCTION_ID(IncludeFileTree, "BrowseIncludeFiles", dispidBrowseIncludeFiles, BrowseIncludeFiles, VT_BOOL, VTS_BSTR)
END_DISPATCH_MAP()

// Note: we add support for IID_IIncludeFileTree to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .IDL file.

// {1006E3E6-1F6F-11D2-A275-006097B8DD84}
static const IID IID_IIncludeFileTree =
{ 0x1006e3e6, 0x1f6f, 0x11d2, { 0xa2, 0x75, 0x0, 0x60, 0x97, 0xb8, 0xdd, 0x84 } };

BEGIN_INTERFACE_MAP(IncludeFileTree, CCmdTarget)
	INTERFACE_PART(IncludeFileTree, IID_IIncludeFileTree, Dispatch)
END_INTERFACE_MAP()

// {1006E3E7-1F6F-11D2-A275-006097B8DD84}
IMPLEMENT_OLECREATE(IncludeFileTree, "IDETools.IncludeFileTree", 0x1006e3e7, 0x1f6f, 0x11d2, 0xa2, 0x75, 0x0, 0x60, 0x97, 0xb8, 0xdd, 0x84)


// message handlers

void IncludeFileTree::OnPickedIncludeFileChanged( void )
{
}

void IncludeFileTree::OnPromptLineNoChanged( void )
{
}

BOOL IncludeFileTree::BrowseIncludeFiles( LPCTSTR targetFileName )
{
	m_pickedIncludeFile.Empty();
	m_promptLineNo = 0;

	if ( nullptr == targetFileName )
		return FALSE;

	if ( _T('\0') == *targetFileName || !fs::FileExist( targetFileName, fs::Read ) )
		TRACE( _T("Specified filename does not exist: [%s]\n"), targetFileName );

	ide::CScopedWindow scopedIDE;
	CFileTreeDialog	pickerDlg( targetFileName, scopedIDE.GetMainWnd() );
	if ( IDCANCEL == pickerDlg.DoModal() )
		return FALSE;

	m_pickedIncludeFile = pickerDlg.GetRootPath().GetPtr();
	m_promptLineNo = pickerDlg.GetSourceLineNo();
	return TRUE;
}
