
#include "stdafx.h"
#include "UserInterface.h"
#include "IdeUtilities.h"
#include "InputBoxDialog.h"
#include "FileLocatorDialog.h"
#include "StringUtilitiesEx.h"
#include "ModuleSession.h"
#include "resource.h"
#include "utl/Clipboard.h"
#include "utl/Registry.h"
#include "utl/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE(UserInterface, CCmdTarget)


UserInterface::UserInterface()
{
	EnableAutomation();

	// To keep the application running as long as an OLE automation
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();
}

UserInterface::~UserInterface()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.

	AfxOleUnlockApp();
}


void UserInterface::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}


// message handlers

BEGIN_MESSAGE_MAP(UserInterface, CCmdTarget)
	//{{AFX_MSG_MAP(UserInterface)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BEGIN_DISPATCH_MAP(UserInterface, CCmdTarget)
	//{{AFX_DISPATCH_MAP(UserInterface)
	DISP_PROPERTY_EX(UserInterface, "IDEToolsRegistryKey", GetIDEToolsRegistryKey, SetIDEToolsRegistryKey, VT_BSTR)
	DISP_FUNCTION(UserInterface, "InputBox", InputBox, VT_BSTR, VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(UserInterface, "GetClipboardText", GetClipboardText, VT_BSTR, VTS_NONE)
	DISP_FUNCTION(UserInterface, "SetClipboardText", SetClipboardText, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION(UserInterface, "IsClipFormatAvailable", IsClipFormatAvailable, VT_BOOL, VTS_I4)
	DISP_FUNCTION(UserInterface, "IsClipFormatNameAvailable", IsClipFormatNameAvailable, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION(UserInterface, "IsKeyPath", IsKeyPath, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION(UserInterface, "CreateKeyPath", CreateKeyPath, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION(UserInterface, "RegReadString", RegReadString, VT_BSTR, VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(UserInterface, "RegReadNumber", RegReadNumber, VT_I4, VTS_BSTR VTS_BSTR VTS_I4)
	DISP_FUNCTION(UserInterface, "RegWriteString", RegWriteString, VT_BOOL, VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(UserInterface, "RegWriteNumber", RegWriteNumber, VT_BOOL, VTS_BSTR VTS_BSTR VTS_I4)
	DISP_FUNCTION(UserInterface, "EnsureStringValue", EnsureStringValue, VT_BOOL, VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(UserInterface, "EnsureNumberValue", EnsureNumberValue, VT_BOOL, VTS_BSTR VTS_BSTR VTS_I4)
	DISP_FUNCTION(UserInterface, "GetEnvironmentVariable", GetEnvironmentVariable, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION(UserInterface, "SetEnvironmentVariable", SetEnvironmentVariable, VT_BOOL, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(UserInterface, "ExpandEnvironmentVariables", ExpandEnvironmentVariables, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION(UserInterface, "LocateFile", LocateFile, VT_BSTR, VTS_BSTR)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IUserInterface to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .ODL file.

// {216EF194-4C10-11D3-A3C8-006097B8DD84}
static const IID IID_IUserInterface =
{ 0x216ef194, 0x4c10, 0x11d3, { 0xa3, 0xc8, 0x0, 0x60, 0x97, 0xb8, 0xdd, 0x84 } };

BEGIN_INTERFACE_MAP(UserInterface, CCmdTarget)
	INTERFACE_PART(UserInterface, IID_IUserInterface, Dispatch)
END_INTERFACE_MAP()

// {216EF195-4C10-11D3-A3C8-006097B8DD84}
IMPLEMENT_OLECREATE(UserInterface, "IDETools.UserInterface", 0x216ef195, 0x4c10, 0x11d3, 0xa3, 0xc8, 0x0, 0x60, 0x97, 0xb8, 0xdd, 0x84)


// implementation

BSTR UserInterface::GetIDEToolsRegistryKey()
{
	CString appRegKeyName;

	appRegKeyName.Format( _T("HKEY_CURRENT_USER\\Software\\%s\\%s"),
						  AfxGetApp()->m_pszRegistryKey, AfxGetApp()->m_pszProfileName );

	return appRegKeyName.AllocSysString();
}

void UserInterface::SetIDEToolsRegistryKey( LPCTSTR lpszNewValue )
{
	lpszNewValue;
	CString msg;

	msg.Format( IDS_ERR_READONLY_PROPERTY, _T("UserInterface.IDEToolsRegistryKey") );
	AfxMessageBox( msg );
}

BSTR UserInterface::InputBox( LPCTSTR title, LPCTSTR prompt, LPCTSTR initialValue )
{
	CString strResult;
	CInputBoxDialog inputDialog( title, prompt, initialValue, ide::GetRootWindow() );
	if ( IDOK == inputDialog.DoModal() )
		strResult = inputDialog.m_inputText;

	return strResult.AllocSysString();
}


//	Clipboard implementation

BSTR UserInterface::GetClipboardText( void )
{
	std::tstring text;
	if ( !CClipboard::CanPasteText() || !CClipboard::PasteText( text ) )
	{
		ui::BeepSignal( MB_ICONERROR );
		AfxThrowOleException( S_OK );
	}

	return CString( text.c_str() ).AllocSysString();
}

void UserInterface::SetClipboardText( LPCTSTR text )
{
	CClipboard::CopyText( text );
}

BOOL UserInterface::IsClipFormatAvailable( long clipFormat )
{
	return IsClipboardFormatAvailable( clipFormat );
}

BOOL UserInterface::IsClipFormatNameAvailable( LPCTSTR formatName )
{
	return IsClipboardFormatAvailable( RegisterClipboardFormat( formatName ) );
}


// Registry implementation

// Returns TRUE if the specified key path exist, otherwise FALSE.
BOOL UserInterface::IsKeyPath( LPCTSTR keyFullPath )
{
	reg::CKey key( keyFullPath, false );

	return key.IsValid();
}

// Recursively creates the specified path, if not already created.
// Returns the number of created keys.
BOOL UserInterface::CreateKeyPath( LPCTSTR keyFullPath )
{
	reg::CKey key( keyFullPath, true );

	return key.IsValid();
}

BSTR UserInterface::RegReadString( LPCTSTR keyFullPath, LPCTSTR valueName, LPCTSTR defaultString )
{
	reg::CKey key( keyFullPath, false );
	std::tstring stringValue;

	if ( key.IsValid() )
		stringValue = key.ReadString( valueName, defaultString );

	return str:: AllocSysString( stringValue );
}

long UserInterface::RegReadNumber( LPCTSTR keyFullPath, LPCTSTR valueName, long defaultNumber )
{
	reg::CKey key( keyFullPath, false );
	long numValue = 0;

	if ( key.IsValid() )
		numValue = key.ReadNumber( valueName, defaultNumber );

	return numValue;
}

BOOL UserInterface::RegWriteString( LPCTSTR keyFullPath, LPCTSTR valueName, LPCTSTR strValue )
{
	reg::CKey key( keyFullPath, true );

	return key.WriteString( valueName, strValue );
}

BOOL UserInterface::RegWriteNumber( LPCTSTR keyFullPath, LPCTSTR valueName, long numValue )
{
	reg::CKey key( keyFullPath, true );

	return key.WriteNumber( valueName, numValue );
}

// Ensures that a registry string value exists. If doesn't exist, a value is created and assigned with default string.
// Returns TRUE if non-existing value was created and assigned, otherwise FALSE.
BOOL UserInterface::EnsureStringValue( LPCTSTR keyFullPath, LPCTSTR valueName, LPCTSTR defaultString )
{
	reg::CKey key( keyFullPath, true );
	static LPCTSTR testString = _T("_TestString_");

	if ( !key.IsValid() )
	{
		CString msg;

		msg.Format( IDS_ERR_CANNOT_CREATE_REGKEY, keyFullPath );
		AfxMessageBox( msg );
	}
	else if ( key.ReadString( valueName, testString ) == testString )
		return key.WriteString( valueName, defaultString );

	return FALSE;
}

// Ensures that a registry numeric value exists. If doesn't exist, a value is created and assigned with default number.
// Returns TRUE if non-existing value was created and assigned, otherwise FALSE.
BOOL UserInterface::EnsureNumberValue( LPCTSTR keyFullPath, LPCTSTR valueName, long defaultNumber )
{
	reg::CKey key( keyFullPath, true );
	static long testNumber = 1234321;

	if ( !key.IsValid() )
	{
		CString msg;
		msg.Format( IDS_ERR_CANNOT_CREATE_REGKEY, keyFullPath );
		AfxMessageBox( msg );
	}
	else if ( key.ReadNumber( valueName, testNumber ) == testNumber )
		return key.WriteNumber( valueName, defaultNumber );
	return FALSE;
}


// Process environment implementation

BSTR UserInterface::GetEnvironmentVariable( LPCTSTR varName )
{
	CString strResult;

	::GetEnvironmentVariable( varName, strResult.GetBuffer( 1024 ), 1024 );
	strResult.ReleaseBuffer();
	return strResult.AllocSysString();
}

BOOL UserInterface::SetEnvironmentVariable( LPCTSTR varName, LPCTSTR varValue )
{
	return ::SetEnvironmentVariable( varName, varValue );
}

BSTR UserInterface::ExpandEnvironmentVariables( LPCTSTR sourceString )
{
	CString expandedString = sourceString;

	for ( int varStartPos = 0, varLength; ; )
	{
		CString varName = str::findEnvironmentVariable( expandedString, varStartPos, varStartPos, varLength );

		if ( !varName.IsEmpty() )
		{
			CString varValue;
			int charCount =::GetEnvironmentVariable( varName, varValue.GetBuffer( 1024 ), 1024 );

			varValue.ReleaseBuffer();
			if ( charCount > 0 )
			{	// Environment variable successfully substituted by it's value:
				expandedString.Delete( varStartPos, varLength );
				expandedString.Insert( varStartPos, varValue );
				varStartPos += str::Length( varValue );
			}
			else
				// Environment variable not found -> skip it!
				varStartPos += varLength;
		}
		else
			break;
	}

	return expandedString.AllocSysString();
}

BSTR UserInterface::LocateFile( LPCTSTR localDirPath )
{
	std::tstring includeFilePath;
	CFileLocatorDialog dlg( ide::GetRootWindow() );

	dlg.SetLocalDirPath( localDirPath );

	if ( IDOK == dlg.DoModal() )
		includeFilePath = dlg.m_selectedFilePath;

	return str::AllocSysString( includeFilePath );
}
