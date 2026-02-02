
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/ShortcutTests.h"
#include "Shortcut.h"
#include "ComUtils.h"
#include "utl/FlagTags.h"
#include "utl/FmtUtils.h"
#include "utl/ScopedValue.h"
#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	static std::tstring FormatProperty( const PROPERTYKEY& propKey, const PROPVARIANT& propValue )
	{
		std::tstring canonicalName = com::GetPropCanonicalName( propKey );
		std::tstring propertyLabel;

		if ( CComPtr<IPropertyDescription> pPropDescr = com::GetPropertyDescription( canonicalName.c_str() ) )
			propertyLabel = com::GetPropertyLabel( pPropDescr );

		return str::Format( _T("CanonicalName='%s'  PropertyLabel='%s'  DisplayValue='%s'"),
			canonicalName.c_str(), propertyLabel.c_str(), com::FormatPropDisplayValue( propKey, propValue, PDFF_DEFAULT ).c_str() );
	}

	void TraceFileProperties( const TCHAR* pFilePath )
	{
		CComPtr<IPropertyStore> pPropertyStore = com::OpenFileProperties( pFilePath );
		ASSERT_PTR( pPropertyStore );

		CScopedValue<bool> scopedNoTracing( &CTracing::m_hResultDisabled, true );

		std::vector<PROPERTYKEY> propKeys;
		ASSERT( com::QueryProperties( propKeys, pPropertyStore ) );

		com::CPropVariant prop;

		TRACE( _T("\n%d properties in '%s'\n"), propKeys.size(), pFilePath );
		for ( std::vector<PROPERTYKEY>::const_iterator itPropKey = propKeys.begin(); itPropKey != propKeys.end(); ++itPropKey )
		{
			HR_AUDIT( pPropertyStore->GetValue( *itPropKey, &prop ) );
			TRACE( _T("\t%s\n"), ut::FormatProperty( *itPropKey, prop.Get() ).c_str() );
		}
	}
}


CShortcutTests::CShortcutTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

bool CShortcutTests::MakeTestLinkPath( OUT fs::CPath* pLinkPath, TestLink testLink )
{
	ASSERT_PTR( pLinkPath );
	ASSERT( testLink < _LinkCount );

	static const TCHAR* s_linkFilenames[_LinkCount] =
	{
		_T("Dice.png.lnk"), _T("Dice.png_envVar.lnk"), _T("Region_CPL.lnk"), _T("ResourceMonitor_envVar.lnk"),
		_T("xDice.png_bad.lnk"), _T("xDice.png_envVar_bad.lnk")
	};

	fs::CPath linkPath( ( ut::GetShellLinksDirPath() / s_linkFilenames[testLink] ).Get() );

	if ( !linkPath.IsEmpty() && !linkPath.FileExist() )
	{
		TRACE( _T("\n * Cannot find the local test link file: %s\n"), linkPath.GetPtr() );
		return false;
	}
	*pLinkPath = linkPath;
	return true;
}

CComPtr<IShellLink> CShortcutTests::LoadShellLink( TestLink testLink )
{
	CComPtr<IShellLink> pShellLink;
	fs::CPath linkPath;

	if ( CShortcutTests::MakeTestLinkPath( &linkPath, testLink ) )
	{
		ASSERT( shell::IsValidShortcutFile( linkPath.GetPtr() ) );
		pShellLink = shell::LoadLinkFromFile( linkPath.GetPtr() );
	}
	return pShellLink;
}

CShortcutTests& CShortcutTests::Instance( void )
{
	static CShortcutTests s_testCase;
	return s_testCase;
}


