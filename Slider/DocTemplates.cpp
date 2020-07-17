
#include "stdafx.h"
#include "DocTemplates.h"
#include "AlbumDoc.h"
#include "AlbumChildFrame.h"
#include "AlbumImageView.h"
#include "ImageDoc.h"
#include "Application_fwd.h"
#include "resource.h"
#include "utl/Path.h"
#include "utl/FileSystem.h"
#include "utl/Registry.h"
#include "utl/StringUtilities.h"
#include "utl/ScopedValue.h"
#include "utl/UI/ImagingWic.h"
#include "utl/UI/MenuUtilities.h"
#include "utl/UI/ShellFileDialog.h"
#include "utl/UI/ShellRegistryAssoc.h"
#include "utl/UI/Utilities.h"
#include "utl/UI/WicImage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace app
{
	bool PromptFileDialogImpl( CString& rFilePath, const fs::CFilterJoiner& filterJoiner, UINT titleId, DWORD flags, BOOL openDlg )
	{
		fs::CPath filePath( rFilePath.GetString() );
		std::tstring title = str::Load( titleId );

		if ( !filterJoiner.BrowseFile( filePath, static_cast< shell::BrowseMode >( openDlg ), flags, NULL, NULL, !title.empty() ? title.c_str() : NULL ) )
			return false;

		rFilePath = filePath.GetPtr();
		return true;
	}
}


// CAppDocManager implementation

CAppDocManager::CAppDocManager( void )
	: CDocManager()
	, m_pAlbumTemplate( app::CAlbumDocTemplate::Instance() )
{
	AddDocTemplate( m_pAlbumTemplate );
	AddDocTemplate( app::CImageDocTemplate::Instance() );
}

CAppDocManager::~CAppDocManager()
{
}

void CAppDocManager::OnFileNew( void )
{
	ASSERT_PTR( m_pAlbumTemplate );

	m_pAlbumTemplate->OpenDocumentFile( NULL );
}

BOOL CAppDocManager::DoPromptFileName( CString& rFilePath, UINT titleId, DWORD flags, BOOL openDlg, CDocTemplate* pTemplate )
{
	if ( app::CSharedDocTemplate* pSharedTemplate = dynamic_cast< app::CSharedDocTemplate* >( pTemplate ) )
		return pSharedTemplate->PromptFileDialog( rFilePath, titleId, flags, static_cast< shell::BrowseMode >( openDlg ) );

	return app::PromptFileDialogImpl( rFilePath, app::CSliderFilters::Instance(), titleId, flags, openDlg );
}

void CAppDocManager::RegisterShellFileTypes( BOOL compatMode )
{
	{
		CScopedValue< bool > scopedSingleExt( &app::CSharedDocTemplate::s_useSingleFilterExt, true );		// prevent creating ".sld;.ias;.cid;.icf" registry key
		__super::RegisterShellFileTypes( compatMode );
	}

	m_pAlbumTemplate->RegisterAdditionalDocExtensions();
	m_pAlbumTemplate->RegisterAlbumShellDirectory( true );
}

void CAppDocManager::RegisterImageAdditionalShellExt( bool doRegister )
{
	// process known registered image files extensions
	const std::vector< std::tstring >& imageExts = app::CImageDocTemplate::Instance()->GetAllExts();

	for ( std::vector< std::tstring >::const_iterator itImageExt = imageExts.begin(); itImageExt != imageExts.end(); ++itImageExt )
	{
		std::tstring handlerName;
		if ( shell::QueryHandlerName( handlerName, itImageExt->c_str() ) )			// valid indirect shell extension handler?
		{
			reg::TKeyPath openPath = shell::MakeShellHandlerVerbPath( handlerName.c_str(), _T("OpenWithSlider") );				// "<handler>\\shell\\OpenWithSlider"
			reg::TKeyPath enqueuePath = shell::MakeShellHandlerVerbPath( handlerName.c_str(), _T("EnqueueInSlider") );			// "<handler>\\shell\\EnqueueInSlider"

			if ( doRegister )
			{
				shell::RegisterShellVerb( openPath, app::GetModulePath(), _T("Open with &Slider"), shell::GetDdeOpenCmd() );
				shell::RegisterShellVerb( enqueuePath, app::GetModulePath(), _T("Enque&ue in Slider"), _T("[queue(\"%1\")]") );
			}
			else
			{	// remove slider entries for shell
				shell::UnregisterShellVerb( openPath );
				shell::UnregisterShellVerb( enqueuePath );
			}
		}
	}
}


namespace app
{
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

