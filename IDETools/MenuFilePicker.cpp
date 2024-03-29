
#include "pch.h"
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
	__super::OnFinalRelease();
}


// message handlers

BEGIN_MESSAGE_MAP(MenuFilePicker, CCmdTarget)
END_MESSAGE_MAP()


BEGIN_DISPATCH_MAP(MenuFilePicker, CCmdTarget)
	DISP_PROPERTY_NOTIFY_ID(MenuFilePicker, "TrackPosX", dispidTrackPosX, m_trackPosX, OnTrackPosXChanged, VT_I4)
	DISP_PROPERTY_NOTIFY_ID(MenuFilePicker, "TrackPosY", dispidTrackPosY, m_trackPosY, OnTrackPosYChanged, VT_I4)
	DISP_PROPERTY_EX_ID(MenuFilePicker, "OptionFlags", dispidOptionFlags, GetOptionFlags, SetOptionFlags, VT_I4)
	DISP_PROPERTY_EX_ID(MenuFilePicker, "FolderLayout", dispidFolderLayout, GetFolderLayout, SetFolderLayout, VT_I4)
	DISP_PROPERTY_EX_ID(MenuFilePicker, "CurrentFilePath", dispidCurrentFilePath, GetCurrentFilePath, SetCurrentFilePath, VT_BSTR)
	DISP_DEFVALUE_ID(MenuFilePicker, dispidCurrentFilePath)
	DISP_FUNCTION_ID(MenuFilePicker, "SetProfileSection", dispidSetProfileSection, SetProfileSection, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION_ID(MenuFilePicker, "AddFolder", dispidAddFolder, AddFolder, VT_BOOL, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION_ID(MenuFilePicker, "AddFolderArray", dispidAddFolderArray, AddFolderArray, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(MenuFilePicker, "AddRootFile", dispidAddRootFile, AddRootFile, VT_BOOL, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION_ID(MenuFilePicker, "AddSortOrder", dispidAddSortOrder, AddSortOrder, VT_EMPTY, VTS_I4 VTS_BOOL)
	DISP_FUNCTION_ID(MenuFilePicker, "ClearSortOrder", dispidClearSortOrder, ClearSortOrder, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION_ID(MenuFilePicker, "StoreTrackPos", dispidStoreTrackPos, StoreTrackPos, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION_ID(MenuFilePicker, "ChooseFile", dispidChooseFile, ChooseFile, VT_BOOL, VTS_NONE)
END_DISPATCH_MAP()

// Note: we add support for IID_IMenuFilePicker to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .IDL file.

// {4DFA7BE1-8484-11D2-A2C3-006097B8DD84}
static const IID IID_IMenuFilePicker =
{ 0x4dfa7be1, 0x8484, 0x11d2, { 0xa2, 0xc3, 0x0, 0x60, 0x97, 0xb8, 0xdd, 0x84 } };

BEGIN_INTERFACE_MAP(MenuFilePicker, CCmdTarget)
	INTERFACE_PART(MenuFilePicker, IID_IMenuFilePicker, Dispatch)
END_INTERFACE_MAP()

// {4DFA7BE2-8484-11D2-A2C3-006097B8DD84}
IMPLEMENT_OLECREATE(MenuFilePicker, "IDETools.MenuFilePicker", 0x4dfa7be2, 0x8484, 0x11d2, 0xa2, 0xc3, 0x0, 0x60, 0x97, 0xb8, 0xdd, 0x84)


// MenuFilePicker automation properties

void MenuFilePicker::OnTrackPosXChanged( void )
{
}

void MenuFilePicker::OnTrackPosYChanged( void )
{
}

long MenuFilePicker::GetOptionFlags( void )
{
	return m_browser.m_options.GetFlags();
}

void MenuFilePicker::SetOptionFlags( long nNewValue )
{
	m_browser.m_options.SetFlags( nNewValue );
}

long MenuFilePicker::GetFolderLayout( void )
{
	return m_browser.m_options.m_folderLayout;
}

void MenuFilePicker::SetFolderLayout( long nNewValue )
{
	m_browser.m_options.m_folderLayout = (FolderLayout)nNewValue;
}

BSTR MenuFilePicker::GetCurrentFilePath( void )
{
	CString currFilePath( m_browser.GetCurrFilePath().GetPtr() );
	return currFilePath.AllocSysString();
}

void MenuFilePicker::SetCurrentFilePath( LPCTSTR pCurrFilePath )
{
	m_browser.SetCurrFilePath( fs::CPath( pCurrFilePath ) );
}


// automation methods

/**
	Set the profile section where browsing options will be persisted.
	CSection layout is: "FolderOptions[\\SubSection]"
*/
BSTR MenuFilePicker::SetProfileSection( LPCTSTR pSubSection )
{
	m_browser.m_options.SetSubSection( pSubSection );

	CString result( m_browser.m_options.GetSectionName().c_str() );
	return result.AllocSysString();
}

BOOL MenuFilePicker::AddFolder( LPCTSTR pFolderPathFilter, LPCTSTR pFolderAlias )
{
	return m_browser.AddFolder( fs::CPath( pFolderPathFilter ), pFolderAlias );
}

/**
	'pFolderItemFlatArray' contains folder items separated by ';' separator.
	Each folder item contains a folder path filter, and optional, a folder alias separated by '|' separator.
	Example:
		"C:\Documents\*.doc;C:\TextFiles\*.txt|My Text Files;C:\Documents\*.bmp,*.dib|Image Files"
*/
BOOL MenuFilePicker::AddFolderArray( LPCTSTR pFolderItemFlatArray )
{
	return m_browser.AddFolderItems( pFolderItemFlatArray );
}

/*
	Add source files open in Visual Studio.
*/
BOOL MenuFilePicker::AddRootFile( LPCTSTR pFilePath, LPCTSTR pLabel )
{
	return m_browser.AddRootFile( fs::CPath( pFilePath ), pLabel );
}

void MenuFilePicker::AddSortOrder( long pathField, BOOL exclusive )
{
	ASSERT( pathField >= pfDrive && pathField < _pfFieldCount );

	if ( exclusive )
		m_browser.m_options.m_fileSortOrder.Clear();

	m_browser.m_options.m_fileSortOrder.Add( static_cast<PathField>( pathField ) );
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
