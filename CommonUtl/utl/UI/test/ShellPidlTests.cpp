
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/ShellPidlTests.h"
#include "ShellPidl.h"
#include "ShellContextMenuHost.h"
#include "utl/FlagTags.h"
#include "utl/TextClipboard.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	static const CValueTags& GetTags_SIGDN( void )
	{
		static const CValueTags::ValueDef s_valueDefs[] =
		{
			{ VALUE_TAG( SIGDN_NORMALDISPLAY ) },
			{ VALUE_TAG( SIGDN_PARENTRELATIVEPARSING ) },
			{ VALUE_TAG( SIGDN_DESKTOPABSOLUTEPARSING ) },
			{ VALUE_TAG( SIGDN_PARENTRELATIVEEDITING ) },
			{ VALUE_TAG( SIGDN_DESKTOPABSOLUTEEDITING ) },
			{ VALUE_TAG( SIGDN_FILESYSPATH ) },
			{ VALUE_TAG( SIGDN_URL ) },
			{ VALUE_TAG( SIGDN_PARENTRELATIVEFORADDRESSBAR ) },
			{ VALUE_TAG( SIGDN_PARENTRELATIVE ) },
			{ VALUE_TAG( SIGDN_PARENTRELATIVEFORUI ) }
		};

		static const CValueTags s_tags( ARRAY_SPAN( s_valueDefs ) );
		return s_tags;
	}

	void TracePidlNames( LPCITEMIDLIST pidl )
	{
		static const SIGDN sigdnType[] =
		{
			SIGDN_NORMALDISPLAY, SIGDN_PARENTRELATIVEPARSING, SIGDN_DESKTOPABSOLUTEPARSING, SIGDN_PARENTRELATIVEEDITING, SIGDN_DESKTOPABSOLUTEEDITING,
			SIGDN_FILESYSPATH, SIGDN_URL, SIGDN_PARENTRELATIVEFORADDRESSBAR, SIGDN_PARENTRELATIVE, SIGDN_PARENTRELATIVEFORUI
		};

		TRACE( "\n" );
		for ( size_t i = 0; i != COUNT_OF( sigdnType ); ++i )
			TRACE( _T("%s:\t\"%s\"\n"),
				   GetTags_SIGDN().FormatKey( sigdnType[i] ).c_str(),
				   shell::pidl::GetName(reinterpret_cast<PCIDLIST_ABSOLUTE>( pidl ), sigdnType[i] ).c_str() );
	}

	int TrackContextMenu( IContextMenu* pCtxMenu )
	{
		int cmdId = 0;

		if ( pCtxMenu != nullptr )
		{
			CTextClipboard::CMessageWnd msgWnd;
			CShellContextMenuHost menuHost( CWnd::FromHandle( msgWnd.GetWnd() ), pCtxMenu );

			cmdId = menuHost.TrackMenu( CPoint( 300, 100 ) );
		}
		else
			ASSERT( false );

		return cmdId;
	}
}

CShellPidlTests::CShellPidlTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CShellPidlTests& CShellPidlTests::Instance( void )
{
	static CShellPidlTests s_testCase;
	return s_testCase;
}

void CShellPidlTests::TestNullAndEmptyPidl( void )
{
	fs::TDirPath tempDirPath = fs::GetTempDirPath();

	{
		shell::CPidlAbsolute pidl;

		ASSERT( pidl.IsNull() );
		pidl.SetEmpty();
		ASSERT( pidl.IsEmpty() );
		ASSERT( !pidl.HasValue() );
		ASSERT_EQUAL( _T("Desktop"), pidl.ToShellPath().GetFilename() );

		ASSERT( pidl.CreateFromPath( tempDirPath.GetPtr() ) );
		ASSERT( !pidl.IsEmpty() );
		ASSERT( pidl.HasValue() );
		ASSERT_EQUAL( tempDirPath, pidl.ToShellPath() );

		{
			shell::CPidlAbsolute pidl2;

			ASSERT( pidl2.CreateFromShellPath( tempDirPath.GetPtr() ) );

			ASSERT_EQUAL( pidl.ToShellPath(), pidl2.ToShellPath() );
			// CreateFromPath() and CreateFromShellPath() created different PIDs (bitwise), but are equivalent in parsing name:
			ASSERT( pidl != pidl2 );
		}

		{
			shell::CPidlAbsolute copyPidl( pidl );		// copy constructor

			ASSERT( !pidl.IsNull() && copyPidl.Get() != pidl.Get() );
			ASSERT_EQUAL( tempDirPath, copyPidl.ToShellPath() );
		}

		{
			shell::CPidlAbsolute copyPidl;

			copyPidl = pidl;	// copy assignment
			ASSERT( !pidl.IsNull() && copyPidl.Get() != pidl.Get() );
			ASSERT_EQUAL( tempDirPath, copyPidl.ToShellPath() );
		}
	}

	{
		CComPtr<IShellFolder> pDesktopFolder = shell::GetDesktopFolder();
		ASSERT_PTR( pDesktopFolder );

		shell::CPidlAbsolute desktopPidl;

		ASSERT( desktopPidl.CreateFromFolder( pDesktopFolder ) );
		ASSERT( desktopPidl.IsEmpty() );		// Desktop == empty

		shell::CPidlAbsolute desktopPidl2;

		ASSERT( desktopPidl2.CreateFrom( pDesktopFolder ) );
		ASSERT( desktopPidl2.IsEmpty() );		// Desktop == empty
		ASSERT( desktopPidl == desktopPidl2 );
	}
}

