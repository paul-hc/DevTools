
#include "pch.h"
#include "ShellDialogs.h"
#include "ShellPidl.h"
#include "FileSystem_fwd.h"
#include "TreeControl.h"
#include "utl/Algorithms.h"
#include "utl/FlagTags.h"
#include "utl/MultiThreading.h"
#include <unordered_map>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	bool s_useVistaStyle = true;			// set to false to disable Vista style file dialog, which for certain apps is crashing; Vista style requires COM initialization

	namespace impl
	{
		// Allows creation and modal execution in 2 separate steps, with eventual customization in between.
		// Persists last selected filter index per each used array of filters.
		//
		struct CScopedFileDialog
		{
			CScopedFileDialog( const TCHAR* pFileFilter );
			~CScopedFileDialog();

			void StoreDialog( CFileDialog* pFileDlg );
		public:
			std::tstring m_fileFilter;
			std::auto_ptr<CFileDialog> m_pFileDlg;							// Vista style dialog requires COM initialization (done in CFileDialog)

			static const TCHAR s_allFilesFilter[];
			static std::unordered_map<std::tstring, int> s_selFilterMap;	// filter text to selected filter index
		};


		CComPtr<IFileOpenDialog> GetFileOpenDialog( CFileDialog* pFileDlg )
		{
			ASSERT_PTR( pFileDlg );
			CComPtr<IFileOpenDialog> pFileOpenDialog;

			if ( s_useVistaStyle )
				pFileOpenDialog.Attach( pFileDlg->GetIFileOpenDialog() );

			return pFileOpenDialog;
		}

		bool GetFirstItemShellPath( OUT shell::TPath& rShellPath, IShellItemArray* pSelItems )
		{
			ASSERT_PTR( pSelItems );

			DWORD itemCount = 0;
			if ( HR_OK( pSelItems->GetCount( &itemCount ) ) && itemCount != 0 )
			{
				CComPtr<IShellItem> pFirstItem;
				if ( HR_OK( pSelItems->GetItemAt( 0, &pFirstItem ) ) )
				{
					rShellPath = shell::GetItemShellPath( pFirstItem );
					return true;
				}
			}
			return false;
		}

		bool GetFirstItemShellPath( OUT shell::TPath& rShellPath, CFileDialog* pFileDlg )
		{
			CComPtr<IShellItemArray> pSelItems;
			pSelItems.Attach( pFileDlg->GetResults() );

			return pSelItems != nullptr && GetFirstItemShellPath( rShellPath, pSelItems );
		}


		// BrowseForFolder callback

		enum BrowseFolderStatus { Done, Initialized };

		static BrowseFolderStatus s_browseFolderStatus = Done;
		static fs::TDirPath s_initDirPath;

		int CALLBACK BrowseFolderCallback( HWND hDlg, UINT msg, LPARAM lParam, LPARAM data );
	}


	bool BrowseForFolder( OUT shell::TFolderPath& rFolderPath, CWnd* pParentWnd, std::tstring* pDisplayedName /*= nullptr*/,
						  BrowseFlags flags /*= BF_FileSystem*/, const TCHAR* pTitle /*= nullptr*/, bool useNetwork /*= false*/ )
	{
		bool isOk = false;

		impl::s_initDirPath = rFolderPath;

		TCHAR displayName[ MAX_PATH ] = _T("");
		BROWSEINFO bi;
		utl::ZeroStruct( &bi );

		bi.hwndOwner = pParentWnd->GetSafeHwnd();
		bi.pszDisplayName = displayName;
		bi.lpszTitle = pTitle;
		bi.ulFlags = BIF_VALIDATE | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;

		switch( flags )
		{
			case BF_Computers:				bi.ulFlags |= BIF_BROWSEFORCOMPUTER; break;
			case BF_Printers:				bi.ulFlags |= BIF_BROWSEFORPRINTER; break;
			case BF_FileSystem:				bi.ulFlags |= BIF_RETURNONLYFSDIRS; break;
			case BF_FileSystemIncludeFiles:	bi.ulFlags |= ( BIF_RETURNONLYFSDIRS | BIF_BROWSEINCLUDEFILES ); break;
			case BF_AllIncludeFiles:		bi.ulFlags |= BIF_BROWSEINCLUDEFILES; break;
			default: ASSERT( BF_All == flags );
		}

		if ( useNetwork )
			bi.ulFlags |= BIF_DONTGOBELOWDOMAIN;		// could be slow network binding
		bi.lpfn = impl::BrowseFolderCallback;

		CPidlAbsolute folderPidl( ::SHBrowseForFolder( &bi ) );

		if ( pDisplayedName != nullptr )
			*pDisplayedName = displayName;

		if ( !folderPidl.IsNull() )		// was !folderPidl.IsEmpty()
		{
			rFolderPath = folderPidl.ToShellPath();
			if ( !rFolderPath.IsEmpty() )
				isOk = true;
			else if ( flags == BF_Computers || flags == BF_All || flags == BF_AllIncludeFiles )
			{
				rFolderPath.Set( displayName );
				isOk = true;
			}

			if ( isOk )
				impl::s_initDirPath = rFolderPath;
		}

		return isOk;
	}


	bool BrowseForFile( OUT fs::CPath& rFilePath, CWnd* pParentWnd, BrowseMode browseMode /*= FileOpen*/,
						const TCHAR* pFileFilter /*= nullptr*/, DWORD flags /*= 0*/, const TCHAR* pTitle /*= nullptr*/ )
	{
		CScopedInitializeCom scopedCom;			// Vista-style file dialogs require COM initialization in main thread

		impl::CScopedFileDialog scopedDlg( pFileFilter );

		scopedDlg.StoreDialog( impl::MakeFileDialog( rFilePath, pParentWnd, browseMode, scopedDlg.m_fileFilter, flags, pTitle ) );
		return impl::RunFileDialog( rFilePath, scopedDlg.m_pFileDlg.get() );
	}


	bool PickFolder( OUT shell::TFolderPath& rFolderShellPath, CWnd* pParentWnd, FILEOPENDIALOGOPTIONS options /*= 0*/, const TCHAR* pTitle /*= nullptr*/ )
	{
		CScopedInitializeCom scopedCom;			// Vista-style file dialogs require COM initialization in main thread

		DWORD ofnFlags = OFN_PATHMUSTEXIST;
		shell::TFolderPath folderBrowsePath;	// browseable folder path, excluding wildcards
		std::tstring wildPattern;
		const TCHAR* pDirPath;

		// cut the wildcard pattern, since it breaks the dialog's behaviour (simple OK doesn't work anymore)
		if ( fs::GuidPath == fs::CSearchPatternParts::SplitPattern( &folderBrowsePath, &wildPattern, rFolderShellPath ) )		// split into folder path and wildcard pattern
		{
			pDirPath = nullptr;					// will select by folder item via PIDL
			options |= FOS_ALLNONSTORAGEITEMS;	// allow browsing of virtual folders
		}
		else
			pDirPath = folderBrowsePath.GetPtr();

		CFolderPickerDialog folderDlg( pDirPath, ofnFlags, pParentWnd, 0, true );

		if ( pTitle != nullptr )
			folderDlg.m_ofn.lpstrTitle = pTitle;

		if ( CComPtr<IFileOpenDialog> pFileOpenDialog = impl::GetFileOpenDialog( &folderDlg ) )
		{
			FILEOPENDIALOGOPTIONS newOptions;
			if ( HR_OK( pFileOpenDialog->GetOptions( &newOptions ) ) )
			{
				newOptions |= options;
					TRACE( _T("- PickFolder OFN flags={%s}\n"), GetTags_OFN_Flags().FormatKey( folderDlg.GetOFN().Flags ).c_str() );
					TRACE( _T("- PickFolder FILEOPENDIALOGOPTIONS={%s}\n"), GetTags_FILEOPENDIALOGOPTIONS().FormatKey( newOptions ).c_str() );

				if ( options != 0 )
					if ( !HR_OK( pFileOpenDialog->SetOptions( newOptions ) ) )
						TRACE( "# Warning: error modifying FILEOPENDIALOGOPTIONS options in PickFolder() dialog!\n" );
			}

			if ( folderBrowsePath.IsGuidPath() )
				if ( CComPtr<IShellItem> pFolderItem = shell::MakeShellItem( folderBrowsePath.GetPtr() ) )
					if ( !HR_OK( pFileOpenDialog->SetFolder( pFolderItem ) ) )
						TRACE( "# Warning: error selecting initial virtual folder in PickFolder() dialog!\n" );
		}

		if ( IDOK == folderDlg.DoModal() )
			if ( impl::GetFirstItemShellPath( folderBrowsePath, &folderDlg ) )
			{
				if ( !wildPattern.empty() )
					folderBrowsePath /= wildPattern;		// restore the original wildcard pattern

				utl::ModifyValue( rFolderShellPath, folderBrowsePath );
				return true;
			}

		return false;
	}

	bool BrowseAutoPath( OUT fs::CPath& rFilePath, CWnd* pParent, const TCHAR* pFileFilter /*= nullptr*/ )
	{
		fs::CSearchPatternParts patternParts;

		if ( fs::ValidFile == patternParts.Split( rFilePath ) )
			return BrowseForFile( rFilePath, pParent, FileBrowse, pFileFilter );

		return PickFolder( rFilePath, pParent );
	}


	namespace impl
	{
		// File Dialog

		CFileDialog* MakeFileDialog( const fs::CPath& filePath, CWnd* pParentWnd, BrowseMode browseMode, const std::tstring& fileFilter,
									 DWORD flags /*= 0*/, const TCHAR* pTitle /*= nullptr*/ )
		{
			SetFlag( flags, OFN_NOTESTFILECREATE | OFN_PATHMUSTEXIST | OFN_ENABLESIZING );

			switch ( browseMode )
			{
				case FileSaveAs:
					SetFlag( flags, OFN_OVERWRITEPROMPT );
					break;
				case FileOpen:
					SetFlag( flags, OFN_PATHMUSTEXIST );
					if ( path::ContainsWildcards( filePath.GetFilenamePtr() ) )
					{
						ClearFlag( flags, OFN_FILEMUSTEXIST );
						SetFlag( flags, OFN_NOVALIDATE );			// allow wildcards in return string

						if ( nullptr == pTitle )
							pTitle = _T("Select Folder Search Pattern");
					}
					break;
				case FileBrowse:
					if ( nullptr == pTitle )
						pTitle = _T("Browse File");
					break;
			}

			CFileDialog* pDlg = new CFileDialog( browseMode != FileSaveAs, nullptr, filePath.GetPtr(), flags, fileFilter.c_str(), pParentWnd, 0, s_useVistaStyle );

			if ( pTitle != nullptr )
				pDlg->m_ofn.lpstrTitle = pTitle;

			return pDlg;
		}

		bool RunFileDialog( OUT fs::CPath& rFilePath, CFileDialog* pFileDialog )
		{
			ASSERT_PTR( pFileDialog );
			if ( pFileDialog->DoModal() != IDOK )
				return false;

			rFilePath.Set( pFileDialog->GetPathName().GetString() );
			return true;
		}


		// CScopedFileDialog implementation

		const TCHAR CScopedFileDialog::s_allFilesFilter[] = _T("All Files (*.*)|*.*||");
		std::unordered_map<std::tstring, int> CScopedFileDialog::s_selFilterMap;

		CScopedFileDialog::CScopedFileDialog( const TCHAR* pFileFilter )
			: m_fileFilter( pFileFilter != nullptr ? pFileFilter : s_allFilesFilter )
		{
		}

		void CScopedFileDialog::StoreDialog( CFileDialog* pFileDlg )
		{
			ASSERT_PTR( pFileDlg );
			m_pFileDlg.reset( pFileDlg );

			std::unordered_map<std::tstring, int>::const_iterator itFilterIndex = s_selFilterMap.find( m_fileFilter );
			if ( itFilterIndex != s_selFilterMap.end() )
				m_pFileDlg->m_pOFN->nFilterIndex = itFilterIndex->second;				// use last selected filter
		}

		CScopedFileDialog::~CScopedFileDialog()
		{
			if ( m_pFileDlg.get() != nullptr )
				s_selFilterMap[ m_fileFilter ] = m_pFileDlg->m_pOFN->nFilterIndex;		// save selected file filter
		}


		// BrowseForFolder callback

		bool IsTreeCtrl( HWND hCtrl )
		{
			TCHAR className[ 256 ];
			::GetClassName( hCtrl, className, COUNT_OF( className ) );
			return str::Find<str::IgnoreCase>( className, _T("SysTreeView") ) != std::tstring::npos;
		}

		static BOOL CALLBACK FindChildTreeCtrlProc( HWND hWnd, HWND* phCtrl )
		{
			ASSERT_PTR( phCtrl );
			if ( !IsTreeCtrl( hWnd ) )
				return TRUE;			// continue enumeration
			*phCtrl = hWnd;
			return FALSE;				// found, stop enumeration
		}

		CTreeControl* FindBrowseFolderTree( HWND hDlg )
		{
			HWND hCtrl = nullptr;
			::EnumChildWindows( hDlg, (WNDENUMPROC)&FindChildTreeCtrlProc, reinterpret_cast<LPARAM>( &hCtrl ) );
			return hCtrl != nullptr ? (CTreeControl*)CWnd::FromHandle( hCtrl ) : nullptr;
		}

		int CALLBACK BrowseFolderCallback( HWND hDlg, UINT msg, LPARAM lParam, LPARAM data )
		{
			lParam, data;
			switch( msg )
			{
				case BFFM_INITIALIZED:
					if ( !s_initDirPath.IsEmpty() )
						SendMessage( hDlg, BFFM_SETSELECTION, TRUE, (LPARAM)s_initDirPath.GetPtr() );
					s_browseFolderStatus = Initialized;
					break;
				case BFFM_SELCHANGED:
				{
					// it seems no longer necessary
					// set the status window to the currently selected path
					PCIDLIST_ABSOLUTE folderPidl = (PCIDLIST_ABSOLUTE)lParam;
					shell::TPath folderPath = shell::pidl::GetShellPath( folderPidl );

					if ( !folderPath.IsEmpty() )
						SendMessage( hDlg, BFFM_SETSTATUSTEXT, 0, (LPARAM)folderPath.GetPtr() );

					if ( Initialized == s_browseFolderStatus )
					{
						s_browseFolderStatus = Done;

						// the init cycle is: BFFM_SELCHANGED -> BFFM_SELCHANGED -> BFFM_INITIALIZED -> BFFM_IUNKNOWN -> BFFM_SELCHANGED;
						// only as late as now is possible to ensure the tree item is visible (earlier on BFFM_INITIALIZED it doesn't always work)
						if ( CTreeControl* pTreeCtrl = FindBrowseFolderTree( hDlg ) )
							if ( HTREEITEM hSelItem = pTreeCtrl->GetSelectedItem() )
								pTreeCtrl->EnsureVisible( hSelItem );
					}
					break;
				}
				default:
					break;
			}
			return 0;
		}
	}
}