void CShortcutTests::TestValidLinks( void )
{
	fs::CPath linkPath;

	if ( MakeTestLinkPath( &linkPath, Dice_png ) )
	{
		ASSERT( shell::IsValidShortcutFile( linkPath.GetPtr() ) );

		CComPtr<IShellLink> pShellLink = shell::LoadLinkFromFile( linkPath.GetPtr() );
		shell::CShortcut shortcut( pShellLink );

		ASSERT( !shortcut.IsTargetEmpty() );
		ASSERT( shortcut.IsTargetValid() );
		ASSERT( shortcut.IsTargetFileSys() );

		ASSERT_EQUAL( _T("TestDataUtl\\images\\Dice.png"), shortcut.GetTargetPath().GetRelativePath( 3 ) );
		ASSERT_EQUAL( _T("TestDataUtl\\images\\Dice.png"), shortcut.GetTargetPidl().ToShellPath().GetRelativePath( 3 ) );
		ASSERT_EQUAL( _T("TestDataUtl\\images"), shortcut.GetWorkDirPath().GetRelativePath( 2 ) );
		ASSERT_EQUAL( _T("-h"), shortcut.GetArguments() );
		ASSERT_EQUAL( _T("Shortcut to PNG file"), shortcut.GetDescription() );
		ASSERT_EQUAL( _T("%SystemRoot%\\System32\\SHELL32.dll"), shortcut.GetIconLocation().m_path );
		ASSERT_EQUAL( 139, shortcut.GetIconLocation().m_index );
		ASSERT_EQUAL( 'M', LOBYTE( shortcut.GetHotKey() ) );
		ASSERT_EQUAL( HOTKEYF_CONTROL | HOTKEYF_SHIFT | HOTKEYF_ALT, HIBYTE( shortcut.GetHotKey() ) );
		ASSERT_EQUAL( SW_SHOWNORMAL, shortcut.GetShowCmd() );

		ASSERT_EQUAL( SLDF_HAS_ID_LIST | SLDF_HAS_LINK_INFO | SLDF_HAS_NAME | SLDF_HAS_RELPATH | SLDF_HAS_WORKINGDIR | SLDF_HAS_ARGS | SLDF_HAS_ICONLOCATION | SLDF_UNICODE | SLDF_ENABLE_TARGET_METADATA,
					  shortcut.GetLinkDataFlags() );
		//TRACE( _T("\nlinkPath='%s' - SHELL_LINK_DATA_FLAGS={%s}\n"), linkPath.GetPtr(), shell::GetTags_ShellLinkDataFlags().FormatKey( shortcut.GetLinkDataFlags() ).c_str() );

		ASSERT_EQUAL( _T("Ctrl + Shift + Alt + M"), fmt::FormatHotKey( shortcut.GetHotKey() ) );

		// pidl name
		{
			const shell::CPidlAbsolute& targetPidl = shortcut.GetTargetPidl();

			ASSERT_EQUAL( _T("Dice.png"), targetPidl.GetName( SIGDN_NORMALDISPLAY ) );
			ASSERT_EQUAL( _T("Dice.png"), targetPidl.GetName( SIGDN_PARENTRELATIVEPARSING ) );
			// local:	ASSERT_EQUAL( _T("C:\\dev\\code\\DevTools\\TestDataUtl\\images\\Dice.png"), targetPidl.GetName( SIGDN_DESKTOPABSOLUTEPARSING ) );	// dependent on username
			ASSERT_EQUAL( _T("Dice.png"), targetPidl.GetName( SIGDN_PARENTRELATIVEEDITING ) );
			// local:	ASSERT_EQUAL( _T("C:\\dev\\code\\DevTools\\TestDataUtl\\images\\Dice.png"), targetPidl.GetName( SIGDN_DESKTOPABSOLUTEEDITING ) );	// dependent on username
			// local:	ASSERT_EQUAL( _T("file:///C:/dev/code/DevTools/TestDataUtl/images/Dice.png"), targetPidl.GetName( SIGDN_URL ) );	// dependent on username
			ASSERT_EQUAL( _T("Dice.png"), targetPidl.GetName( SIGDN_PARENTRELATIVEFORADDRESSBAR ) );
			ASSERT_EQUAL( _T("Dice.png"), targetPidl.GetName( SIGDN_PARENTRELATIVE ) );
			ASSERT_EQUAL( _T("Dice.png"), targetPidl.GetName( SIGDN_PARENTRELATIVEFORUI ) );
		}
	}

	if ( MakeTestLinkPath( &linkPath, Dice_png_envVar ) )
	{
		ASSERT( shell::IsValidShortcutFile( linkPath.GetPtr() ) );

		CComPtr<IShellLink> pShellLink = shell::LoadLinkFromFile( linkPath.GetPtr() );
		shell::CShortcut shortcut( pShellLink );

		ASSERT( !shortcut.IsTargetEmpty() );
		ASSERT( shortcut.IsTargetValid() );
		ASSERT( shortcut.IsTargetFileSys() );

		ASSERT_EQUAL( _T("%UTL_TESTDATA_PATH%\\images\\Dice.png"), shortcut.GetTargetPath() );
		ASSERT_EQUAL( _T("TestDataUtl\\images\\Dice.png"), shortcut.GetTargetPidl().ToShellPath().GetRelativePath( 3 ) );	// evaluated env. var.
		ASSERT_EQUAL( _T("%UTL_TESTDATA_PATH%\\images"), shortcut.GetWorkDirPath() );
		ASSERT_EQUAL( _T("--help"), shortcut.GetArguments() );
		ASSERT_EQUAL( _T("Shortcut to PNG file using environment variable"), shortcut.GetDescription() );
		ASSERT_EQUAL( _T(""), shortcut.GetIconLocation().m_path );
		ASSERT_EQUAL( 0, shortcut.GetIconLocation().m_index );
		ASSERT_EQUAL( 0, shortcut.GetHotKey() );
		ASSERT_EQUAL( SW_SHOWMAXIMIZED, shortcut.GetShowCmd() );

		ASSERT_EQUAL( SLDF_HAS_ID_LIST | SLDF_HAS_LINK_INFO | SLDF_HAS_NAME | SLDF_HAS_RELPATH | SLDF_HAS_WORKINGDIR | SLDF_HAS_ARGS | SLDF_UNICODE | SLDF_HAS_EXP_SZ | SLDF_ENABLE_TARGET_METADATA,
					  shortcut.GetLinkDataFlags() );
		//TRACE( _T("\nlinkPath='%s' - SHELL_LINK_DATA_FLAGS={%s}\n"), linkPath.GetPtr(), shell::GetTags_ShellLinkDataFlags().FormatKey( shortcut.GetLinkDataFlags() ).c_str() );
	}
	// "Control Panel > Region":
	if ( MakeTestLinkPath( &linkPath, Region_CPL ) )
	{
		ASSERT( shell::IsValidShortcutFile( linkPath.GetPtr() ) );

		CComPtr<IShellLink> pShellLink = shell::LoadLinkFromFile( linkPath.GetPtr() );
		shell::CShortcut shortcut( pShellLink );

		ASSERT( !shortcut.IsTargetEmpty() );
		ASSERT( shortcut.IsTargetValid() );
		ASSERT( !shortcut.IsTargetFileSys() );

		ASSERT_EQUAL( _T(""), shortcut.GetTargetPath() );
		ASSERT_EQUAL( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"), shortcut.GetTargetPidl().ToShellPath().Get() );
			ASSERT_EQUAL( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"), shortcut.GetTargetPidl().GetName( SIGDN_DESKTOPABSOLUTEPARSING ) );
			ASSERT_EQUAL( _T("Control Panel\\All Control Panel Items\\Region"), shortcut.GetTargetPidl().GetName( SIGDN_DESKTOPABSOLUTEEDITING ) );
		ASSERT_EQUAL( _T(""), shortcut.GetWorkDirPath() );
		ASSERT_EQUAL( _T(""), shortcut.GetArguments() );
		ASSERT_EQUAL( _T("Shortcut to Control Panel: Region"), shortcut.GetDescription() );
		ASSERT_EQUAL( _T(""), shortcut.GetIconLocation().m_path );
		ASSERT_EQUAL( 0, shortcut.GetIconLocation().m_index );
		ASSERT_EQUAL( 0, shortcut.GetHotKey() );
		ASSERT_EQUAL( SW_SHOWNORMAL, shortcut.GetShowCmd() );

		ASSERT_EQUAL( SLDF_HAS_ID_LIST | SLDF_HAS_NAME | SLDF_UNICODE | SLDF_ENABLE_TARGET_METADATA,
					  shortcut.GetLinkDataFlags() );
		//TRACE( _T("\nlinkPath='%s' - SHELL_LINK_DATA_FLAGS={%s}\n"), linkPath.GetPtr(), shell::GetTags_ShellLinkDataFlags().FormatKey( shortcut.GetLinkDataFlags() ).c_str() );

		// pidl name
		{
			const shell::CPidlAbsolute& targetPidl = shortcut.GetTargetPidl();

			ASSERT_EQUAL( _T("Region"), targetPidl.GetName( SIGDN_NORMALDISPLAY ) );
			ASSERT_EQUAL( _T("::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"), targetPidl.GetName( SIGDN_PARENTRELATIVEPARSING ) );
			ASSERT_EQUAL( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"), targetPidl.GetName( SIGDN_DESKTOPABSOLUTEPARSING ) );
			ASSERT_EQUAL( _T("Region"), targetPidl.GetName( SIGDN_PARENTRELATIVEEDITING ) );
			ASSERT_EQUAL( _T("Control Panel\\All Control Panel Items\\Region"), targetPidl.GetName( SIGDN_DESKTOPABSOLUTEEDITING ) );
			// local:	ASSERT_EQUAL( _T(""), targetPidl.GetName( SIGDN_URL ) );		// hResult=E_NOTIMPL
			ASSERT_EQUAL( _T("Region"), targetPidl.GetName( SIGDN_PARENTRELATIVEFORADDRESSBAR ) );
			ASSERT_EQUAL( _T("Region"), targetPidl.GetName( SIGDN_PARENTRELATIVE ) );
			ASSERT_EQUAL( _T("Region"), targetPidl.GetName( SIGDN_PARENTRELATIVEFORUI ) );
		}
	}

	// link to executable:
	if ( MakeTestLinkPath( &linkPath, ResourceMonitor_envVar ) )
	{
		ASSERT( shell::IsValidShortcutFile( linkPath.GetPtr() ) );

		CComPtr<IShellLink> pShellLink = shell::LoadLinkFromFile( linkPath.GetPtr() );
		shell::CShortcut shortcut( pShellLink );

		ASSERT( !shortcut.IsTargetEmpty() );
		ASSERT( shortcut.IsTargetValid() );
		ASSERT( shortcut.IsTargetFileSys() );

		ASSERT_EQUAL( _T("%SYSTEMROOT%\\System32\\resmon.exe"), shortcut.GetTargetPath() );
		ASSERT_EQUAL( _T("Windows\\System32\\resmon.exe"), shortcut.GetTargetPidl().ToShellPath().GetRelativePath( 3 ) );	// evaluated env. var.
		ASSERT_EQUAL( _T("%SYSTEMROOT%\\System32"), shortcut.GetWorkDirPath() );
		ASSERT_EQUAL( _T("/H"), shortcut.GetArguments() );
		ASSERT_EQUAL( _T("Shortcut to Resource Monitor using environment variable"), shortcut.GetDescription() );
		ASSERT_EQUAL( _T(""), shortcut.GetIconLocation().m_path );
		ASSERT_EQUAL( 0, shortcut.GetIconLocation().m_index );
		ASSERT_EQUAL( 0, shortcut.GetHotKey() );
		ASSERT_EQUAL( SW_SHOWNORMAL, shortcut.GetShowCmd() );

		// pidl name
		{
			const shell::CPidlAbsolute& targetPidl = shortcut.GetTargetPidl();

			ASSERT_EQUAL( _T("resmon.exe"), targetPidl.GetName( SIGDN_NORMALDISPLAY ) );
			ASSERT_EQUAL( _T("resmon.exe"), targetPidl.GetName( SIGDN_PARENTRELATIVEPARSING ) );
			ASSERT_EQUAL( _T("C:\\Windows\\System32\\resmon.exe"), targetPidl.GetName( SIGDN_DESKTOPABSOLUTEPARSING ) );
			ASSERT_EQUAL( _T("resmon.exe"), targetPidl.GetName( SIGDN_PARENTRELATIVEEDITING ) );
			ASSERT_EQUAL( _T("C:\\Windows\\System32\\resmon.exe"), targetPidl.GetName( SIGDN_DESKTOPABSOLUTEEDITING ) );
			ASSERT_EQUAL( _T("file:///C:/Windows/System32/resmon.exe"), targetPidl.GetName( SIGDN_URL ) );
			ASSERT_EQUAL( _T("resmon.exe"), targetPidl.GetName( SIGDN_PARENTRELATIVEFORADDRESSBAR ) );
			ASSERT_EQUAL( _T("resmon.exe"), targetPidl.GetName( SIGDN_PARENTRELATIVE ) );
			ASSERT_EQUAL( _T("resmon.exe"), targetPidl.GetName( SIGDN_PARENTRELATIVEFORUI ) );
		}
	}
}

void CShortcutTests::TestBrokenLinks( void )
{
	fs::CPath linkPath;

	if ( MakeTestLinkPath( &linkPath, xDice_png_bad ) )
	{
		ASSERT( shell::IsValidShortcutFile( linkPath.GetPtr() ) );

		CComPtr<IShellLink> pShellLink = shell::LoadLinkFromFile( linkPath.GetPtr() );
		shell::CShortcut shortcut( pShellLink );

		ASSERT( !shortcut.IsTargetEmpty() );
		ASSERT( !shortcut.IsTargetValid() );

		ASSERT_EQUAL( _T("TestDataUtl\\images\\Dice_NA.png"), shortcut.GetTargetPath().GetRelativePath( 3 ) );
		ASSERT_EQUAL( _T("TestDataUtl\\images\\Dice_NA.png"), shortcut.GetTargetPidl().ToShellPath().GetRelativePath( 3 ) );	// evaluated env. var.
		ASSERT_EQUAL( _T("TestDataUtl\\images\\NA"), shortcut.GetWorkDirPath().GetRelativePath( 3 ) );
		ASSERT_EQUAL( _T(""), shortcut.GetArguments() );
		ASSERT_EQUAL( _T("Broken shortcut to PNG file"), shortcut.GetDescription() );
		ASSERT_EQUAL( _T(""), shortcut.GetIconLocation().m_path );
		ASSERT_EQUAL( 0, shortcut.GetIconLocation().m_index );
		ASSERT_EQUAL( 0, shortcut.GetHotKey() );
		ASSERT_EQUAL( SW_SHOWNORMAL, shortcut.GetShowCmd() );

		// pidl name
		{
			const shell::CPidlAbsolute& targetPidl = shortcut.GetTargetPidl();

			ASSERT_EQUAL( _T("Dice_NA.png"), targetPidl.GetName( SIGDN_NORMALDISPLAY ) );
			ASSERT_EQUAL( _T("Dice_NA.png"), targetPidl.GetName( SIGDN_PARENTRELATIVEPARSING ) );
			//ASSERT_EQUAL( _T("C:\\dev\\code\\DevTools\\TestDataUtl\\images\\Dice_NA.png"), targetPidl.GetName( SIGDN_DESKTOPABSOLUTEPARSING ) );	// dependent on username
			ASSERT_EQUAL( _T("Dice_NA.png"), targetPidl.GetName( SIGDN_PARENTRELATIVEEDITING ) );
			//ASSERT_EQUAL( _T("C:\\dev\\code\\DevTools\\TestDataUtl\\images\\Dice_NA.png"), targetPidl.GetName( SIGDN_DESKTOPABSOLUTEEDITING ) );	// dependent on username
			ASSERT_EQUAL( _T("file:///C:/dev/code/DevTools/TestDataUtl/images/Dice_NA.png"), targetPidl.GetName( SIGDN_URL ) );	// dependent on username
			ASSERT_EQUAL( _T("Dice_NA.png"), targetPidl.GetName( SIGDN_PARENTRELATIVEFORADDRESSBAR ) );
			ASSERT_EQUAL( _T("Dice_NA.png"), targetPidl.GetName( SIGDN_PARENTRELATIVE ) );
			ASSERT_EQUAL( _T("Dice_NA.png"), targetPidl.GetName( SIGDN_PARENTRELATIVEFORUI ) );
		}
	}

	if ( MakeTestLinkPath( &linkPath, xDice_png_envVar_bad ) )
	{
		ASSERT( shell::IsValidShortcutFile( linkPath.GetPtr() ) );

		CComPtr<IShellLink> pShellLink = shell::LoadLinkFromFile( linkPath.GetPtr() );
		shell::CShortcut shortcut( pShellLink );

		ASSERT( !shortcut.IsTargetEmpty() );
		ASSERT( !shortcut.IsTargetValid() );
		ASSERT( shortcut.IsTargetFileSys() );

		ASSERT_EQUAL( _T("%UTL_TESTDATA_PATH%\\images\\Dice_NA.png"), shortcut.GetTargetPath() );
		ASSERT_EQUAL( _T("TestDataUtl\\images\\Dice_NA.png"), shortcut.GetTargetPidl().ToShellPath().GetRelativePath( 3 ) );	// evaluated env. var.
		ASSERT_EQUAL( _T("%UTL_TESTDATA_PATH%\\images\\NA"), shortcut.GetWorkDirPath() );
		ASSERT_EQUAL( _T(""), shortcut.GetArguments() );
		ASSERT_EQUAL( _T("Broken shortcut to PNG file using environment variable"), shortcut.GetDescription() );
		ASSERT_EQUAL( _T(""), shortcut.GetIconLocation().m_path );
		ASSERT_EQUAL( 0, shortcut.GetIconLocation().m_index );
		ASSERT_EQUAL( 0, shortcut.GetHotKey() );
		ASSERT_EQUAL( SW_SHOWMAXIMIZED, shortcut.GetShowCmd() );
	}
}

void CShortcutTests::TestSaveLinkFile( void )
{
	fs::CPath linkPath;
	ASSERT( MakeTestLinkPath( &linkPath, Dice_png_envVar ) );

	CComPtr<IShellLink> pShellLink = shell::LoadLinkFromFile( linkPath.GetPtr() );
	shell::CShortcut srcShortcut( pShellLink );
	ASSERT_EQUAL( _T("%UTL_TESTDATA_PATH%\\images\\Dice.png"), srcShortcut.GetTargetPath() );
	ASSERT_EQUAL( _T("TestDataUtl\\images\\Dice.png"), shell::GetLinkTargetPath( pShellLink, 0 ).GetRelativePath( 3 ) );		// updated to a physical path (expanded environemnt variables)
	ASSERT_EQUAL( _T("--help"), srcShortcut.GetArguments() );

	// save the link to itself: should fail since the .lnk file is read-only
	ASSERT( !shell::SaveLinkToFile( linkPath.GetPtr(), pShellLink ) );	// E_ACCESSDENIED: "General access denied error."

	//ASSERT( HR_OK( pShellLink->Resolve( nullptr, SLR_UPDATE | SLR_NO_UI ) ) );		// IShellLink::Resolve() does not expand the environment variables!

	ut::CTempFilePool pool;		// empty, just create the temp directory (with auto files cleanup)
	fs::CPath destLinkPath = pool.GetPoolDirPath() / _T("newDice.png.lnk");

	ASSERT( shell::SaveLinkToFile( destLinkPath.GetPtr(), pShellLink ) );
	{
		// load the saved dest link
		pShellLink = shell::LoadLinkFromFile( destLinkPath.GetPtr() );
		shell::CShortcut destShortcut( pShellLink );

		ASSERT( destShortcut == srcShortcut );
		ASSERT_EQUAL( _T("--help"), destShortcut.GetArguments() );

		destShortcut.SetArguments( _T("-options") );
		ASSERT( destShortcut.WriteToLink( pShellLink ) );			// apply the changed fields
		ASSERT( shell::SaveLinkToFile( destLinkPath.GetPtr(), pShellLink ) );

		// reload the dest link
		pShellLink = shell::LoadLinkFromFile( destLinkPath.GetPtr() );
		destShortcut.Reset( pShellLink );
		ASSERT_EQUAL( _T("-options"), destShortcut.GetArguments() );
	}

	ut::TraceFileProperties( destLinkPath.GetPtr() );
}

void CShortcutTests::TestCreateLinkFile( void )
{
	shell::CShortcut shortcut;
	const std::tstring description = _T("New shortcut to Windows CharMap executable");

	shortcut.SetTargetPath( fs::CPath( _T("CharMap.exe") ) );
	shortcut.SetDescription( description );

	CComPtr<IShellLink> pShellLink = shell::CreateShellLink();
	ASSERT_PTR( pShellLink );

	shortcut.WriteToLink( pShellLink );
	ASSERT( HR_OK( pShellLink->Resolve( nullptr, SLR_UPDATE | SLR_NO_UI ) ) );

	shortcut.Reset( pShellLink );	// fill-in the resolved fields

	ASSERT_EQUAL( _T("Windows\\system32\\CharMap.exe"), shortcut.GetTargetPath().GetRelativePath( 3 ) );		// updated to a physical path (expanded environemnt variables)
	ASSERT_EQUAL( description, shortcut.GetDescription() );

	ut::CTempFilePool pool;		// empty, just create the temp directory (with auto files cleanup)
	fs::CPath newLinkPath = pool.GetPoolDirPath() / _T("newCharMap.lnk");

	ASSERT( shell::SaveLinkToFile( newLinkPath.GetPtr(), pShellLink ) );
	{
		pShellLink = shell::LoadLinkFromFile( newLinkPath.GetPtr() );
		ASSERT_PTR( pShellLink );

		shortcut.Reset( pShellLink );	// fill-in the loaded fields

		ASSERT_EQUAL( _T("Windows\\system32\\CharMap.exe"), shortcut.GetTargetPath().GetRelativePath( 3 ) );	// updated to a physical path (expanded environemnt variables)
		ASSERT_EQUAL( _T("Windows\\system32\\CharMap.exe"), shortcut.GetTargetPidl().ToShellPath().GetRelativePath( 3 ) );	// evaluated env. var.
		//ASSERT_EQUAL( _T("C:\\Windows\\system32\\CharMap.exe"), shortcut.GetTargetPath() );		// specific path for local OS install
		ASSERT_EQUAL( description, shortcut.GetDescription() );
	}

	ut::TraceFileProperties( newLinkPath.GetPtr() );
}

void CShortcutTests::TestFormatParseLink( void )
{
	fs::CPath linkPath;

	if ( MakeTestLinkPath( &linkPath, Dice_png ) )
	{
		shell::CShortcut shortcut( shell::LoadLinkFromFile( linkPath.GetPtr() ) );

		ASSERT( shortcut.IsTargetFileSys() );
		ASSERT_EQUAL( _T("TestDataUtl\\images\\Dice.png"), shortcut.GetTargetPath().GetRelativePath( 3 ) );
		ASSERT_EQUAL( _T("TestDataUtl\\images\\Dice.png"), shortcut.GetTargetPidl().ToShellPath().GetRelativePath( 3 ) );

		// field I/O
		{
			std::tstring fieldText;
			shell::CShortcut destShortcut;

			fieldText = shortcut.FormatTarget();
			ASSERT_EQUAL_STR( _T("TestDataUtl\\images\\Dice.png"), path::GetRelativePath( fieldText.c_str(), 3 ) );
			destShortcut.StoreTarget( fieldText );
			ASSERT( destShortcut.GetTargetPath() == shortcut.GetTargetPath() );
			ASSERT( destShortcut.GetTargetPidl() == shortcut.GetTargetPidl() );

			ASSERT_EQUAL( _T("%SystemRoot%\\System32\\SHELL32.dll:139"), fieldText = fmt::FormatIconLocation( shortcut ) );
			ASSERT( fmt::ParseIconLocation( destShortcut, fieldText ) );
			ASSERT( destShortcut.GetIconLocation() == shortcut.GetIconLocation() );

			ASSERT_EQUAL( _T("074D (Ctrl + Shift + Alt + M)"), fieldText = fmt::FormatHotKeyValue( shortcut ) );
			ASSERT( fmt::ParseHotKeyValue( destShortcut, fieldText ) );
			ASSERT( destShortcut.GetHotKey() == shortcut.GetHotKey() );

			ASSERT_EQUAL( _T("Normal Window"), fieldText = fmt::FormatShowCmd( shortcut ) );
			ASSERT( fmt::ParseShowCmd( destShortcut, fieldText ) );
			ASSERT( destShortcut.GetShowCmd() == shortcut.GetShowCmd() );

			ASSERT_EQUAL( _T("000800FF"), fieldText = fmt::FormatLinkDataFlags( shortcut ) );
			ASSERT( fmt::ParseLinkDataFlags( destShortcut, fieldText ) );
			ASSERT( destShortcut.GetLinkDataFlags() == shortcut.GetLinkDataFlags() );
		}

		// clipboard I/O
		{
			shell::CShortcut destShortcut;
			std::tstring line = fmt::FormatClipShortcut( linkPath.GetFilename(), shortcut );

			fmt::ParseClipShortcut( destShortcut, line.c_str() );

			ASSERT( destShortcut == shortcut );
			TRACE( _T("Diff fields: {%s}\n"), shell::CShortcut::GetTags_Fields().FormatKey( shortcut.GetDiffFields( destShortcut ) ).c_str() );
		}
	}

	// "Control Panel > Region":
	if ( MakeTestLinkPath( &linkPath, Region_CPL ) )
	{
		shell::CShortcut shortcut( shell::LoadLinkFromFile( linkPath.GetPtr() ) );

		ASSERT( shortcut.IsTargetNonFileSys() );
		ASSERT_EQUAL( _T(""), shortcut.GetTargetPath() );
		ASSERT_EQUAL( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"), shortcut.GetTargetPidl().GetName( SIGDN_DESKTOPABSOLUTEPARSING ) );
		ASSERT_EQUAL( _T("Control Panel\\All Control Panel Items\\Region"), shortcut.GetTargetPidl().GetName( SIGDN_DESKTOPABSOLUTEEDITING ) );

		shell::CShortcut destShortcut;
		std::tstring line = fmt::FormatClipShortcut( linkPath.GetFilename(), shortcut );

		fmt::ParseClipShortcut( destShortcut, line.c_str() );

		ASSERT( destShortcut == shortcut );
		//TRACE( _T("Diff fields: {%s}\n"), shell::CShortcut::GetTags_Fields().FormatKey( shortcut.GetDiffFields( destShortcut ) ).c_str() );
	}
}


void CShortcutTests::TestLinkProperties( void )
{
	fs::CPath linkPath;

	if ( !MakeTestLinkPath( &linkPath, Dice_png ) )
		return;

	CComPtr<IPropertyStore> pPropertyStore = com::OpenFileProperties( linkPath.GetPtr() );
	ASSERT_PTR( pPropertyStore );

	com::CPropVariant prop;
	std::tstring strValue;
	fs::CPath filePath;

	if ( HR_OK( pPropertyStore->GetValue( PKEY_Link_Arguments, &prop ) ) )
	{	// VT_LPWSTR
		ASSERT( prop.GetString( &strValue ) );
		ASSERT_EQUAL( _T("-h"), strValue );
	}
	if ( HR_OK( pPropertyStore->GetValue( PKEY_Link_TargetExtension, &prop ) ) )
	{	// VT_VECTOR | VT_LPWSTR: {".png"}
		std::vector<std::tstring> strExts;

		ASSERT( prop.QueryStrings( &strExts ) );
		ASSERT_EQUAL( 1, strExts.size() );
		ASSERT_EQUAL( _T(".png"), strExts.front() );
	}
	if ( HR_OK( pPropertyStore->GetValue( PKEY_Link_TargetParsingPath, &prop ) ) )
	{	// VT_LPWSTR
		ASSERT( prop.GetFilePath( &filePath ) );
		ASSERT_EQUAL( _T("TestDataUtl\\images\\Dice.png"), filePath.GetRelativePath( 3 ) );
	}
	if ( HR_OK( pPropertyStore->GetValue( PKEY_Link_TargetSFGAOFlags, &prop ) ) )
	{	// VT_UI4
		ULONG sfgaoFlags;
		ASSERT( prop.GetUInt( &sfgaoFlags ) );
		ASSERT_EQUAL( _T("SFGAO_CANCOPY|SFGAO_CANMOVE|SFGAO_CANLINK|SFGAO_CANRENAME|SFGAO_CANDELETE|SFGAO_HASPROPSHEET"), shell::GetTags_SFGAO_Flags().FormatUi( sfgaoFlags ) );
	}

	// empty properties:
	if ( HR_OK( pPropertyStore->GetValue( PKEY_Link_Comment, &prop ) ) )
		ASSERT( prop.IsEmpty() );	// why not "Shortcut to PNG file"?
	if ( HR_OK( pPropertyStore->GetValue( PKEY_Link_Description, &prop ) ) )
		ASSERT( prop.IsEmpty() );
	if ( HR_OK( pPropertyStore->GetValue( PKEY_Link_DateVisited, &prop ) ) )
		ASSERT( prop.IsEmpty() );
	if ( HR_OK( pPropertyStore->GetValue( PKEY_Link_FeedItemLocalId, &prop ) ) )
		ASSERT( prop.IsEmpty() );
	if ( HR_OK( pPropertyStore->GetValue( PKEY_Link_Status, &prop ) ) )
		ASSERT( prop.IsEmpty() );
	if ( HR_OK( pPropertyStore->GetValue( PKEY_Link_TargetUrlHostName, &prop ) ) )
		ASSERT( prop.IsEmpty() );
	if ( HR_OK( pPropertyStore->GetValue( PKEY_Link_TargetUrlPath, &prop ) ) )
		ASSERT( prop.IsEmpty() );
}

void CShortcutTests::TestEnumLinkProperties( void )
{
	fs::CPath linkPath;

	ASSERT( MakeTestLinkPath( &linkPath, Dice_png ) );
	ut::TraceFileProperties( linkPath.GetPtr() );
}


void CShortcutTests::Run( void )
{
	RUN_TEST( TestValidLinks );
	RUN_TEST( TestBrokenLinks );
	RUN_TEST( TestSaveLinkFile );
	RUN_TEST( TestCreateLinkFile );
	RUN_TEST( TestFormatParseLink );
	RUN_TEST( TestLinkProperties );
	RUN_TEST( TestEnumLinkProperties );
}


#endif //USE_UT