	bool CSharedDocTemplate::PromptFileDialog( CString& rFilePath, UINT titleId, DWORD flags, shell::BrowseMode browseMode ) const
	{
		if ( shell::FileSaveAs == browseMode )
			AlterSaveAsPath( rFilePath );

		fs::CFilterJoiner filterJoiner( *m_pFilterStore );
		return app::PromptFileDialogImpl( rFilePath, filterJoiner, titleId, flags, browseMode );
	}

	void CSharedDocTemplate::RegisterAdditionalDocExtensions( void )
	{
		std::tstring docType;
		{
			CString docTypeId;
			GetDocString( docTypeId, CDocTemplate::regFileTypeId );
			if ( docTypeId.IsEmpty() )
				return;
			docType = docTypeId;
		}

		// skip first assuming is already registered
		if ( m_allExts.size() > 1 )
			for ( size_t i = 1; i != m_allExts.size(); ++i )
				shell::RegisterAdditionalDocumentExt( m_allExts[ i ], docType );		// register all additional extensions
	}


	// CImageDocTemplate implementation

	CImageDocTemplate::CImageDocTemplate( void )
		: CSharedDocTemplate( IDR_IMAGETYPE, RUNTIME_CLASS( CImageDoc ), RUNTIME_CLASS( CChildFrame ), RUNTIME_CLASS( CImageView ) )
	{
		SetFilterStore( &fs::CImageFilterStore::Instance( shell::FileOpen ) );
	}

	CImageDocTemplate* CImageDocTemplate::Instance( void )
	{
		static CImageDocTemplate* s_pImageDocTemplate = new CImageDocTemplate;		// owned by the doc manager
		return s_pImageDocTemplate;
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
		static CAlbumDocTemplate* s_pAlbumDocTemplate = new CAlbumDocTemplate;		// owned by the doc manager
		return s_pAlbumDocTemplate;
	}

	CAlbumDocTemplate::OpenPathType CAlbumDocTemplate::GetOpenPathType( const TCHAR* pPath )
	{
		if ( fs::IsValidDirectory( pPath ) )
			return DirPath;
		else if ( fs::FileExist( pPath ) )
			if ( path::MatchExt( pPath, _T(".sld") ) )
				return SlideAlbum;
			else if ( app::IsCatalogFile( pPath ) )
				return CatalogStorageDoc;

		return InvalidPath;
	}

	bool CAlbumDocTemplate::IsSlideAlbumFile( const TCHAR* pFilePath )
	{
		return path::MatchExt( pFilePath, _T(".sld") );
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
		// unregister any old Slider version 'Directory' entry - that's necessary since otherwise Explorer gets confused and invokes the old handler
		reg::TKeyPath oldVerbPath = shell::MakeShellHandlerVerbPath( _T("Directory"), _T("Slide &View") );		// "Directory\\shell\\Slide &View"
		if ( shell::UnregisterShellVerb( oldVerbPath ) )
			TRACE( _T(" Note: cleaned-up the old Slider shell registration: %s\n"), oldVerbPath.GetPtr() );

		// register Slider as view for sliding images in 'Directory' file type handler:
		reg::TKeyPath verbPath = shell::MakeShellHandlerVerbPath( _T("Directory"), _T("SlideView") );		// "Directory\\shell\\SlideView"

		if ( doRegister )
			shell::RegisterShellVerb( verbPath, app::GetModulePath(), _T("Slide &View"), shell::GetDdeOpenCmd() );		// e.g. command="C:\My\Tools\mine\Slider.exe" "%1"
		else
			shell::UnregisterShellVerb( verbPath );
	}

} //namespace app


namespace app
{
	// CAlbumFilterStore implementation

	CAlbumFilterStore::CAlbumFilterStore( void )
		: fs::CFilterStore( _T("All Albums") )
	{
		AddFilter( str::LoadPair( IDS_SLIDE_ALBUM_FILTER_SPEC ) );
		AddFilter( str::LoadPair( IDS_CATALOG_STG_FILTER_SPEC ) );
	}

	CAlbumFilterStore& CAlbumFilterStore::Instance( void )
	{
		static CAlbumFilterStore albumStore;
		return albumStore;
	}

	std::tstring CAlbumFilterStore::MakeAlbumFilters( void ) const
	{
		const UINT positions[] = { SlideFilter, CatalogStgFilter };
		return MakeFilters( ARRAY_PAIR( positions ) );
	}

	std::tstring CAlbumFilterStore::MakeCatalogStgFilters( void ) const
	{
		const UINT positions[] = { CatalogStgFilter };
		return MakeFilters( ARRAY_PAIR( positions ) );
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