namespace shell
{
	const CFlagTags& GetTags_OFN_Flags( void )
	{
		static const CFlagTags::FlagDef s_flagDefs[] =
		{
		#ifdef _DEBUG
			{ FLAG_TAG( OFN_READONLY ) },
			{ FLAG_TAG( OFN_OVERWRITEPROMPT ) },
			{ FLAG_TAG( OFN_HIDEREADONLY ) },
			{ FLAG_TAG( OFN_NOCHANGEDIR ) },
			{ FLAG_TAG( OFN_SHOWHELP ) },
			{ FLAG_TAG( OFN_ENABLEHOOK ) },
			{ FLAG_TAG( OFN_ENABLETEMPLATE ) },
			{ FLAG_TAG( OFN_ENABLETEMPLATEHANDLE ) },
			{ FLAG_TAG( OFN_NOVALIDATE ) },
			{ FLAG_TAG( OFN_ALLOWMULTISELECT ) },
			{ FLAG_TAG( OFN_EXTENSIONDIFFERENT ) },
			{ FLAG_TAG( OFN_PATHMUSTEXIST ) },
			{ FLAG_TAG( OFN_FILEMUSTEXIST ) },
			{ FLAG_TAG( OFN_CREATEPROMPT ) },
			{ FLAG_TAG( OFN_SHAREAWARE ) },
			{ FLAG_TAG( OFN_NOREADONLYRETURN ) },
			{ FLAG_TAG( OFN_NOTESTFILECREATE ) },
			{ FLAG_TAG( OFN_NONETWORKBUTTON ) },
			{ FLAG_TAG( OFN_NOLONGNAMES ) },
		#if ( WINVER >= 0x0400 )
			{ FLAG_TAG( OFN_EXPLORER ) },
			{ FLAG_TAG( OFN_NODEREFERENCELINKS ) },
			{ FLAG_TAG( OFN_LONGNAMES ) },
			{ FLAG_TAG( OFN_ENABLEINCLUDENOTIFY ) },
			{ FLAG_TAG( OFN_ENABLESIZING ) },
		#endif // WINVER >= 0x0400
		#if ( _WIN32_WINNT >= 0x0500 )
			{ FLAG_TAG( OFN_DONTADDTORECENT ) },
			{ FLAG_TAG( OFN_FORCESHOWHIDDEN ) },
		#endif // _WIN32_WINNT >= 0x0500

		#else
			NULL_TAG
		#endif //_DEBUG
		};
		static const CFlagTags s_tags( ARRAY_SPAN( s_flagDefs ) );
		return s_tags;
	}

