
#include "stdafx.h"
#include "ShellDialogs.h"
#include "ShellTypes.h"
#include "TreeControl.h"
#include "ContainerUtilities.h"
#include <hash_map>

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
			std::auto_ptr< CFileDialog > m_pFileDlg;							// Vista style dialog requires COM initialization (done in CFileDialog)

			static const TCHAR s_allFilesFilter[];
			static stdext::hash_map< std::tstring, int > s_selFilterMap;		// filter text to selected filter index
		};


		// BrowseForFolder callback

		static TCHAR s_initFolderPath[ MAX_PATH ];
		static enum BrowseFolderStatus { Done, Initialized } s_browseFolderStatus = Done;

		int CALLBACK BrowseFolderCallback( HWND hDlg, UINT msg, LPARAM lParam, LPARAM data );
	}


	bool BrowseForFolder( std::tstring& rFolderPath, CWnd* pParentWnd, std::tstring* pDisplayedName /*= NULL*/,
						  BrowseFlags flags /*= BF_FileSystem*/, const TCHAR* pTitle /*= NULL*/, bool useNetwork /*= false*/ )
	{
		bool isOk = false;

		str::Copy( impl::s_initFolderPath, rFolderPath );

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

		CPidl pidlFolder( ::SHBrowseForFolder( &bi ) );

		if ( pDisplayedName != NULL )
			*pDisplayedName = displayName;

		if ( !pidlFolder.IsEmpty() )
		{
			if ( ::SHGetPathFromIDList( pidlFolder.Get(), impl::s_initFolderPath ) )
			{
				rFolderPath = impl::s_initFolderPath;
				isOk = true;
			}
			else if ( flags == BF_Computers || flags == BF_All || flags == BF_AllIncludeFiles )
			{
				rFolderPath = displayName;
				isOk = true;
			}
		}

		return isOk;
	}


	bool BrowseForFile( std::tstring& rFilePath, CWnd* pParentWnd, BrowseMode browseMode /*= FileOpen*/,
						const TCHAR* pFileFilter /*= NULL*/, DWORD flags /*= 0*/, const TCHAR* pTitle /*= NULL*/ )
	{
		impl::CScopedFileDialog scopedDlg( pFileFilter );

		scopedDlg.StoreDialog( impl::MakeFileDialog( rFilePath, pParentWnd, browseMode, scopedDlg.m_fileFilter, flags, pTitle ) );
		return impl::RunFileDialog( rFilePath, scopedDlg.m_pFileDlg.get() );
	}

	bool PickFolder( std::tstring& rFilePath, CWnd* pParentWnd,
					 FILEOPENDIALOGOPTIONS options /*= 0*/, const TCHAR* pTitle /*= NULL*/ )
	{
		impl::CScopedFileDialog scopedDlg( NULL );

		scopedDlg.StoreDialog( impl::MakeFileDialog( rFilePath, pParentWnd, FileOpen, std::tstring(), 0, pTitle ) );		// no filter for picking folders

		if ( IFileOpenDialog* pFileOpenDialog = scopedDlg.m_pFileDlg->GetIFileOpenDialog() )
		{
			pFileOpenDialog->SetOptions( FOS_PICKFOLDERS | options );
			pFileOpenDialog->Release();			// ** cannot use CComPtr< IFileOpenDialog > here since CFileDialog::~CFileDialog() fires an assertion any way, shape and form
		}

		return impl::RunFileDialog( rFilePath, scopedDlg.m_pFileDlg.get() );
	}


	namespace impl
	{
		// File Dialog

		CFileDialog* MakeFileDialog( const std::tstring& filePath, CWnd* pParentWnd, BrowseMode browseMode, const std::tstring& fileFilter,
									 DWORD flags /*= 0*/, const TCHAR* pTitle /*= NULL*/ )
		{
			SetFlag( flags, OFN_NOTESTFILECREATE | OFN_PATHMUSTEXIST | OFN_ENABLESIZING );

			if ( FileOpen == browseMode )
				SetFlag( flags, OFN_FILEMUSTEXIST );
			else
				SetFlag( flags, OFN_OVERWRITEPROMPT );

			CFileDialog* pDlg = new CFileDialog( browseMode, NULL, filePath.c_str(), flags, fileFilter.c_str(), pParentWnd, 0, s_useVistaStyle );

			if ( pTitle != NULL )
				pDlg->m_ofn.lpstrTitle = pTitle;

			return pDlg;
		}

		bool RunFileDialog( std::tstring& rFilePath, CFileDialog* pFileDialog )
		{
			ASSERT_PTR( pFileDialog );
			if ( !pFileDialog->DoModal() )
				return false;

			rFilePath = pFileDialog->GetPathName().GetString();
			return true;
		}


		// CScopedFileDialog implementation

		const TCHAR CScopedFileDialog::s_allFilesFilter[] = _T("All Files (*.*)|*.*||");
		stdext::hash_map< std::tstring, int > CScopedFileDialog::s_selFilterMap;

		CScopedFileDialog::CScopedFileDialog( const TCHAR* pFileFilter )
			: m_fileFilter( pFileFilter != NULL ? pFileFilter : s_allFilesFilter )
		{
		}

		void CScopedFileDialog::StoreDialog( CFileDialog* pFileDlg )
		{
			ASSERT_PTR( pFileDlg );
			m_pFileDlg.reset( pFileDlg );

			stdext::hash_map< std::tstring, int >::const_iterator itFilterIndex = s_selFilterMap.find( m_fileFilter );
			if ( itFilterIndex != s_selFilterMap.end() )
				m_pFileDlg->m_pOFN->nFilterIndex = itFilterIndex->second;				// use last selected filter
		}

		CScopedFileDialog::~CScopedFileDialog()
		{
			if ( m_pFileDlg.get() != NULL )
				s_selFilterMap[ m_fileFilter ] = m_pFileDlg->m_pOFN->nFilterIndex;		// save selected file filter
		}


		// BrowseForFolder callback

		bool IsTreeCtrl( HWND hCtrl )
		{
			TCHAR className[ 256 ];
			::GetClassName( hCtrl, className, COUNT_OF( className ) );
			return str::Find< str::IgnoreCase >( className, _T("SysTreeView") ) != std::tstring::npos;
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
			HWND hCtrl = NULL;
			::EnumChildWindows( hDlg, (WNDENUMPROC)&FindChildTreeCtrlProc, reinterpret_cast< LPARAM >( &hCtrl ) );
			return hCtrl != NULL ? (CTreeControl*)CWnd::FromHandle( hCtrl ) : NULL;
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
					TCHAR dirPath[ MAX_PATH ];
					if ( ::SHGetPathFromIDList( (LPITEMIDLIST)lParam, dirPath ) )
						SendMessage( hDlg, BFFM_SETSTATUSTEXT, 0, (LPARAM)dirPath );

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