void CShellPidlTests::TestShellPidl( void )
{
	ut::CTempFilePool pool( _T("fa.txt|d1\\fb.txt") );
	const fs::TDirPath& poolDirPath = pool.GetPoolDirPath();

	CComPtr<IShellFolder> pDesktopFolder = shell::GetDesktopFolder();
	ASSERT_PTR( pDesktopFolder );

	shell::CPidlAbsolute desktopPidl;
	ASSERT( desktopPidl.CreateFromFolder( pDesktopFolder ) );
	fs::TDirPath desktopPath = desktopPidl.ToShellPath();

	shell::CPidlAbsolute poolDirPidl;
	{
		ASSERT( poolDirPidl.IsNull() );
		poolDirPidl.SetEmpty();
		ASSERT( poolDirPidl.IsEmpty() );
		ASSERT( !poolDirPidl.HasValue() );

		ASSERT( poolDirPidl.CreateFromShellPath( poolDirPath.GetPtr() ) );
		ASSERT( !poolDirPidl.IsEmpty() );
		ASSERT( poolDirPidl.HasValue() );
		ASSERT_EQUAL( poolDirPath, poolDirPidl.ToShellPath() );
	}

	{
		shell::CPidlAbsolute poolDirPidl2;
		poolDirPidl2.AssignCopy( poolDirPidl );
		ASSERT( poolDirPidl == poolDirPidl2 );				// absolute compare
	}

	CComPtr<IShellFolder> pPoolFolder = poolDirPidl.OpenFolder();
	ASSERT_PTR( pPoolFolder );

	shell::CPidlAbsolute pidl;
	{
		ASSERT( pidl.CreateFrom( pDesktopFolder ) );
		ASSERT_EQUAL( desktopPath, pidl.ToShellPath() );

		ASSERT( pidl.CreateFrom( pPoolFolder ) );
		ASSERT_EQUAL( poolDirPath, pidl.ToShellPath() );
	}

	// FILE[0]
	{
		const fs::CPath& filePath = pool.GetFilePaths()[0];

		shell::CPidlAbsolute filePidl( filePath.GetPtr() );

		ASSERT( !filePidl.IsNull() );
		ASSERT( filePidl.GetItemCount() > 1 );
		ASSERT_EQUAL( filePath, filePidl.ToShellPath() );
		ASSERT_EQUAL( _T("fa.txt"), filePidl.GetFilename() );		// filePath.GetFilename()
		ASSERT_EQUAL( _T("temp_ut\\_OUT\\fa.txt"), filePidl.ToShellPath().GetRelativePath( 3 ) );

		{	// last item ID
			shell::CPidlChild lastPidl;
			lastPidl.AssignCopy( filePidl.GetLastItem() );
			ASSERT_EQUAL( _T("fa.txt"), lastPidl.GetFilename() );
		}

		{	// child PIDL
			shell::CPidlRelative itemPidl;
			ASSERT( itemPidl.CreateRelative( pPoolFolder, filePath.GetFilenamePtr() ) );
			ASSERT_EQUAL( 1, itemPidl.GetItemCount() );
			ASSERT_EQUAL( desktopPath / itemPidl.GetFilename(), itemPidl.ToShellPath() );		// strangely, for child PIDLs the desktop directory is implicitly prepended
			ASSERT_EQUAL( _T("fa.txt"), itemPidl.GetFilename() );		// filePath.GetFilename()

			// shell item from child PIDL relative to parent folder
			CComPtr<IShellItem> pChildFileItem = itemPidl.OpenShellItem( pPoolFolder );
			ASSERT_PTR( pChildFileItem );
			ASSERT_EQUAL( filePath, shell::GetItemShellPath( pChildFileItem ) );

			ASSERT( pidl.CreateFrom( pChildFileItem ) );
			ASSERT_EQUAL( filePath, pidl.ToShellPath() );
		}

		{	// shell item
			CComPtr<IShellItem> pFileItem = filePidl.OpenShellItem();
			ASSERT_PTR( pFileItem );
			shell::TPath itemPath = shell::GetItemShellPath( pFileItem );
			ASSERT_EQUAL( filePath, itemPath );

			ASSERT( pidl.CreateFrom( pFileItem ) );
			ASSERT_EQUAL( filePath, pidl.ToShellPath() );
		}

		shell::CPidlAbsolute copyPidl = filePidl;	// copy assignment

		ASSERT( !pidl.IsNull() && copyPidl.Get() != filePidl.Get() );
		ASSERT( !copyPidl.IsEmpty() );
		ASSERT_EQUAL( filePath, copyPidl.ToShellPath() );
	}

	// FILE[1]
	{
		const fs::CPath& filePath = pool.GetFilePaths()[1];

		shell::CPidlAbsolute filePidl;
		ASSERT( filePidl.CreateFromShellPath( filePath.GetPtr() ) );
		ASSERT_EQUAL( filePath, filePidl.ToShellPath() );
		ASSERT_EQUAL( _T("fb.txt"), filePidl.GetFilename() );		// filePath.GetFilename()
		ASSERT_EQUAL( _T("temp_ut\\_OUT\\d1\\fb.txt"), filePidl.ToShellPath().GetRelativePath( 4 ) );

		shell::CPidlRelative fileRelPidl;
		fileRelPidl.AssignCopy( ::ILFindChild( poolDirPidl, filePidl ) );

		ASSERT_EQUAL( _T("fb.txt"), fileRelPidl.ToFilename( pPoolFolder ) );
	}

	{	// relative PIDL
		static const TCHAR s_relFilePath[] = _T("d1\\fb.txt");		// relative to pool folder

		shell::CPidlRelative relativePidl;
		ASSERT( relativePidl.CreateRelative( pPoolFolder, s_relFilePath ) );
		ASSERT_EQUAL( 2, relativePidl.GetItemCount() );
		ASSERT_EQUAL( _T("fb.txt"), relativePidl.GetFilename() );	// don't know how to find the actual relative path

		//ASSERT_EQUAL( _T("C:\\Users\\Paul\\Desktop\\d1\\fb.txt"), relativePidl.ToShellPath() );		// depends on user name on the computer
	}
}