	const CFlagTags& GetTags_FILEOPENDIALOGOPTIONS( void )
	{
		static const CFlagTags::FlagDef s_flagDefs[] =
		{
		#ifdef _DEBUG
			{ FLAG_TAG( FOS_OVERWRITEPROMPT ) },
			{ FLAG_TAG( FOS_STRICTFILETYPES ) },
			{ FLAG_TAG( FOS_NOCHANGEDIR ) },
			{ FLAG_TAG( FOS_PICKFOLDERS ) },
			{ FLAG_TAG( FOS_FORCEFILESYSTEM ) },
			{ FLAG_TAG( FOS_ALLNONSTORAGEITEMS ) },
			{ FLAG_TAG( FOS_NOVALIDATE ) },
			{ FLAG_TAG( FOS_ALLOWMULTISELECT ) },
			{ FLAG_TAG( FOS_PATHMUSTEXIST ) },
			{ FLAG_TAG( FOS_FILEMUSTEXIST ) },
			{ FLAG_TAG( FOS_CREATEPROMPT ) },
			{ FLAG_TAG( FOS_SHAREAWARE ) },
			{ FLAG_TAG( FOS_NOREADONLYRETURN ) },
			{ FLAG_TAG( FOS_NOTESTFILECREATE ) },
			{ FLAG_TAG( FOS_HIDEMRUPLACES ) },
			{ FLAG_TAG( FOS_HIDEPINNEDPLACES ) },
			{ FLAG_TAG( FOS_NODEREFERENCELINKS ) },
			{ FLAG_TAG( FOS_OKBUTTONNEEDSINTERACTION ) },
			{ FLAG_TAG( FOS_DONTADDTORECENT ) },
			{ FLAG_TAG( FOS_FORCESHOWHIDDEN ) },
			{ FLAG_TAG( FOS_DEFAULTNOMINIMODE ) },
			{ FLAG_TAG( FOS_FORCEPREVIEWPANEON ) },
			{ FLAG_AS_TAG( FILEOPENDIALOGOPTIONS, FOS_SUPPORTSTREAMABLEITEMS ) }		// aka DWORD
		#else
			NULL_TAG
		#endif //_DEBUG
		};
		static const CFlagTags s_tags( ARRAY_SPAN( s_flagDefs ) );
		return s_tags;
	}
}
