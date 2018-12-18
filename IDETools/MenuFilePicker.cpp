
#include "stdafx.h"
#include "MenuFilePicker.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE( MenuFilePicker, CCmdTarget )


MenuFilePicker::MenuFilePicker( void )
	: CCmdTarget()
	, m_browser()
{
	m_trackPosX = m_trackPosY = -1;

	EnableAutomation();
	// To keep the application running as long as an OLE automation
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();
}

MenuFilePicker::~MenuFilePicker()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	AfxOleUnlockApp();
}

void MenuFilePicker::OnFinalRelease( void )
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.
	CCmdTarget::OnFinalRelease();
}


// message handlers

BEGIN_MESSAGE_MAP(MenuFilePicker, CCmdTarget)
END_MESSAGE_MAP()


BEGIN_DISPATCH_MAP(MenuFilePicker, CCmdTarget)
	//{{AFX_DISPATCH_MAP(MenuFilePicker)
	DISP_PROPERTY_NOTIFY(MenuFilePicker, "TrackPosX", m_trackPosX, OnTrackPosXChanged, VT_I4)
	DISP_PROPERTY_NOTIFY(MenuFilePicker, "TrackPosY", m_trackPosY, OnTrackPosYChanged, VT_I4)
	DISP_PROPERTY_EX(MenuFilePicker, "OptionFlags", GetOptionFlags, SetOptionFlags, VT_I4)
	DISP_PROPERTY_EX(MenuFilePicker, "FolderLayout", GetFolderLayout, SetFolderLayout, VT_I4)
	DISP_PROPERTY_EX(MenuFilePicker, "CurrentFileName", GetCurrentFileName, SetCurrentFileName, VT_BSTR)
	DISP_FUNCTION(MenuFilePicker, "SetProfileSection", SetProfileSection, VT_BSTR, VTS_BSTR VTS_BOOL)
	DISP_FUNCTION(MenuFilePicker, "AddFolder", AddFolder, VT_BOOL, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(MenuFilePicker, "AddFolderArray", AddFolderArray, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION(MenuFilePicker, "AddRootFile", AddRootFile, VT_BOOL, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(MenuFilePicker, "AddSortOrder", AddSortOrder, VT_EMPTY, VTS_I4 VTS_BOOL)
	DISP_FUNCTION(MenuFilePicker, "ClearSortOrder", ClearSortOrder, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(MenuFilePicker, "StoreTrackPos", StoreTrackPos, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(MenuFilePicker, "ChooseFile", ChooseFile, VT_BOOL, VTS_NONE)
	DISP_FUNCTION(MenuFilePicker, "OverallExcludeFile", OverallExcludeFile, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION(MenuFilePicker, "ExcludeFileFromFolder", ExcludeFileFromFolder, VT_BOOL, VTS_BSTR VTS_BSTR)
	DISP_DEFVALUE(MenuFilePicker, "CurrentFileName")
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IMenuFilePicker to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .ODL file.

// {4DFA7BE1-8484-11D2-A2C3-006097B8DD84}
static const IID IID_IMenuFilePicker =
{ 0x4dfa7be1, 0x8484, 0x11d2, { 0xa2, 0xc3, 0x0, 0x60, 0x97, 0xb8, 0xdd, 0x84 } };

BEGIN_INTERFACE_MAP(MenuFilePicker, CCmdTarget)
	INTERFACE_PART(MenuFilePicker, IID_IMenuFilePicker, Dispatch)
END_INTERFACE_MAP()

// {4DFA7BE2-8484-11D2-A2C3-006097B8DD84}
IMPLEMENT_OLECREATE(MenuFilePicker, "IDETools.MenuFilePicker", 0x4dfa7be2, 0x8484, 0x11d2, 0xa2, 0xc3, 0x0, 0x60, 0x97, 0xb8, 0xdd, 0x84)


/**
	MenuFilePicker automation properties
*/

void MenuFilePicker::OnTrackPosXChanged( void )
{
}

void MenuFilePicker::OnTrackPosYChanged( void )
{
}

long MenuFilePicker::GetOptionFlags( void )
{
	return m_browser.m_options.getFlags();
}

void MenuFilePicker::SetOptionFlags( long nNewValue )
{
	m_browser.m_options.setFlags( nNewValue );
}

long MenuFilePicker::GetFolderLayout( void )
{
	return m_browser.m_options.m_folderLayout;
}

void MenuFilePicker::SetFolderLayout( long nNewValue )
{
	m_browser.m_options.m_folderLayout = (FolderLayout)nNewValue;
}

BSTR MenuFilePicker::GetCurrentFileName( void )
{
	CString selectedFileName( m_browser.m_options.GetSelectedFileName().c_str() );
	return selectedFileName.AllocSysString();
}

void MenuFilePicker::SetCurrentFileName( LPCTSTR pSelectedFileName )
{
	m_browser.m_options.SetSelectedFileName( pSelectedFileName );
}


// automation methods

/**
	Set the profile section where browsing options will be persisted.
	Section layout is: "FolderOptions[\\SubSection]"
*/
BSTR MenuFilePicker::SetProfileSection( LPCTSTR pSubSection, BOOL loadNow )
{
	m_browser.m_options.SetSubSection( pSubSection );

	if ( loadNow )
		m_browser.m_options.LoadAll();

	CString result( m_browser.m_options.GetSection().c_str() );
	return result.AllocSysString();
}

BOOL MenuFilePicker::AddFolder( LPCTSTR folderPathFilter, LPCTSTR folderAlias )
{
	return m_browser.addFolder( folderPathFilter, folderAlias );
}

/**
	'folderItemFlatArray' contains folder items separated by ';' token (at end no token);
	Each folder item contains a folder path filter, and optional, a folder alias separated
	by '|' separator.
	Example:
		"C:\Documents\*.doc;C:\TextFiles\*.txt|My Text Files;C:\Documents\*.bmp,*.dib|Image Files"
*/
BOOL MenuFilePicker::AddFolderArray( LPCTSTR folderItemFlatArray )
{
	return m_browser.addFolderStringArray( folderItemFlatArray );
}

BOOL MenuFilePicker::AddRootFile( LPCTSTR filePath, LPCTSTR label )
{
	return m_browser.addRootFile( filePath, label );
}

void MenuFilePicker::AddSortOrder( long pathField, BOOL exclusive )
{
	ASSERT( pathField >= pfDrive && pathField < pfFieldCount );
	if ( exclusive )
		m_browser.m_options.m_fileSortOrder.Clear();
	m_browser.m_options.m_fileSortOrder.Add( (PathField)pathField );
}

void MenuFilePicker::ClearSortOrder( void )
{
	m_browser.m_options.m_fileSortOrder.Clear();
}

void MenuFilePicker::StoreTrackPos( void )
{
	CPoint trackPos;

	::GetCursorPos( &trackPos );
	m_trackPosX = trackPos.x;
	m_trackPosY = trackPos.y;
}

BOOL MenuFilePicker::ChooseFile( void )
{
	return m_browser.PickFile( CPoint( m_trackPosX, m_trackPosY ) );
}

BOOL MenuFilePicker::OverallExcludeFile( LPCTSTR filePathFilter )
{
	return m_browser.overallExcludeFile( filePathFilter );
}

BOOL MenuFilePicker::ExcludeFileFromFolder( LPCTSTR folderPath, LPCTSTR fileFilter )
{
	return m_browser.excludeFileFromFolder( folderPath, fileFilter );
}
