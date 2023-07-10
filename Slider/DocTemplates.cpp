
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
#include "utl/UI/WndUtils.h"
#include "utl/UI/WicImage.h"
#include <unordered_set>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace app
{
	static const TCHAR s_verb_OpenWithSlider[] = _T("OpenWithSlider");
	static const TCHAR s_verb_EnqueueInSlider[] = _T("EnqueueInSlider");

	reg::TKeyPath MakeShellHandlerVerbFullPath( const TCHAR imageExt[], const TCHAR verb[] = app::s_verb_OpenWithSlider )
	{
		REQUIRE( !str::IsEmpty( imageExt ) );

		reg::TKeyPath openKeyPath;
		std::tstring handlerName;

		if ( shell::QueryHandlerName( handlerName, imageExt ) )		// valid indirect shell extension handler?
			openKeyPath = reg::TKeyPath( _T("HKEY_CLASSES_ROOT") ) / shell::MakeShellHandlerVerbPath( handlerName.c_str(), verb );	// "HKEY_CLASSES_ROOT\\<handler>\\shell\\OpenWithSlider"

		return openKeyPath;
	}


	bool PromptFileDialogImpl( CString& rFilePath, const fs::CFilterJoiner& filterJoiner, UINT titleId, DWORD flags, BOOL openDlg )
	{
		fs::CPath filePath( rFilePath.GetString() );
		std::tstring title = str::Load( titleId );

		if ( !filterJoiner.BrowseFile( filePath, static_cast<shell::BrowseMode>( openDlg ), flags, nullptr, nullptr, !title.empty() ? title.c_str() : nullptr ) )
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

	m_pAlbumTemplate->OpenDocumentFile( nullptr );
}

BOOL CAppDocManager::DoPromptFileName( CString& rFilePath, UINT titleId, DWORD flags, BOOL openDlg, CDocTemplate* pTemplate )
{
	if ( app::CSharedDocTemplate* pSharedTemplate = dynamic_cast<app::CSharedDocTemplate*>( pTemplate ) )
		return pSharedTemplate->PromptFileDialog( rFilePath, titleId, flags, static_cast<shell::BrowseMode>( openDlg ) );

	return app::PromptFileDialogImpl( rFilePath, app::CSliderFilters::Instance(), titleId, flags, openDlg );
}

void CAppDocManager::RegisterShellFileTypes( BOOL compatMode )
{
	{
		CScopedValue<bool> scopedSingleExt( &app::CSharedDocTemplate::s_useSingleFilterExt, true );		// prevent creating multi-extension ".sld;.ias;.cid;.icf" registry key
		__super::RegisterShellFileTypes( compatMode );			// CAlbumDocTemplate + CImageDocTemplate
	}

	m_pAlbumTemplate->RegisterAdditionalDocExtensions();		// ".sld", ".ias", ".cid", ".icf"
	m_pAlbumTemplate->RegisterAlbumShell_Directory( true );		// "HKEY_CLASSES_ROOT\Directory\shell\Slide &View"
}

/* Handlers sample (Windows 10 MyThinkPad): "<HANDLER>": "<EXT>"

	"Photoshop.BMPFile.10": ".bmp"
	"Paint.Picture": ".dib"
	"rlefile": ".rle"
	"giffile": ".gif"
	"IcoFX.ico": ".ico"
	"IcoFX.cur": ".cur"
	"jpegfile": ".jpeg", ".jpe", ".jpg"
	"pjpegfile": ".jfif"
	"pngfile": ".png"
	"Photoshop.TIFFFile.10": ".tiff", ".tif"
	"Photoshop.CameraRawFileDigital.10": ".dng"
	"wdpfile": ".wdp", ".jxr"
	"ddsfile": ".dds"
	"Photoshop.CameraRawFileCanon2.10": ".CR2"
	"Photoshop.CameraRawFileCanon.10": ".CRW"
	"dcsfile": ".DCS"
	"Photoshop.CameraRawFileKodak.10": ".DCR"
	"Photoshop.CameraRawFileEpson.10": ".ERF"
	"Photoshop.CameraRawFileLeaf.10": ".MOS"
	"Photoshop.CameraRawFileMinolta.10": ".MRW"
	"Photoshop.CameraRawFileNikon.10": ".NEF"
	"Photoshop.CameraRawFileOlympus.10": ".ORF"
	"Photoshop.CameraRawFilePentax.10": ".PEF"
	"Photoshop.CameraRawFileFujifilm.10": ".RAF"
	"VisualStudio.srf.9.0": ".SRF"
	"Photoshop.CameraRawFileFoveon.10": ".X3F"
*/
void CAppDocManager::RegisterImageAdditionalShellExt( bool doRegister )
{
	// process known registered image files extensions
	const std::vector<std::tstring>& imageExts = app::CImageDocTemplate::Instance()->GetAllExts();
	std::tstring handlerName;
	std::unordered_set<std::tstring> uniqueHandlers;
	bool accessDenied = false;

	for ( std::vector<std::tstring>::const_iterator itImageExt = imageExts.begin(); itImageExt != imageExts.end() && !accessDenied; ++itImageExt )
	{
		if ( shell::QueryHandlerName( handlerName, itImageExt->c_str() ) )		// valid indirect shell extension handler?
			if ( uniqueHandlers.insert( handlerName ).second )					// first entry encountered?
			{	// e.g. "Photoshop.BMPFile.10", "Paint.Picture", "rlefile", "giffile", "IcoFX.ico", "IcoFX.cur", "jpegfile", 
				reg::TKeyPath openKeyPath = shell::MakeShellHandlerVerbPath( handlerName.c_str(), app::s_verb_OpenWithSlider );			// "<handler>\\shell\\OpenWithSlider"
				reg::TKeyPath enqueueKeyPath = shell::MakeShellHandlerVerbPath( handlerName.c_str(), app::s_verb_EnqueueInSlider );		// "<handler>\\shell\\EnqueueInSlider" - obsolete

				if ( doRegister )
				{
					if ( !shell::RegisterShellVerb( openKeyPath, app::GetModulePath(), _T("Open with &Slider"), shell::GetDdeOpenCmd() ) )
						accessDenied = reg::CKey::IsLastError_AccessDenied();

					//shell::RegisterShellVerb( enqueueKeyPath, app::GetModulePath(), _T("Enque&ue in Slider"), _T("[queue(\"%1\")]") );	// PHC 2023-07-05: remove the obsolete "[queue(\"%1\")" DDE command
				}
				else
				{	// remove slider entries for shell
					if ( !shell::UnregisterShellVerb( openKeyPath ) )
						accessDenied = reg::CKey::IsLastError_AccessDenied();

					if ( !shell::UnregisterShellVerb( enqueueKeyPath ) )
						accessDenied = reg::CKey::IsLastError_AccessDenied();
				}
			}
	}

	if ( accessDenied )
		ui::MessageBox( _T("WARNING:\nCannot register Slider application as handler for the known image file types!\n\nYou must run the program as Administrator."), MB_OK | MB_ICONWARNING );
}

bool CAppDocManager::IsAppRegisteredForImageExt( const TCHAR imageExt[] /*= _T(".bmp")*/ )
{
	reg::TKeyPath openKeyPath = app::MakeShellHandlerVerbFullPath( imageExt );

	if ( !openKeyPath.IsEmpty() )		// valid indirect shell extension handler?
		return reg::KeyExist( openKeyPath.GetPtr(), KEY_READ );

	return false;
}

bool CAppDocManager::CanRegisterImageExt( const TCHAR imageExt[] /*= _T(".bmp")*/ )
{
	reg::TKeyPath openKeyPath = app::MakeShellHandlerVerbFullPath( imageExt );
	bool accessDenied;

	if ( !openKeyPath.IsEmpty() )		// valid indirect shell extension handler?
		return reg::IsKeyWritable( openKeyPath.GetPtr(), &accessDenied ) && !accessDenied;

	return false;
}


namespace app
{
	// CSharedDocTemplate implementation

	bool CSharedDocTemplate::s_useSingleFilterExt = false;

	CSharedDocTemplate::CSharedDocTemplate( UINT idResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass )
		: CMultiDocTemplate( idResource, pDocClass, pFrameClass, pViewClass )
		, m_pFilterStore( nullptr )
		, m_acceptDirPath( false )
		, m_accel( m_hAccelTable )
	{
		m_menu.Attach( m_hMenuShared );
		VERIFY( ui::SetMenuImages( m_menu ) );
	}

	CSharedDocTemplate::~CSharedDocTemplate()
	{	// prevent deleting shared resources by CMultiDocTemplate dtor (owned by this class)
		m_hMenuShared = nullptr;
		m_hAccelTable = nullptr;
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
		static CImageDocTemplate* s_pImageDocTemplate = new CImageDocTemplate();		// owned by the doc manager
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
		static CAlbumDocTemplate* s_pAlbumDocTemplate = new CAlbumDocTemplate();		// owned by the doc manager
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

	void CAlbumDocTemplate::RegisterAlbumShell_Directory( bool doRegister )
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
		return MakeFilters( ARRAY_SPAN( positions ) );
	}

	std::tstring CAlbumFilterStore::MakeCatalogStgFilters( void ) const
	{
		const UINT positions[] = { CatalogStgFilter };
		return MakeFilters( ARRAY_SPAN( positions ) );
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
