
#include "pch.h"
#include "UserInterface.h"
#include "IdeUtilities.h"
#include "InputBoxDialog.h"
#include "FileLocatorDialog.h"
#include "StringUtilitiesEx.h"
#include "ModuleSession.h"
#include "Application.h"
#include "resource.h"
#include "utl/Registry.h"
#include "utl/TextClipboard.h"
#include "utl/UI/WndUtils.h"
#include <iostream>
#include <conio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	int ReadChar( void )
	{
		int ch = ::_getch();
		switch ( ch )
		{
			case 0:			// function key
			case 0xE0:		// arrow key
				ch = ::_getch();
				break;
		}
		return ch;
	}
}


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

	__super::OnFinalRelease();
}


// message handlers

BEGIN_MESSAGE_MAP(UserInterface, CCmdTarget)
END_MESSAGE_MAP()


BEGIN_DISPATCH_MAP(UserInterface, CCmdTarget)
	DISP_PROPERTY_EX_ID(UserInterface, "IDEToolsRegistryKey", dispidIDEToolsRegistryKey, GetIDEToolsRegistryKey, SetIDEToolsRegistryKey, VT_BSTR)
	DISP_FUNCTION_ID(UserInterface, "RunUnitTests", dispidRunUnitTests, RunUnitTests, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION_ID(UserInterface, "InputBox", dispidInputBox, InputBox, VT_BSTR, VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION_ID(UserInterface, "GetClipboardText", dispidGetClipboardText, GetClipboardText, VT_BSTR, VTS_NONE)
	DISP_FUNCTION_ID(UserInterface, "SetClipboardText", dispidSetClipboardText, SetClipboardText, VT_EMPTY, VTS_BSTR)
	DISP_FUNCTION_ID(UserInterface, "IsClipFormatAvailable", dispidIsClipFormatAvailable, IsClipFormatAvailable, VT_BOOL, VTS_I4)
	DISP_FUNCTION_ID(UserInterface, "IsClipFormatNameAvailable", dispidIsClipFormatNameAvailable, IsClipFormatNameAvailable, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(UserInterface, "IsKeyPath", dispidIsKeyPath, IsKeyPath, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(UserInterface, "CreateKeyPath", dispidCreateKeyPath, CreateKeyPath, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(UserInterface, "RegReadString", dispidRegReadString, RegReadString, VT_BSTR, VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION_ID(UserInterface, "RegReadNumber", dispidRegReadNumber, RegReadNumber, VT_I4, VTS_BSTR VTS_BSTR VTS_I4)
	DISP_FUNCTION_ID(UserInterface, "RegWriteString", dispidRegWriteString, RegWriteString, VT_BOOL, VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION_ID(UserInterface, "RegWriteNumber", dispidRegWriteNumber, RegWriteNumber, VT_BOOL, VTS_BSTR VTS_BSTR VTS_I4)
	DISP_FUNCTION_ID(UserInterface, "EnsureStringValue", dispidEnsureStringValue, EnsureStringValue, VT_BOOL, VTS_BSTR VTS_BSTR VTS_BSTR)
	DISP_FUNCTION_ID(UserInterface, "EnsureNumberValue", dispidEnsureNumberValue, EnsureNumberValue, VT_BOOL, VTS_BSTR VTS_BSTR VTS_I4)
	DISP_FUNCTION_ID(UserInterface, "GetEnvironmentVariable", dispidGetEnvironmentVariable, GetEnvironmentVariable, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION_ID(UserInterface, "SetEnvironmentVariable", dispidSetEnvironmentVariable, SetEnvironmentVariable, VT_BOOL, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION_ID(UserInterface, "ExpandEnvironmentVariables", dispidExpandEnvironmentVariables, ExpandEnvironmentVariables, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION_ID(UserInterface, "LocateFile", dispidLocateFile, LocateFile, VT_BSTR, VTS_BSTR)
END_DISPATCH_MAP()

// Note: we add support for IID_IUserInterface to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .IDL file.

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

void UserInterface::RunUnitTests( void )
{
	CApplication::GetApp()->OnRunUnitTests();

	std::clog << std::endl << "Press any key to continue . . .";
	ut::ReadChar();
	std::clog << std::endl;
}

BSTR UserInterface::InputBox( LPCTSTR title, LPCTSTR prompt, LPCTSTR initialValue )
{
	ide::CScopedWindow scopedIDE;
	CInputBoxDialog inputDialog( title, prompt, initialValue, scopedIDE.GetMainWnd() );

	CString strResult;
	if ( IDOK == inputDialog.DoModal() )
		strResult = inputDialog.m_inputText;

	return strResult.AllocSysString();
}


//	Clipboard implementation

BSTR UserInterface::GetClipboardText( void )
{
	std::tstring text;
	if ( !CTextClipboard::CanPasteText() || !CTextClipboard::PasteText( text, AfxGetMainWnd()->GetSafeHwnd() ) )
	{
		ui::BeepSignal( MB_ICONERROR );
		AfxThrowOleException( S_OK );
	}

	return CString( text.c_str() ).AllocSysString();
}

void UserInterface::SetClipboardText( LPCTSTR text )
{
	CTextClipboard::CopyText( text, AfxGetMainWnd()->GetSafeHwnd() );
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

// returns TRUE if the specified key path exist, otherwise FALSE
BOOL UserInterface::IsKeyPath( LPCTSTR pKeyFullPath )
{
	reg::CKey key;
	return reg::OpenKey( &key, pKeyFullPath, KEY_READ );
}

// recursively creates the specified path, if not already created; returns TRUE if the key was created.
BOOL UserInterface::CreateKeyPath( LPCTSTR pKeyFullPath )
{
	reg::CKey key;
	return reg::CreateKey( &key, pKeyFullPath );
}

BSTR UserInterface::RegReadString( LPCTSTR pKeyFullPath, LPCTSTR pValueName, LPCTSTR pDefaultString )
{
	std::tstring stringValue;

	reg::CKey key;
	if ( reg::OpenKey( &key, pKeyFullPath, KEY_READ ) )
		stringValue = key.ReadStringValue( pValueName, pDefaultString );

	return str::AllocSysString( stringValue );
}

long UserInterface::RegReadNumber( LPCTSTR pKeyFullPath, LPCTSTR pValueName, long defaultNumber )
{
	long numValue = 0;
	reg::CKey key;
	if ( reg::OpenKey( &key, pKeyFullPath, KEY_READ ) )
		numValue = key.ReadNumberValue( pValueName, defaultNumber );

	return numValue;
}

BOOL UserInterface::RegWriteString( LPCTSTR pKeyFullPath, LPCTSTR pValueName, LPCTSTR pStrValue )
{
	reg::CKey key;
	return
		reg::CreateKey( &key, pKeyFullPath ) &&
		key.WriteStringValue( pValueName, pStrValue );
}

BOOL UserInterface::RegWriteNumber( LPCTSTR pKeyFullPath, LPCTSTR pValueName, long numValue )
{
	reg::CKey key;
	return
		reg::CreateKey( &key, pKeyFullPath ) &&
		key.WriteNumberValue( pValueName, numValue );
}

// Ensures that a registry string value exists. If doesn't exist, a value is created and assigned with default string.
// Returns TRUE if non-existing value was created and assigned, otherwise FALSE.
BOOL UserInterface::EnsureStringValue( LPCTSTR pKeyFullPath, LPCTSTR pValueName, LPCTSTR pDefaultString )
{
	static std::tstring s_testString = _T("_TestString_");
	reg::CKey key;

	if ( !reg::CreateKey( &key, pKeyFullPath ) )
		AfxMessageBox( str::Format( _T("Cannot create registry key: %s"), pKeyFullPath ).c_str() );
	else if ( key.ReadStringValue( pValueName, s_testString ) == s_testString )
		return key.WriteStringValue( pValueName, pDefaultString );

	return FALSE;
}

// Ensures that a registry numeric value exists. If doesn't exist, a value is created and assigned with default number.
// Returns TRUE if non-existing value was created and assigned, otherwise FALSE.
BOOL UserInterface::EnsureNumberValue( LPCTSTR pKeyFullPath, LPCTSTR pValueName, long defaultNumber )
{
	static long s_testNumber = 1234321;
	reg::CKey key;

	if ( !reg::CreateKey( &key, pKeyFullPath ) )
		AfxMessageBox( str::Format( _T("Cannot create registry key: %s"), pKeyFullPath ).c_str() );
	else if ( key.ReadNumberValue( pValueName, s_testNumber ) == s_testNumber )
		return key.WriteNumberValue( pValueName, defaultNumber );

	return FALSE;
}


// Process environment implementation

BSTR UserInterface::GetEnvironmentVariable( LPCTSTR varName )
{
	CString strResult = env::GetVariableValue( varName ).c_str();

	return strResult.AllocSysString();
}

BOOL UserInterface::SetEnvironmentVariable( LPCTSTR varName, LPCTSTR varValue )
{
	return ::SetEnvironmentVariable( varName, varValue );
}

BSTR UserInterface::ExpandEnvironmentVariables( LPCTSTR sourceString )
{
	CString expandedString = env::ExpandPaths( sourceString ).c_str();

	return expandedString.AllocSysString();
}

BSTR UserInterface::LocateFile( LPCTSTR localDirPath )
{
	ide::CScopedWindow scopedIDE;
	CFileLocatorDialog dlg( scopedIDE.GetMainWnd() );

	dlg.SetLocalDirPath( localDirPath );

	std::tstring includeFilePath;
	if ( IDOK == dlg.DoModal() )
		includeFilePath = dlg.m_selectedFilePath;

	return str::AllocSysString( includeFilePath );
}
