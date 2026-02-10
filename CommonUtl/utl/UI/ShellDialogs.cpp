
#include "pch.h"
#include "ShellDialogs.h"
#include "ShellPidl.h"
#include "FileSystem_fwd.h"
#include "TreeControl.h"
#include "utl/Algorithms.h"
#include "utl/FlagTags.h"
#include <unordered_map>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	bool s_useVistaStyle = true;			// set to false to disable Vista style file dialog, which for certain apps is crashing; Vista style requires COM initialization


	namespace impl
	{
		class FriendlyFileDialog : public CFileDialog
		{
		public:
			static void SetPickNonFileSysFoldersMode( CFileDialog* pFileDlg, bool pickNonFileSysFoldersMode = true )
			{
				reinterpret_cast<FriendlyFileDialog*>( pFileDlg )->m_bPickNonFileSysFoldersMode = pickNonFileSysFoldersMode;
			}
		};

		// Allows creation and modal execution in 2 separate steps, with eventual customization in between.
		// Persists last selected filter index per each used array of filters.
		//
		struct CScopedFileDialog
		{
			CScopedFileDialog( const TCHAR* pFileFilter );			// File dialog
			CScopedFileDialog( FILEOPENDIALOGOPTIONS options );		// Select Folder dialog
			~CScopedFileDialog();

			void ParseInputPath( const shell::TPath& shellPath );
			void StoreDialog( CFileDialog* pFileDlg, const TCHAR* pTitle );
			void MakeFileDialog( CWnd* pParentWnd, BrowseMode browseMode, DWORD flags = 0, const TCHAR* pTitle = nullptr );

			bool Run( void );

			// selected file(s)
			bool FetchSelPath( OUT shell::TPath& rShellPath );
			bool QueryMultiSelPaths( OUT std::vector<shell::TPath>& rShellPaths );
		public:
			FILEOPENDIALOGOPTIONS m_options;
			shell::TFolderPath m_browsePath;		// browseable file/folder path, excluding wildcards
			std::tstring m_wildSpec;
			const TCHAR* m_pInputPath;				// passed to CFileDialog/CFolderPickerDialog contructor - NULL is selection is by PIDL

			std::tstring m_fileFilter;				// used just for CFileDialog, not for CFolderPickerDialog
		private:
			std::auto_ptr<CFileDialog> m_pFileDlg;							// Vista style dialog requires COM initialization (done in CFileDialog)
			CComPtr<IFileOpenDialog> m_pFileOpenDialog;

			static const TCHAR s_allFilesFilter[];
			static std::unordered_map<std::tstring, int> s_selFilterMap;	// filter text to selected filter index
		};


		// CScopedFileDialog implementation

		const TCHAR CScopedFileDialog::s_allFilesFilter[] = _T("All Files (*.*)|*.*||");
		std::unordered_map<std::tstring, int> CScopedFileDialog::s_selFilterMap;

		CScopedFileDialog::CScopedFileDialog( const TCHAR* pFileFilter )
			: m_options( 0 )
			, m_pInputPath( nullptr )
			, m_fileFilter( pFileFilter != nullptr ? pFileFilter : s_allFilesFilter )
		{
		}

		CScopedFileDialog::CScopedFileDialog( FILEOPENDIALOGOPTIONS options )
			: m_options( options )
			, m_pInputPath( nullptr )
		{
		}

		CScopedFileDialog::~CScopedFileDialog()
		{
			if ( m_pFileDlg.get() != nullptr )
				s_selFilterMap[ m_fileFilter ] = m_pFileDlg->m_pOFN->nFilterIndex;		// save selected file filter
		}

		void CScopedFileDialog::ParseInputPath( const shell::TPath& shellPath )
		{
			// cut the wildcard pattern, since it breaks the dialog's behaviour (simple OK doesn't work anymore)
			shell::CPatternParts::SplitPattern( &m_browsePath, &m_wildSpec, shellPath, fs::BrowseMode );	// split into folder path and wildcard pattern
			if ( m_browsePath.IsGuidPath() )
			{
				m_pInputPath = nullptr;					// will select by folder item via PIDL
				m_options |= FOS_ALLNONSTORAGEITEMS;	// allow browsing of GUID folders
			}
			else
				m_pInputPath = m_browsePath.GetPtr();
		}

		void CScopedFileDialog::StoreDialog( CFileDialog* pFileDlg, const TCHAR* pTitle )
		{
			ASSERT_PTR( pFileDlg );
			ASSERT_NULL( m_pFileDlg.get() );
			ASSERT_NULL( m_pFileOpenDialog );

			m_pFileDlg.reset( pFileDlg );

			if ( s_useVistaStyle )
				m_pFileOpenDialog.Attach( m_pFileDlg->GetIFileOpenDialog() );

			if ( pTitle != nullptr )
				pFileDlg->m_ofn.lpstrTitle = pTitle;

			if ( m_browsePath.IsGuidPath() )
				if ( !pFileDlg->IsPickNonFileSysFoldersMode() )								// not using CFolderPickerDialog?
					FriendlyFileDialog::SetPickNonFileSysFoldersMode( pFileDlg, true );		// forces folder PIDL mode, and prevent assertion in CFileDialog::ApplyOFNToShellDialog()

			OPENFILENAME& rOFN = m_pFileDlg->GetOFN();

			if ( !m_wildSpec.empty() )
			{
				SetFlag( rOFN.Flags, OFN_NOVALIDATE );			// allow wildcards in return string
				ClearFlag( rOFN.Flags, OFN_FILEMUSTEXIST );
			}

			if ( !is_a<CFolderPickerDialog>( pFileDlg ) )
				if ( const int* pSelFilterIndex = utl::FindValuePtr( s_selFilterMap, m_fileFilter ) )
					rOFN.nFilterIndex = *pSelFilterIndex;		// use last selected filter

			if ( IFileDialog* pFileDialog = m_pFileOpenDialog )
			{
				FILEOPENDIALOGOPTIONS origOptions;

				// override dialog options
				if ( HR_OK( pFileDialog->GetOptions( &origOptions ) ) )
					m_options |= origOptions;

				// handle flag transfers not handled in CFileDialog
				SetFlag( m_options, FOS_NOCHANGEDIR, HasFlag( rOFN.Flags, OFN_NOCHANGEDIR ) );
				SetFlag( m_options, FOS_NOVALIDATE, HasFlag( rOFN.Flags, OFN_NOVALIDATE ) );
				SetFlag( m_options, FOS_PATHMUSTEXIST, HasFlag( rOFN.Flags, OFN_PATHMUSTEXIST ) );
				SetFlag( m_options, FOS_FILEMUSTEXIST, HasFlag( rOFN.Flags, OFN_FILEMUSTEXIST ) );
					TRACE( _T("- CScopedFileDialog: OFN flags={%s}\n"), GetTags_OFN_Flags().FormatKey( rOFN.Flags ).c_str() );
					TRACE( _T("- CScopedFileDialog: FILEOPENDIALOGOPTIONS={%s}\n"), GetTags_FILEOPENDIALOGOPTIONS().FormatKey( m_options ).c_str() );

				if ( !HR_OK( pFileDialog->SetOptions( m_options ) ) )
					TRACE( "# Warning: error modifying FILEOPENDIALOGOPTIONS options in CScopedFileDialog() dialog!\n" );

				// select item by PIDL
				if ( m_browsePath.IsGuidPath() )
					if ( CComPtr<IShellItem> pFolderItem = shell::MakeShellItem( m_browsePath.GetPtr() ) )
						if ( !HR_OK( pFileDialog->SetFolder( pFolderItem ) ) )
							TRACE( "# Warning: error selecting initial folder by PIDL in CScopedFileDialog() dialog!\n" );
			}
		}

		void CScopedFileDialog::MakeFileDialog( CWnd* pParentWnd, BrowseMode browseMode, DWORD flags /*= 0*/, const TCHAR* pTitle /*= nullptr*/ )
		{
			SetFlag( flags, OFN_NOTESTFILECREATE | OFN_PATHMUSTEXIST | OFN_ENABLESIZING );

			switch ( browseMode )
			{
				case FileSaveAs:
					SetFlag( flags, OFN_OVERWRITEPROMPT );
					break;
				case FileOpen:
					SetFlag( flags, OFN_PATHMUSTEXIST );
					if ( !m_wildSpec.empty() && nullptr == pTitle )
						pTitle = _T("Select Folder Search Pattern");
					break;
				case FileBrowse:
					if ( nullptr == pTitle )
						pTitle = _T("Browse File");
					break;
			}

			CFileDialog* pFileDlg = new CFileDialog( browseMode != FileSaveAs, nullptr, m_pInputPath, flags, m_fileFilter.c_str(), pParentWnd, 0, s_useVistaStyle );

			StoreDialog( pFileDlg, pTitle );
		}

		bool CScopedFileDialog::Run( void )
		{
			bool ok = IDOK == m_pFileDlg->DoModal();

			if (0)
			{
				FILEOPENDIALOGOPTIONS options;
				if ( HR_OK( m_pFileOpenDialog->GetOptions( &options ) ) )
				{
					TRACE( _T("- CScopedFileDialog: OFN flags={%s}\n"), GetTags_OFN_Flags().FormatKey( m_pFileDlg->GetOFN().Flags ).c_str() );
					TRACE( _T("- CScopedFileDialog: FILEOPENDIALOGOPTIONS={%s}\n"), GetTags_FILEOPENDIALOGOPTIONS().FormatKey( options ).c_str() );
				}
			}
			return ok;
		}

		bool CScopedFileDialog::FetchSelPath( OUT shell::TPath& rShellPath )
		{
			if ( m_pFileOpenDialog != nullptr )
			{
				CComPtr<IShellItem> pSelItem;

				if ( HR_OK( m_pFileOpenDialog->GetResult( &pSelItem ) ) && pSelItem != nullptr )
				{
					shell::TPath selPath = shell::GetItemShellPath( pSelItem );
					//TRACE( _T("- CScopedFileDialog::FetchSelPath: selPath='%s'\n\tEditing Name='%s'\n"), selPath.GetPtr(), shell::GetEditingName( pSelItem ).c_str() );

					if ( !m_wildSpec.empty() && shell::ShellFolderExist( selPath.GetPtr() ) )
						selPath /= m_wildSpec;		// folder path: restore the original wildcard pattern

					utl::ModifyValue( rShellPath, selPath );
					return true;
				}
				return false;
			}
			else
				rShellPath.Set( m_pFileDlg->GetPathName().GetString() );

			return true;
		}

		bool CScopedFileDialog::QueryMultiSelPaths( OUT std::vector<shell::TPath>& rShellPaths )
		{
			REQUIRE( HasFlag( m_pFileDlg->GetOFN().Flags, OFN_ALLOWMULTISELECT ) );		// not strictly, it also works with single file selection
			CComPtr<IShellItemArray> pSelItems;

			if ( HR_OK( m_pFileOpenDialog->GetResults( &pSelItems ) ) && pSelItems != nullptr )
			{
				shell::QueryShellItemArrayEnumPaths( rShellPaths, pSelItems );
				return true;
			}

			return false;
		}
	}
}