void CShellPidlTests::TestFolderRelativePidls( void )
{
	ut::CTempFilePool pool( _T("a.txt|d1\\b.txt|d2\\c.txt|d2\\sub3\\d.txt") );

	{
		shell::CFolderRelativePidls multiPidls;
		multiPidls.Build( pool.GetFilePaths() );

		ASSERT( !multiPidls.IsEmpty() );
		ASSERT_EQUAL( 4, multiPidls.GetAbsolutePidls().size() );
		ASSERT_EQUAL( 4, multiPidls.GetRelativePidls().size() );
		ASSERT_PTR( multiPidls.GetAncestorFolder() );

		ASSERT_EQUAL( _T("temp_ut\\_OUT"), shell::GetFolderPath( multiPidls.GetAncestorFolder() ).GetRelativePath( 2 ) );

		ASSERT_EQUAL( _T("a.txt"), multiPidls.GetRelativePathAt( 0 ) );
		ASSERT_EQUAL( _T("d1\\b.txt"), multiPidls.GetRelativePathAt( 1 ) );
		ASSERT_EQUAL( _T("d2\\c.txt"), multiPidls.GetRelativePathAt( 2 ) );
		ASSERT_EQUAL( _T("d2\\sub3\\d.txt"), multiPidls.GetRelativePathAt( 3 ) );

		//ut::TrackContextMenu( multiPidls.MakeContextMenu( nullptr ) );
	}

	{
		shell::CPidlAbsolute cpRegionPidl( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}") );

		ASSERT_EQUAL( _T("Control Panel\\All Control Panel Items\\Region"), cpRegionPidl.GetEditingName() );

		std::vector<shell::TPath> shellPaths;
		shellPaths.push_back( cpRegionPidl.ToShellPath() );

		shell::CFolderRelativePidls multiPidls;
		multiPidls.Build( shellPaths );			// single virtual folder

		// "Control Panel" single virtual folder:
		{
			ASSERT( !multiPidls.IsEmpty() );
			ASSERT_EQUAL( 1, multiPidls.GetAbsolutePidls().size() );
			ASSERT_EQUAL( 1, multiPidls.GetRelativePidls().size() );
			ASSERT_PTR( multiPidls.GetAncestorFolder() );

			ASSERT_EQUAL( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0"), shell::GetFolderPath( multiPidls.GetAncestorFolder() ) );
			ASSERT_EQUAL( _T("::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"), multiPidls.GetRelativePathAt( 0 ) );

			//ut::TrackContextMenu( multiPidls.MakeContextMenu( nullptr ) );
		}

		shell::CPidlAbsolute cpUserAccountsPidl( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{60632754-C523-4B62-B45C-4172DA012619}") );
		ASSERT_EQUAL( _T("Control Panel\\All Control Panel Items\\User Accounts"), cpUserAccountsPidl.GetEditingName() );
		shellPaths.push_back( cpUserAccountsPidl.ToShellPath() );

		multiPidls.Build( shellPaths );
		// "Control Panel" virtual folders:
		{
			ASSERT( !multiPidls.IsEmpty() );
			ASSERT_EQUAL( 2, multiPidls.GetAbsolutePidls().size() );
			ASSERT_EQUAL( 2, multiPidls.GetRelativePidls().size() );
			ASSERT_PTR( multiPidls.GetAncestorFolder() );

			ASSERT_EQUAL( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0"), shell::GetFolderPath( multiPidls.GetAncestorFolder() ) );
			ASSERT_EQUAL( _T("::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"), multiPidls.GetRelativePathAt( 0 ) );
			ASSERT_EQUAL( _T("::{60632754-C523-4B62-B45C-4172DA012619}"), multiPidls.GetRelativePathAt( 1 ) );

			// With 2 virtual folders multiple selection, 'Open' verb in the context menu opens just the first virtual folder, twice!
			// So it can't run the verbs on multiple virtual folders.
			//ut::TrackContextMenu( multiPidls.MakeContextMenu( nullptr ) );
		}

		// mixed file system and "Control Panel" virtual folders:
		shellPaths.insert( shellPaths.end(), pool.GetFilePaths().begin(), pool.GetFilePaths().end() );
		multiPidls.Build( shellPaths );
		{
			ASSERT( !multiPidls.IsEmpty() );
			ASSERT_EQUAL( 6, multiPidls.GetAbsolutePidls().size() );
			ASSERT_EQUAL( 6, multiPidls.GetRelativePidls().size() );
			ASSERT_PTR( multiPidls.GetAncestorFolder() );

			ASSERT_EQUAL( _T(""), multiPidls.GetAncestorPath() );
			ASSERT_EQUAL( _T("This PC"), shell::GetFolderName( multiPidls.GetAncestorFolder() ) );
			ASSERT_EQUAL( _T("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"), shell::GetFolderPath( multiPidls.GetAncestorFolder() ).GetFilename() );	// "This PC"

			ASSERT_EQUAL( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"), multiPidls.GetRelativePathAt( 0 ) );
			ASSERT_EQUAL( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{60632754-C523-4B62-B45C-4172DA012619}"), multiPidls.GetRelativePathAt( 1 ) );

			// no common ancestor folder (except for the Desktop folder), so no relative paths
			ASSERT_EQUAL( _T("a.txt"), multiPidls.GetRelativePathAt( 2 ).GetFilename() );
			ASSERT_EQUAL( _T("b.txt"), multiPidls.GetRelativePathAt( 3 ).GetFilename() );
			ASSERT_EQUAL( _T("c.txt"), multiPidls.GetRelativePathAt( 4 ).GetFilename() );
			ASSERT_EQUAL( _T("d.txt"), multiPidls.GetRelativePathAt( 5 ).GetFilename() );

			//ut::TrackContextMenu( multiPidls.MakeContextMenu( nullptr ) );
		}
	}
}

void CShellPidlTests::TestPidlType( void )
{
	ut::CTempFilePool pool( _T("a.txt|d1\\b.txt") );

	// file (file-system):
	{
		const fs::CPath& filePath = pool.GetFilePaths()[0];
		ASSERT( fs::IsValidFile( filePath.GetPtr() ) );

		shell::CPidlAbsolute filePidl;

		ASSERT( filePidl.CreateFromShellPath( filePath.GetPtr() ) );
		ASSERT( !filePidl.IsSpecialPidl() );

		ASSERT_EQUAL( _T("SFGAO_CANCOPY|SFGAO_CANMOVE|SFGAO_CANLINK|SFGAO_CANRENAME|SFGAO_CANDELETE|SFGAO_HASPROPSHEET|SFGAO_DROPTARGET|SFGAO_STREAM|SFGAO_FILESYSTEM"),
					  shell::GetTags_SFGAO_Flags().FormatUi( filePidl.GetAttributes() ) );
	}

	// directory (file-system):
	{
		fs::TDirPath dirPath = pool.GetFilePaths()[1].GetParentPath();
		ASSERT( fs::IsValidDirectory( dirPath.GetPtr() ) );

		shell::CPidlAbsolute dirPidl;

		ASSERT( dirPidl.CreateFromPath( dirPath.GetPtr() ) );
		ASSERT( !dirPidl.IsSpecialPidl() );

		ASSERT_EQUAL( _T("SFGAO_CANCOPY|SFGAO_CANMOVE|SFGAO_CANLINK|SFGAO_STORAGE|SFGAO_CANRENAME|SFGAO_CANDELETE|SFGAO_HASPROPSHEET|SFGAO_DROPTARGET")
					  _T("|SFGAO_STORAGEANCESTOR|SFGAO_FILESYSANCESTOR|SFGAO_FOLDER|SFGAO_FILESYSTEM"),
					  shell::GetTags_SFGAO_Flags().FormatUi( dirPidl.GetAttributes() ) );
	}

	// non-existent file (file-system, virtual file):
	{
		fs::CPath virtualFilePath = pool.GetFilePaths()[0].GetParentPath() / _T("NA.txt");
		ASSERT( !fs::IsValidFile( virtualFilePath.GetPtr() ) && !fs::IsValidDirectory( virtualFilePath.GetPtr() ) );

		shell::CPidlAbsolute virtualFilePidl( virtualFilePath.GetPtr(), FILE_ATTRIBUTE_NORMAL );

		ASSERT( !virtualFilePidl.IsNull() );
		ASSERT( !virtualFilePidl.IsSpecialPidl() );

		ASSERT_EQUAL( _T(""), shell::GetTags_SFGAO_Flags().FormatUi( virtualFilePidl.GetAttributes() ) );
	}

	// non-existent directory (file-system, virtual file):
	{
		fs::TDirPath virtualDirPath = pool.GetFilePaths()[0].GetParentPath() / _T("NA_DIR");
		ASSERT( !fs::IsValidFile( virtualDirPath.GetPtr() ) && !fs::IsValidDirectory( virtualDirPath.GetPtr() ) );

		shell::CPidlAbsolute virtualDirPidl( virtualDirPath.GetPtr(), FILE_ATTRIBUTE_DIRECTORY );

		ASSERT( !virtualDirPidl.IsNull() );
		ASSERT( !virtualDirPidl.IsSpecialPidl() );

		ASSERT_EQUAL( _T(""), shell::GetTags_SFGAO_Flags().FormatUi( virtualDirPidl.GetAttributes() ) );
	}

	// "Control Panel" special folder (non file-system, valid PIDL):
	{
		shell::CPidlAbsolute controlPanelPidl;

		ASSERT( HR_OK( ::SHGetKnownFolderIDList( FOLDERID_ControlPanelFolder, 0, nullptr, &controlPanelPidl ) ) );
		ASSERT( !controlPanelPidl.IsNull() );
		ASSERT( controlPanelPidl.IsSpecialPidl() );

		ASSERT_EQUAL( _T("Control Panel\\All Control Panel Items"), controlPanelPidl.GetName( SIGDN_DESKTOPABSOLUTEEDITING ) );
		ASSERT_EQUAL( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0"), controlPanelPidl.GetName( SIGDN_DESKTOPABSOLUTEPARSING ) );
		{
			ASSERT_EQUAL( _T("All Control Panel Items"), controlPanelPidl.GetName( SIGDN_NORMALDISPLAY ) );
			ASSERT_EQUAL( _T("0"), controlPanelPidl.GetName( SIGDN_PARENTRELATIVEPARSING ) );
			ASSERT_EQUAL( _T("All Control Panel Items"), controlPanelPidl.GetName( SIGDN_PARENTRELATIVEEDITING ) );
			//ASSERT_EQUAL( _T(""), controlPanelPidl.GetName( SIGDN_FILESYSPATH ) );	// E_INVALIDARG - hResult=0x80070057: 'The parameter is incorrect.'
			//ASSERT_EQUAL( _T(""), controlPanelPidl.GetName( SIGDN_URL ) );			// E_NOTIMPL - hResult=0x80004001: 'Not implemented'
			ASSERT_EQUAL( _T("All Control Panel Items"), controlPanelPidl.GetName( SIGDN_PARENTRELATIVEFORADDRESSBAR ) );
			ASSERT_EQUAL( _T("All Control Panel Items"), controlPanelPidl.GetName( SIGDN_PARENTRELATIVE ) );
		}

		ASSERT_EQUAL( _T("SFGAO_CANLINK|SFGAO_FOLDER|SFGAO_CONTENTSMASK"),
					  shell::GetTags_SFGAO_Flags().FormatUi( controlPanelPidl.GetAttributes() ) );
	}
}

void CShellPidlTests::TestParentPidl( void )
{
	ut::CTempFilePool pool( _T("d1\\b.txt") );

	// file (file-system):
	{
		const fs::CPath& b_txtFilePath = pool.GetFilePaths()[0];
		ASSERT( fs::IsValidFile( b_txtFilePath.GetPtr() ) );

		shell::CPidlAbsolute b_txtPidl( b_txtFilePath.GetPtr() );

		ASSERT( !b_txtPidl.IsNull() );
		ASSERT( !b_txtPidl.IsSpecialPidl() );

		shell::CPidlAbsolute parentPidl;
		shell::CPidlChild childPidl;

		ASSERT( b_txtPidl.GetParentPidl( parentPidl ) );
		ASSERT_EQUAL( _T("temp_ut\\_OUT\\d1"), parentPidl.ToShellPath().GetRelativePath( 3 ) );
		ASSERT( b_txtPidl.GetChildPidl( childPidl ) );
		ASSERT_EQUAL( _T("b.txt"), childPidl.GetFilename() );

		ASSERT( parentPidl.GetChildPidl( childPidl ) );
		ASSERT_EQUAL( _T("d1"), childPidl.GetFilename() );
		ASSERT( parentPidl.GetParentPidl( parentPidl ) );
		ASSERT_EQUAL( _T("temp_ut\\_OUT"), parentPidl.ToShellPath().GetRelativePath( 2 ) );

		ASSERT( parentPidl.GetChildPidl( childPidl ) );
		ASSERT_EQUAL( _T("_OUT"), childPidl.GetFilename() );
		ASSERT( parentPidl.GetParentPidl( parentPidl ) );
		ASSERT_EQUAL( _T("temp_ut"), parentPidl.ToShellPath().GetRelativePath( 1 ) );

		// iterate to root parent
		{
			parentPidl = b_txtPidl;		// copy
			size_t parentLevel = 0;

			TRACE( "\n" );
			do
			{
				TRACE( _T("\tparent[%d]='%s'\n"), parentLevel++, parentPidl.ToShellPath().GetPtr() );
			}
			while ( parentPidl.GetParentPidl( parentPidl ) );

			/*
				parent[0]='C:\dev\code\DevTools\TestDataUtl\temp_ut\_OUT\d1\b.txt'
				parent[1]='C:\dev\code\DevTools\TestDataUtl\temp_ut\_OUT\d1'
				parent[2]='C:\dev\code\DevTools\TestDataUtl\temp_ut\_OUT'
				parent[3]='C:\dev\code\DevTools\TestDataUtl\temp_ut'
				parent[4]='C:\dev\code\DevTools\TestDataUtl'
				parent[5]='C:\dev\code\DevTools'
				parent[6]='C:\dev\code'
				parent[7]='C:\dev'
				parent[8]='C:\'
				parent[9]=''
				parent[10]='C:\Users\Paul\Desktop'
			*/
		}
	}

	// "Control Panel" special folder (non file-system, valid PIDL):
	{
		shell::CPidlAbsolute cpRegionPidl( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}") );

		ASSERT( cpRegionPidl.IsSpecialPidl() );
		ASSERT_EQUAL( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"), cpRegionPidl.ToShellPath() );
		ASSERT_EQUAL( _T("Control Panel\\All Control Panel Items\\Region"), cpRegionPidl.GetName( SIGDN_DESKTOPABSOLUTEEDITING ) );
		ASSERT_EQUAL( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"), cpRegionPidl.GetName( SIGDN_DESKTOPABSOLUTEPARSING ) );

			// Note: name conversion methods GetName(), GetFilename(), ToShellPath() work only for file-system child PIDLs,
			// but not for GUID based child PIDLs, such as a Control Panel applet.
			// Use IShellItem or IShellFolder interfaces for name conversions of the CP applets.

		shell::CPidlAbsolute parentPidl;

		ASSERT( cpRegionPidl.GetParentPidl( parentPidl ) );
		ASSERT( parentPidl.IsSpecialPidl() );
		ASSERT_EQUAL( _T("Control Panel\\All Control Panel Items"), parentPidl.GetName( SIGDN_DESKTOPABSOLUTEEDITING ) );
		ASSERT_EQUAL( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0"), parentPidl.GetName( SIGDN_DESKTOPABSOLUTEPARSING ) );

		ASSERT( parentPidl.GetParentPidl( parentPidl ) );
		ASSERT( parentPidl.IsSpecialPidl() );
		ASSERT_EQUAL( _T("Control Panel"), parentPidl.GetName( SIGDN_DESKTOPABSOLUTEEDITING ) );
		ASSERT_EQUAL( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}"), parentPidl.GetName( SIGDN_DESKTOPABSOLUTEPARSING ) );

		ASSERT( parentPidl.GetParentPidl( parentPidl ) );
		ASSERT( !parentPidl.IsSpecialPidl() );			// root PIDL, which is not a special (GUID) PIDL
		ASSERT_EQUAL( _T("Desktop"), parentPidl.GetName( SIGDN_DESKTOPABSOLUTEEDITING ) );
		//ASSERT_EQUAL( _T("C:\\Users\\Paul\\Desktop"), parentPidl.GetName( SIGDN_DESKTOPABSOLUTEPARSING ) );		// local-dependent

		ASSERT( !parentPidl.GetParentPidl( parentPidl ) );		// no parent of Desktop root
	}
}

void CShellPidlTests::TestCommonAncestorPidl( void )
{
	ut::CTempFilePool pool( _T("A.txt|B.txt|d1\\C.txt|d2\\D.txt|d2\\sub3\\E.txt") );
	shell::CPidlAbsolute ancestorPidl;

	// file-system PIDLs
	{
		fs::CPath filePath1 = pool.GetFilePathAt( 0 ), filePath2;
		ASSERT_EQUAL( _T("A.txt"), filePath1.GetFilename() );

		shell::CPidlAbsolute pidl1( filePath1.GetPtr() );
		shell::CPidlAbsolute pidl2;
		// "A.txt" and "B.txt"
		{
			filePath2 = pool.GetFilePathAt( 1 );
			ASSERT_EQUAL( _T("B.txt"), filePath2.GetFilename() );

			pidl2.CreateFromPath( filePath2.GetPtr() );

			ancestorPidl.Reset( shell::pidl::GetCommonAncestor( pidl1, pidl2 ) );

			ASSERT_EQUAL( pool.GetPoolDirPath(), ancestorPidl.ToShellPath() );
			ASSERT_EQUAL( _T("_OUT"), ancestorPidl.ToShellPath().GetFilename() );		// e.g. "C:\\dev\\code\\DevTools\\TestDataUtl\\temp_ut\\_OUT"
		}
		// "A.txt" and "d1\C.txt"
		{
			filePath2 = pool.GetFilePathAt( 2 ); ASSERT_EQUAL( _T("C.txt"), filePath2.GetFilename() );

			pidl1.CreateFromPath( filePath1.GetPtr() );
			pidl2.CreateFromPath( filePath2.GetPtr() );

			ancestorPidl.Reset( shell::pidl::GetCommonAncestor( pidl1, pidl2 ) );
			ASSERT_EQUAL( _T("_OUT"), ancestorPidl.ToShellPath().GetFilename() );

			ancestorPidl.Reset( shell::pidl::GetCommonAncestor( pidl2, pidl1 ) );		// swap 1 and 2
			ASSERT_EQUAL( _T("_OUT"), ancestorPidl.ToShellPath().GetFilename() );
		}
		// "d1\C.txt" and "d2\\D.txt"
		{
			filePath1 = pool.GetFilePathAt( 2 ); ASSERT_EQUAL( _T("C.txt"), filePath1.GetFilename() );
			filePath2 = pool.GetFilePathAt( 3 ); ASSERT_EQUAL( _T("D.txt"), filePath2.GetFilename() );

			pidl1.CreateFromPath( filePath1.GetPtr() );
			pidl2.CreateFromPath( filePath2.GetPtr() );

			ancestorPidl.Reset( shell::pidl::GetCommonAncestor( pidl1, pidl2 ) );
			ASSERT_EQUAL( _T("_OUT"), ancestorPidl.ToShellPath().GetFilename() );

			ancestorPidl.Reset( shell::pidl::GetCommonAncestor( pidl2, pidl1 ) );		// swap 1 and 2
			ASSERT_EQUAL( _T("_OUT"), ancestorPidl.ToShellPath().GetFilename() );
		}
		// "d2\D.txt" and "d2\sub3\E.txt"
		{
			filePath1 = pool.GetFilePathAt( 3 ); ASSERT_EQUAL( _T("D.txt"), filePath1.GetFilename() );
			filePath2 = pool.GetFilePathAt( 4 ); ASSERT_EQUAL( _T("E.txt"), filePath2.GetFilename() );

			pidl1.CreateFromPath( filePath1.GetPtr() );
			pidl2.CreateFromPath( filePath2.GetPtr() );

			ancestorPidl.Reset( shell::pidl::GetCommonAncestor( pidl1, pidl2 ) );
			ASSERT_EQUAL( _T("d2"), ancestorPidl.ToShellPath().GetFilename() );

			ancestorPidl.Reset( shell::pidl::GetCommonAncestor( pidl2, pidl1 ) );		// swap 1 and 2
			ASSERT_EQUAL( _T("d2"), ancestorPidl.ToShellPath().GetFilename() );
		}

		// distant file path sharing the root drive
		{
			filePath2.Set( _T("C:\\Windows\\notepad.exe") );
			if ( filePath2.FileExist() )
			{
				pidl2.CreateFromShellPath( filePath2.GetPtr() );

				ancestorPidl.Reset( shell::pidl::GetCommonAncestor( pidl1, pidl2 ) );
				ASSERT_EQUAL( _T("C:\\"), ancestorPidl.ToShellPath() );
			}
		}
	}

	// multiple pidls: file system
	{
		std::vector<PIDLIST_ABSOLUTE> absPidls;
		shell::CreateAbsolutePidls( absPidls, pool.GetFilePaths() );

		ancestorPidl.Reset( shell::pidl::ExtractCommonAncestorPidl( absPidls ) );
		ASSERT_EQUAL( _T("_OUT"), ancestorPidl.ToShellPath().GetFilename() );
		shell::ClearOwningPidls( absPidls );
	}


	// non file-system PIDLs
	{
		shell::CPidlAbsolute cpRegionPidl( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}") );
		shell::CPidlAbsolute cpUserAccountsPidl( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{60632754-C523-4B62-B45C-4172DA012619}") );

		ASSERT_EQUAL( _T("Control Panel\\All Control Panel Items\\Region"), cpRegionPidl.GetEditingName() );
		ASSERT_EQUAL( _T("Control Panel\\All Control Panel Items\\User Accounts"), cpUserAccountsPidl.GetEditingName() );

		ancestorPidl.Reset( shell::pidl::GetCommonAncestor( cpUserAccountsPidl, cpRegionPidl ) );
		ASSERT_EQUAL( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0"), ancestorPidl.ToShellPath() );
		ASSERT_EQUAL( _T("Control Panel\\All Control Panel Items"), ancestorPidl.GetEditingName() );

		// "Control Panel\All Control Panel Items\Region" and "Control Panel":
		shell::CPidlAbsolute cpRootPidl;
		ASSERT( cpRegionPidl.GetParentPidl( cpRootPidl ) );
		ASSERT( cpRootPidl.GetParentPidl( cpRootPidl ) );
		ASSERT_EQUAL( _T("Control Panel"), cpRootPidl.GetEditingName() );

		ancestorPidl.Reset( shell::pidl::GetCommonAncestor( cpRegionPidl, cpRootPidl ) );
		ASSERT_EQUAL( _T("::{26EE0668-A00A-44D7-9371-BEB064C98683}"), ancestorPidl.ToShellPath() );
		ASSERT_EQUAL( _T("Control Panel"), ancestorPidl.GetEditingName() );

		// "...\_OUT\A.txt" and "Control Panel\All Control Panel Items\Region"
		shell::CPidlAbsolute fileSystemPidl( pool.GetFilePathAt( 0 ).GetPtr() );

		ancestorPidl.Reset( shell::pidl::GetCommonAncestor( fileSystemPidl, cpRegionPidl ) );
		ASSERT( ancestorPidl.IsEmpty() );
		ASSERT_EQUAL( _T("Desktop"), ancestorPidl.ToShellPath().GetFilename() );		// "C:\Users\Paul\Desktop"
		ASSERT_EQUAL( _T("Desktop"), ancestorPidl.GetEditingName() );

		// multiple pidls: Control Panel applets, then mixed with file system
		{
			std::vector<PCIDLIST_ABSOLUTE> absPidls;
			absPidls.push_back( cpRegionPidl );
			absPidls.push_back( cpUserAccountsPidl );
			absPidls.push_back( cpRootPidl );

			ancestorPidl.Reset( shell::pidl::ExtractCommonAncestorPidl( absPidls ) );
			ASSERT_EQUAL( _T("Control Panel"), cpRootPidl.GetEditingName() );

			absPidls.push_back( fileSystemPidl );		// add file system to mix with Control Panel applets

			ancestorPidl.Reset( shell::pidl::ExtractCommonAncestorPidl( absPidls ) );
			ASSERT_EQUAL( _T("Desktop"), ancestorPidl.GetEditingName() );		// Desktop is the only common root due to file-system path mixed with non file-system paths

			//shell::ClearOwningPidls( absPidls );		// no owership, it's owned by the source PIDLs
		}
	}
}


void CShellPidlTests::Run( void )
{
	RUN_TEST( TestNullAndEmptyPidl );
	RUN_TEST( TestShellPidl );
	RUN_TEST( TestFolderRelativePidls );
	RUN_TEST( TestPidlType );
	RUN_TEST( TestParentPidl );
	RUN_TEST( TestCommonAncestorPidl );
}


#endif //USE_UT
