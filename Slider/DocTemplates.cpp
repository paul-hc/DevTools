
#include "stdafx.h"
#include "DocTemplates.h"
#include "AlbumDoc.h"
#include "AlbumChildFrame.h"
#include "AlbumImageView.h"
#include "ImageDoc.h"
#include "ShellRegHelpers.h"
#include "Application.h"
#include "resource.h"
#include "utl/Path.h"
#include "utl/Registry.h"
#include "utl/StringUtilities.h"
#include "utl/ScopedValue.h"
#include "utl/UI/ImagingWic.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/ShellFileDialog.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/WicImage.h"
#include <afxpriv.h>		// for AfxRegQueryValue

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace app
{
	bool PromptFileDialogImpl( CString& rFilePath, const fs::CFilterJoiner& filterJoiner, UINT titleId, DWORD flags, BOOL openDlg )
	{
		std::tstring filePath = rFilePath.GetString();
		std::tstring title = str::Load( titleId );

		if ( !filterJoiner.BrowseFile( filePath, static_cast< shell::BrowseMode >( openDlg ), flags, NULL, NULL, !title.empty() ? title.c_str() : NULL ) )
			return false;

		rFilePath = filePath.c_str();
		return true;
	}


	// CSharedDocTemplate implementation

	bool CSharedDocTemplate::s_useSingleFilterExt = false;

	CSharedDocTemplate::CSharedDocTemplate( UINT idResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass )
		: CMultiDocTemplate( idResource, pDocClass, pFrameClass, pViewClass )
		, m_pFilterStore( NULL )
		, m_acceptDirPath( false )
		, m_accel( m_hAccelTable )
	{
		m_menu.Attach( m_hMenuShared );
		VERIFY( ui::SetMenuImages( m_menu ) );
	}

	CSharedDocTemplate::~CSharedDocTemplate()
	{	// prevent deleting shared resources by CMultiDocTemplate dtor (owned by this class)
		m_hMenuShared = NULL;
		m_hAccelTable = NULL;
	}

	void CSharedDocTemplate::SetFilterStore( const fs::CFilterStore* pFilterStore )
	{
		m_pFilterStore = pFilterStore;
		m_fileFilters = m_pFilterStore->MakeFilters();
		m_knownExts = m_pFilterStore->GetKnownExtensions().MakeAllExts();
		m_pFilterStore->GetKnownExtensions().QueryAllExts( m_allExts );
	}

	BOOL CSharedDocTemplate::GetDocString( CString& rString, enum DocStringIndex index ) const
	{
		if ( !CMultiDocTemplate::GetDocString( rString, index ) )
			return false;

		if ( filterExt == index )
			if ( s_useSingleFilterExt )
				rString = m_allExts.front().c_str();		// use just the first (default) extension
			else
				rString = m_knownExts.c_str();

		return true;
	}

	CDocTemplate::Confidence CSharedDocTemplate::MatchDocType( LPCTSTR pPathName, CDocument*& rpDocMatch )
	{
		Confidence confidence = CMultiDocTemplate::MatchDocType( pPathName, rpDocMatch );

		if ( yesAttemptForeign == confidence )
			if ( m_pFilterStore->GetKnownExtensions().ContainsExt( pPathName ) )
			    confidence = yesAttemptNative;					// known extension match
			else if ( fs::IsValidDirectory( pPathName ) )
			    if ( m_acceptDirPath )
					confidence = yesAttemptNative;				// accept directory path
				else
					confidence = noAttempt;						// reject directory path

		return confidence;
	}

	void CSharedDocTemplate::AlterSaveAsPath( CString& rFilePath ) const
	{
		rFilePath;
	}

	bool CSharedDocTemplate::PromptFileDialog( CString& rFilePath, UINT titleId, DWORD flags, BOOL openDlg ) const
	{
		if ( shell::FileSaveAs == openDlg )
			AlterSaveAsPath( rFilePath );

		fs::CFilterJoiner filterJoiner( *m_pFilterStore );
		return app::PromptFileDialogImpl( rFilePath, filterJoiner, titleId, flags, openDlg );
	}

	void CSharedDocTemplate::RegisterAdditionalDocExts( void )
	{
		CString docTypeId;
		GetDocString( docTypeId, CDocTemplate::regFileTypeId );
		if ( docTypeId.IsEmpty() )
			return;

		// skip first assuming is already registered
		if ( m_allExts.size() > 1 )
			for ( size_t i = 1; i != m_allExts.size(); ++i )
				VERIFY( RegisterAdditionalDocExt( docTypeId, m_allExts[ i ].c_str() ) );		// register all additional extensions
	}

	bool CSharedDocTemplate::RegisterAdditionalDocExt( const TCHAR* pDocTypeId, const TCHAR* pDocExt )
	{
		ASSERT( pDocExt != NULL && _T('.') == pDocExt[ 0 ] );

		TCHAR textBuffer[ MAX_PATH * 2 ];
		long size = COUNT_OF( textBuffer );
		long result = AfxRegQueryValue( HKEY_CLASSES_ROOT, pDocExt, textBuffer, &size );
		std::tstring text( textBuffer );

		if ( result != ERROR_SUCCESS || text.empty() || text == pDocTypeId )
		{
			// no association for that suffix
			if ( !SetRegKey( pDocExt, pDocTypeId, NULL ) )
				return false;

			SetRegKey( str::Format( _T("%s\\ShellNew"), pDocExt ).c_str(), _T(""), _T("NullFile") );
		}
		return true;
	}

	bool CSharedDocTemplate::SetRegKey( const TCHAR* pKey, const TCHAR* pValue, const TCHAR* pValueName )
	{
		if ( NULL == pValueName )
			return ERROR_SUCCESS == AfxRegSetValue( HKEY_CLASSES_ROOT, pKey, REG_SZ, pValue, (DWORD)( str::GetLength( pValue ) * sizeof( TCHAR ) ) );

		HKEY hKey;
		if ( ERROR_SUCCESS == AfxRegCreateKey( HKEY_CLASSES_ROOT, pKey, &hKey ) )
		{
			long result = ::RegSetValueEx( hKey, pValueName, 0, REG_SZ, (const BYTE*)pValue, (DWORD)( str::GetLength( pValue ) + 1 ) * sizeof( TCHAR ) );
			if ( ERROR_SUCCESS == ::RegCloseKey( hKey ) && ERROR_SUCCESS == result )
				return true;
		}
		return false;
	}


	// CImageDocTemplate implementation

	CImageDocTemplate::CImageDocTemplate( void )
		: CSharedDocTemplate( IDR_IMAGETYPE, RUNTIME_CLASS( CImageDoc ), RUNTIME_CLASS( CChildFrame ), RUNTIME_CLASS( CImageView ) )
	{
		SetFilterStore( &fs::CImageFilterStore::Instance( shell::FileOpen ) );
	}

	CImageDocTemplate* CImageDocTemplate::Instance( void )
	{
		static CImageDocTemplate* pImageDocTemplate = new CImageDocTemplate;		// owned by doc manager
		return pImageDocTemplate;
	}


	// CAlbumDocTemplate implementation

	CAlbumDocTemplate::CAlbumDocTemplate( void )
		: CSharedDocTemplate( IDR_ALBUMTYPE, RUNTIME_CLASS( CAlbumDoc ), RUNTIME_CLASS( CAlbumChildFrame ), RUNTIME_CLASS( CAlbumImageView ) )
	{
		SetFilterStore( &CAlbumFilterStore::Instance() );
		m_acceptDirPath = true;
	}

	CAlbumDocTemplate* CAlbumDocTemplate::Instance( void )
	{
		static CAlbumDocTemplate* pAlbumDocTemplate = new CAlbumDocTemplate;		// owned by doc manager
		return pAlbumDocTemplate;
	}

	CAlbumDocTemplate::OpenPathType CAlbumDocTemplate::GetOpenPathType( const TCHAR* pPath )
	{
		if ( fs::IsValidDirectory( pPath ) )
			return DirPath;
		else if ( fs::FileExist( pPath ) )
			if ( path::MatchExt( pPath, _T(".sld") ) )
				return SlideAlbum;
			else if ( IsImageArchiveDoc( pPath ) )
				return ImageArchiveDoc;

		return InvalidPath;
	}

	void CAlbumDocTemplate::AlterSaveAsPath( CString& rFilePath ) const
	{
		if ( fs::IsValidDirectory( rFilePath ) )				// most likely
		{
			std::tstring fnameExt = std::tstring( path::FindFilename( rFilePath ) ) + _T(".sld");
			fs::CPath albumPath = fs::CPath( rFilePath.GetString() ).GetParentPath() / fnameExt;
			rFilePath = albumPath.GetPtr();
		}
	}

	void CAlbumDocTemplate::RegisterAlbumShellDirectory( bool doRegister )
	{
		using namespace shell_reg;

		static fs::CPath s_sliderKeyPath( _T("Directory\\shell\\SlideView") );
		s_sliderKeyPath / _T("abc");
		s_sliderKeyPath /= _T("xyz");

		// process the 'Directory' file type handler
		if ( doRegister )
		{	// register Slider as view for sliding directory images

			reg::CKey commandKey;
			if ( commandKey.Create( HKEY_CLASSES_ROOT, s_dirHandler[ ShellCommand ] ) )
			{
				std::tstring cmdLine = arg::Enquote( app::GetModulePath() ) + _T(" ") + arg::Enquote( _T("%1") );
				commandKey.WriteStringValue( NULL, cmdLine );						// write the directory command value (such as "C:\My\Tools\mine\Slider.exe" "%1")
			}

			reg::CKey ddeExecKey;
			if ( ddeExecKey.Create( HKEY_CLASSES_ROOT, s_dirHandler[ ShellDdeExec ] ) )
				ddeExecKey.WriteStringValue( NULL, s_fmtValueDdeExec[ 0 ] );		// write the directory ddeexec value (such as "[open("%L")]")
		}
		else
		{	// remove Slider registration entries
			reg::CKey dirShellKey;
			if ( dirShellKey.Open( HKEY_CLASSES_ROOT, _T("Directory\\shell") ) )
				dirShellKey.DeleteSubKey( s_dirVerb_SlideView );
		}
	}


	// CDocManager implementation

	CDocManager::CDocManager( void )
	{
		AddDocTemplate( CImageDocTemplate::Instance() );
		AddDocTemplate( CAlbumDocTemplate::Instance() );
	}

	void CDocManager::OnFileNew( void )
	{
		if ( m_templateList.IsEmpty() )
		{
			TRACE( "Error: no document templates registered with CWinApp.\n" );
			AfxMessageBox( AFX_IDP_FAILED_TO_CREATE_DOC );
		}
		else
		{
			CDocTemplate* pTemplate = (CDocTemplate*)m_templateList.GetHead();
			// avoid prompting for new template dialog since Slider template is a backing template
			pTemplate->OpenDocumentFile( NULL );		// if returns NULL, the user has already been alerted
		}
	}

	BOOL CDocManager::DoPromptFileName( CString& rFilePath, UINT titleId, DWORD flags, BOOL openDlg, CDocTemplate* pTemplate )
	{
		if ( CSharedDocTemplate* pSharedTemplate = dynamic_cast< CSharedDocTemplate* >( pTemplate ) )
			return pSharedTemplate->PromptFileDialog( rFilePath, titleId, flags, openDlg );

		return app::PromptFileDialogImpl( rFilePath, CSliderFilters::Instance(), titleId, flags, openDlg );
	}

	void CDocManager::RegisterShellFileTypes( BOOL compatMode )
	{
		{
			CScopedValue< bool > scopedSingleExt( &CSharedDocTemplate::s_useSingleFilterExt, true );		// prevent creating ".sld;.ias;.cid;.icf" registry key
			::CDocManager::RegisterShellFileTypes( compatMode );
		}

		CAlbumDocTemplate* pAlbumTemplate = CAlbumDocTemplate::Instance();
		pAlbumTemplate->RegisterAdditionalDocExts();
		pAlbumTemplate->RegisterAlbumShellDirectory( true );
	}

	void CDocManager::RegisterImageAdditionalShellExt( bool doRegister )
	{
		using namespace shell_reg;

		static const fs::CPath sliderExePath = app::GetModulePath();

		// process known registered image files extensions
		const std::vector< std::tstring >& imageExts = CImageDocTemplate::Instance()->GetAllExts();
		for ( std::vector< std::tstring >::const_iterator itImageExt = imageExts.begin(); itImageExt != imageExts.end(); ++itImageExt )
		{
			reg::CKey extKey;
			if ( extKey.Open( HKEY_CLASSES_ROOT, itImageExt->c_str() ) )				// extension already registered?
			{
				// query for default key value (shell handler name)
				std::tstring shellHandlerName = extKey.ReadStringValue( NULL );
				if ( !shellHandlerName.empty() )
				{
					reg::CKey shellHandlerKey;
					if ( shellHandlerKey.Open( HKEY_CLASSES_ROOT, shellHandlerName.c_str() ) )
						if ( doRegister )
						{	// valid indirect shell file handler, add command and ddeexec keys
							for ( size_t k = 0; k != COUNT_OF( s_fmtShlHandlerCommand ); ++k )
							{
								reg::CKey commandKey;
								if ( commandKey.Create( HKEY_CLASSES_ROOT, str::Format( s_fmtShlHandlerCommand[ k ], shellHandlerName.c_str() ).c_str() ) )
									commandKey.WriteStringValue( NULL, arg::Enquote( sliderExePath ).c_str() );			// write the shell command (like "C:\My\Tools\mine\Slider.exe" "%1")

								reg::CKey ddeExecKey;
								if ( ddeExecKey.Create( HKEY_CLASSES_ROOT, str::Format( s_fmtShlHandlerDdeExec[ k ], shellHandlerName.c_str() ).c_str() ) )
									ddeExecKey.WriteStringValue( NULL, s_fmtValueDdeExec[ k ] );						// write the shell ddeexec, like "[open("%1")]" or "[queue(\"%1\")]", etc
							}
						}
						else
						{	// remove slider entries for shell
							reg::CKey shellKey;
							if ( shellKey.Open( HKEY_CLASSES_ROOT, str::Format( _T("%s\\shell"), shellHandlerName.c_str() ).c_str() ) )
							{
								shellKey.DeleteSubKey( s_imgVerb_OpenWithSlider, Deep );
								shellKey.DeleteSubKey( s_imgVerb_QueueInSlider, Deep );
							}
						}
				}
			}
		}
	}

} //namespace app


namespace app
{
	// CAlbumFilterStore implementation

	CAlbumFilterStore::CAlbumFilterStore( void )
		: fs::CFilterStore( _T("Albums") )
	{
		AddFilter( str::LoadPair( IDS_SLIDE_ALBUM_FILTER_SPEC ) );
		AddFilter( str::LoadPair( IDS_ARCHIVE_STG_FILTER_SPEC ) );
	}

	CAlbumFilterStore& CAlbumFilterStore::Instance( void )
	{
		static CAlbumFilterStore albumStore;
		return albumStore;
	}

	std::tstring CAlbumFilterStore::MakeArchiveStgFilters( void ) const
	{
		UINT positions = ArchiveStgFilter;
		return MakeFilters( &positions, 1 );
	}


	// CSliderFilters implementation

	CSliderFilters::CSliderFilters( void )
	{
		Add( fs::CImageFilterStore::s_classTag );
		Add( CAlbumFilterStore::Instance().GetClassTag() );
	}

	CSliderFilters& CSliderFilters::Instance( void )
	{
		static CSliderFilters appFilterSpecs;
		return appFilterSpecs;
	}

} //namespace app