namespace shell
{
	bool BrowseForFile( IN OUT shell::TPath& rShellPath, CWnd* pParentWnd, BrowseMode browseMode /*= FileOpen*/,
						const TCHAR* pFileFilter /*= nullptr*/, DWORD flags /*= 0*/, const TCHAR* pTitle /*= nullptr*/ )
	{
		impl::CScopedFileDialog scopedDlg( pFileFilter );

		scopedDlg.ParseInputPath( rShellPath );
		scopedDlg.MakeFileDialog( pParentWnd, browseMode, flags, pTitle );

		if ( scopedDlg.Run() )
			return scopedDlg.FetchSelPath( rShellPath );

		return false;
	}

	bool BrowseForFiles( IN OUT std::vector<shell::TPath>& rShellPaths, CWnd* pParentWnd,
						 const TCHAR* pFileFilter /*= nullptr*/, DWORD flags /*= 0*/, const TCHAR* pTitle /*= nullptr*/ )
	{
		impl::CScopedFileDialog scopedDlg( pFileFilter );

		if ( !rShellPaths.empty() )
			scopedDlg.ParseInputPath( rShellPaths.front() );

		SetFlag( flags, OFN_ALLOWMULTISELECT | OFN_NOTESTFILECREATE | OFN_PATHMUSTEXIST | OFN_ENABLESIZING );

		CFileDialog* pFileDlg = new CFileDialog( TRUE, nullptr, scopedDlg.m_pInputPath, flags, scopedDlg.m_fileFilter.c_str(), pParentWnd, 0, s_useVistaStyle );

		scopedDlg.StoreDialog( pFileDlg, pTitle );

		if ( scopedDlg.Run() )
			return scopedDlg.QueryMultiSelPaths( rShellPaths );

		return false;
	}


