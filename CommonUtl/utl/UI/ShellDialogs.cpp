
#include "pch.h"
#include "ShellDialogs.h"
#include "ShellPidl.h"
#include "FileSystem_fwd.h"
#include "TreeControl.h"
#include "utl/Algorithms.h"
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
			static std::unordered_map<std::tstring, int> s_selFilterMap;		// filter text to selected filter index
		};


		CComPtr<IFileOpenDialog> GetFileOpenDialog( CFileDialog* pFileDlg )
		{
			ASSERT_PTR( pFileDlg );
			CComPtr<IFileOpenDialog> pFileOpenDialog;

			if ( s_useVistaStyle )
				pFileOpenDialog.Attach( pFileDlg->GetIFileOpenDialog() );

			return pFileOpenDialog;
		}

		bool GetFirstShellPath( OUT shell::TPath& rShellPath, IShellItemArray* pSelItems )
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

		bool GetFirstShellPath( OUT shell::TPath& rShellPath, CFileDialog* pFileDlg )
		{
			ASSERT_PTR( pFileDlg );

			CComPtr<IShellItemArray> pSelItems;
			pSelItems.Attach( pFileDlg->GetResults() );

			return pSelItems != nullptr && GetFirstShellPath( rShellPath, pSelItems );
		}


		// BrowseForFolder callback

		static TCHAR s_initFolderPath[ MAX_PATH ];
		static enum BrowseFolderStatus { Done, Initialized } s_browseFolderStatus = Done;

		int CALLBACK BrowseFolderCallback( HWND hDlg, UINT msg, LPARAM lParam, LPARAM data );
	}


	bool BrowseForFolder( OUT shell::TDirPath& rFolderPath, CWnd* pParentWnd, std::tstring* pDisplayedName /*= nullptr*/,
						  BrowseFlags flags /*= BF_FileSystem*/, const TCHAR* pTitle /*= nullptr*/, bool useNetwork /*= false*/ )
	{
		bool isOk = false;

		str::Copy( impl::s_initFolderPath, rFolderPath.Get() );

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
				str::Copy( impl::s_initFolderPath, rFolderPath.Get() );
		}

		return isOk;
	}


	bool BrowseForFile( OUT fs::CPath& rFilePath, CWnd* pParentWnd, BrowseMode browseMode /*= FileOpen*/,
						const TCHAR* pFileFilter /*= nullptr*/, DWORD flags /*= 0*/, const TCHAR* pTitle /*= nullptr*/ )
	{
		impl::CScopedFileDialog scopedDlg( pFileFilter );

		scopedDlg.StoreDialog( impl::MakeFileDialog( rFilePath, pParentWnd, browseMode, scopedDlg.m_fileFilter, flags, pTitle ) );
		return impl::RunFileDialog( rFilePath, scopedDlg.m_pFileDlg.get() );
	}

	bool PickFolder( OUT fs::TDirPath& rFolderPath, CWnd* pParentWnd,
					 FILEOPENDIALOGOPTIONS options /*= 0*/, const TCHAR* pTitle /*= nullptr*/ )
	{
		mt::CScopedInitializeCom scopedCom;		// Vista-style file dialogs require COM initialization

		DWORD ofnFlags = OFN_PATHMUSTEXIST;
		std::tstring wildPattern;

		if ( path::ContainsWildcards( rFolderPath.GetFilenamePtr() ) )
		{
			SetFlag( ofnFlags, OFN_NOVALIDATE );		// allow wildcards in return string
			wildPattern = rFolderPath.GetFilenamePtr();

			if ( nullptr == pTitle )
				pTitle = _T("Select Folder Pattern");
		}

		CFolderPickerDialog folderDlg( rFolderPath.GetPtr(), ofnFlags, pParentWnd, 0, true );

		if ( pTitle != nullptr )
			folderDlg.m_ofn.lpstrTitle = pTitle;

		if ( options != 0 )
			if ( CComPtr<IFileOpenDialog> pFileOpenDialog = impl::GetFileOpenDialog( &folderDlg ) )
			{
				FILEOPENDIALOGOPTIONS origOptions;
				if ( HR_OK( pFileOpenDialog->GetOptions( &origOptions ) ) )
					if ( !HR_OK( pFileOpenDialog->SetOptions( origOptions | options ) ) )
						TRACE( "# Warning: couldn't modify FILEOPENDIALOGOPTIONS options in PickFolder() dialog!\n" );
			}

		if ( folderDlg.DoModal() != IDOK )
			return false;

		if ( !impl::GetFirstShellPath( rFolderPath, &folderDlg ) )
			return false;

		if ( !path::ContainsWildcards( rFolderPath.GetFilenamePtr() ) )		// no returned pattern? - actually that's not possible
			if ( !wildPattern.empty() )
				rFolderPath /= wildPattern;		// restore the original wildcard pattern

		return true;
	}

	bool BrowseAutoPath( OUT fs::CPath& rFilePath, CWnd* pParent, const TCHAR* pFileFilter /*= nullptr*/ )
	{
		fs::CPath path;
		std::tstring wildSpec;

		if ( fs::ValidFile == fs::SplitPatternPath( &path, &wildSpec, rFilePath ) )
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
					if ( !str::IsEmpty( s_initFolderPath ) )
						SendMessage( hDlg, BFFM_SETSELECTION, TRUE, (LPARAM)s_initFolderPath );
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
