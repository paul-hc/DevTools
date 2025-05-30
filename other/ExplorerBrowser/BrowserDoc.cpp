
#include "pch.h"
#include "BrowserDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_view[] = _T("Settings\\View");
	static const TCHAR section_selItems[] = _T("Settings\\SelItems");
	static const TCHAR entry_lastDirPath[] = _T("LastDirPath");
	static const TCHAR entry_viewMode[] = _T("ViewMode");
}


IMPLEMENT_DYNCREATE( CBrowserDoc, CDocument )

CBrowserDoc::CBrowserDoc( void )
	: CDocument()
	, m_filePaneViewMode( static_cast<FOLDERVIEWMODE>( AfxGetApp()->GetProfileInt( reg::section_view, reg::entry_viewMode, FVM_DETAILS ) ) )
{
}

CBrowserDoc::~CBrowserDoc()
{
}

BOOL CBrowserDoc::OnNewDocument( void )
{
	if ( !CDocument::OnNewDocument() )
		return FALSE;

	std::tstring selDirPath = AfxGetApp()->GetProfileString( reg::section_view, reg::entry_lastDirPath ).GetString();
	if ( !fs::IsValidDirectory( selDirPath.c_str() ) )
	{
		// use current working directory
		TCHAR currDirPath[ MAX_PATH ];
		if ( ::GetCurrentDirectory( MAX_PATH, currDirPath ) )
			selDirPath = currDirPath;
	}

	if ( fs::IsValidDirectory( selDirPath.c_str() ) )
		SetPathName( selDirPath.c_str(), TRUE );

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)
	return TRUE;
}

BOOL CBrowserDoc::OnOpenDocument( LPCTSTR pDirPath )
{
	if ( !fs::IsValidDirectory( pDirPath ) )
		return false;

	SetPathName( pDirPath, TRUE );
	SetModifiedFlag( FALSE );			// start off with unmodified
	return TRUE;
}

void CBrowserDoc::OnCloseDocument( void )
{
	m_bAutoDelete = FALSE;				// we'll destroy the document at the end of this function
	CDocument::OnCloseDocument();		// destroy all views; that will store current view mode and dirPath in this document

	CWinApp* pApp = AfxGetApp();
	pApp->WriteProfileString( reg::section_view, reg::entry_lastDirPath, GetPathName() );
	pApp->WriteProfileInt( reg::section_view, reg::entry_viewMode, m_filePaneViewMode );
	pApp->WriteProfileString( reg::section_selItems, GetPathName().GetString(), m_selItems.c_str() );		// use dir path as registry value name

	delete this;
}

void CBrowserDoc::Serialize( CArchive& ar )
{
	if ( ar.IsStoring() )
	{
	}
	else
	{
	}
}

bool CBrowserDoc::QuerySelItems( std::vector<std::tstring>& rSelItems )
{
	m_selItems = AfxGetApp()->GetProfileString( reg::section_selItems, GetPathName().GetString() ).GetString();		// use dir path as registry value name
	str::Split( rSelItems, m_selItems.c_str(), _T("|") );
	return !m_selItems.empty();
}

void CBrowserDoc::StoreSelItems( const std::vector<std::tstring>& selPaths )
{
	std::vector<std::tstring> selFilenames; selFilenames.reserve( selPaths.size() );

	for ( std::vector<std::tstring>::const_iterator itSelPath = selPaths.begin(); itSelPath != selPaths.end(); ++itSelPath )
		selFilenames.push_back( ::PathFindFileName( itSelPath->c_str() ) );			// just the filename

	m_selItems = str::Join( selFilenames, _T("|") );
}


// command handlers

BEGIN_MESSAGE_MAP( CBrowserDoc, CDocument )
END_MESSAGE_MAP()