	bool PickFolder( IN OUT shell::TFolderPath& rFolderShellPath, CWnd* pParentWnd, FILEOPENDIALOGOPTIONS options /*= 0*/, const TCHAR* pTitle /*= nullptr*/ )
	{
		impl::CScopedFileDialog scopedDlg( options );

		scopedDlg.ParseInputPath( rFolderShellPath );

		CFileDialog* pFileDlg = new CFolderPickerDialog( scopedDlg.m_pInputPath, OFN_PATHMUSTEXIST, pParentWnd, 0, HasFlag( scopedDlg.m_options, FOS_ALLNONSTORAGEITEMS ) );

		scopedDlg.StoreDialog( pFileDlg, pTitle );

		if ( scopedDlg.Run() )
			return scopedDlg.FetchSelPath( rFolderShellPath );

		return false;
	}

	bool BrowseAutoPath( IN OUT shell::TPath& rShellPath, CWnd* pParent, const TCHAR* pFileFilter /*= nullptr*/ )
	{
		shell::CPatternParts parts( fs::BrowseMode );

		if ( fs::ValidFile == parts.Split( rShellPath ) )
			return BrowseForFile( rShellPath, pParent, FileBrowse, pFileFilter );

		return PickFolder( rShellPath, pParent );
	}
}


namespace shell
{
	namespace impl
	{
		// BrowseForFolder callback

		enum BrowseFolderStatus { Done, Initialized };

		static BrowseFolderStatus s_browseFolderStatus = Done;
		static fs::TDirPath s_initDirPath;

		int CALLBACK BrowseFolderCallback( HWND hDlg, UINT msg, LPARAM lParam, LPARAM data );
	}


	// legacy/classic browse folder tree dialog

	bool BrowseForFolder( IN OUT shell::TFolderPath& rFolderPath, CWnd* pParentWnd, std::tstring* pDisplayedName /*= nullptr*/,
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


	namespace impl
	{
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
