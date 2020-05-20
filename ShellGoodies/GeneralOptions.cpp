
#include "stdafx.h"
#include "GeneralOptions.h"
#include "Application.h"
#include "utl/UI/Image_fwd.h"
#include "utl/UI/CustomDrawImager.h"
#include "utl/UI/ReportListControl.h"
#include "utl/UI/Thumbnailer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section[] = _T("Settings\\GeneralOptions");
	static const TCHAR entry_smallIconDim[] = _T("IconDimensionSmall");
	static const TCHAR entry_largeIconDim[] = _T("IconDimensionLarge");
	static const TCHAR entry_useListThumbs[] = _T("UseListThumbs");
	static const TCHAR entry_useListDoubleBuffer[] = _T("UseListDoubleBuffer");
	static const TCHAR entry_highlightTextDiffsFrame[] = _T("HighlightTextDiffsFrame");
	static const TCHAR entry_undoLogPersist[] = _T("UndoLogPersist");
	static const TCHAR entry_undoLogFormat[] = _T("UndoLogFormat");
	static const TCHAR entry_undoEditingCmds[] = _T("UndoEditingCmds");
	static const TCHAR entry_trimFname[] = _T("TrimFname");
	static const TCHAR entry_normalizeWhitespace[] = _T("NormalizeWhitespace");
}


CGeneralOptions::CGeneralOptions( void )
	: m_smallIconDim( CFileItemsThumbnailStore::GetDefaultGlyphDimension( ui::SmallGlyph ) )
	, m_largeIconDim( CFileItemsThumbnailStore::GetDefaultGlyphDimension( ui::LargeGlyph ) )
	, m_useListThumbs( true )
	, m_useListDoubleBuffer( true )
	, m_highlightTextDiffsFrame( true )
	, m_undoLogPersist( true )
	, m_undoLogFormat( cmd::BinaryFormat )
	, m_undoEditingCmds( true )
	, m_trimFname( true )
	, m_normalizeWhitespace( true )
{
}

CGeneralOptions::~CGeneralOptions()
{
}

CGeneralOptions& CGeneralOptions::Instance( void )
{
	static CGeneralOptions generalOptions;
	return generalOptions;
}

const std::tstring& CGeneralOptions::GetCode( void ) const
{
	static const std::tstring s_code = _T("General Options");
	return s_code;
}

void CGeneralOptions::LoadFromRegistry( void )
{
	CWinApp* pApp = AfxGetApp();

	m_smallIconDim = pApp->GetProfileInt( reg::section, reg::entry_smallIconDim, m_smallIconDim );
	m_largeIconDim = pApp->GetProfileInt( reg::section, reg::entry_largeIconDim, m_largeIconDim );
	m_useListThumbs = pApp->GetProfileInt( reg::section, reg::entry_useListThumbs, m_useListThumbs ) != FALSE;
	m_useListDoubleBuffer = pApp->GetProfileInt( reg::section, reg::entry_useListDoubleBuffer, m_useListDoubleBuffer ) != FALSE;
	m_highlightTextDiffsFrame = pApp->GetProfileInt( reg::section, reg::entry_highlightTextDiffsFrame, m_highlightTextDiffsFrame ) != FALSE;
	m_undoLogPersist = pApp->GetProfileInt( reg::section, reg::entry_undoLogPersist, m_undoLogPersist ) != FALSE;
	m_undoLogFormat = static_cast< cmd::FileFormat >( pApp->GetProfileInt( reg::section, reg::entry_undoLogFormat, m_undoLogFormat ) );
	m_undoEditingCmds = pApp->GetProfileInt( reg::section, reg::entry_undoEditingCmds, m_undoEditingCmds ) != FALSE;
	m_trimFname = pApp->GetProfileInt( reg::section, reg::entry_trimFname, m_trimFname ) != FALSE;
	m_normalizeWhitespace = pApp->GetProfileInt( reg::section, reg::entry_normalizeWhitespace, m_normalizeWhitespace ) != FALSE;
}

void CGeneralOptions::SaveToRegistry( void ) const
{
	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileInt( reg::section, reg::entry_smallIconDim, m_smallIconDim );
	pApp->WriteProfileInt( reg::section, reg::entry_largeIconDim, m_largeIconDim );
	pApp->WriteProfileInt( reg::section, reg::entry_useListThumbs, m_useListThumbs );
	pApp->WriteProfileInt( reg::section, reg::entry_useListDoubleBuffer, m_useListDoubleBuffer );
	pApp->WriteProfileInt( reg::section, reg::entry_highlightTextDiffsFrame, m_highlightTextDiffsFrame );
	pApp->WriteProfileInt( reg::section, reg::entry_undoLogPersist, m_undoLogPersist );
	pApp->WriteProfileInt( reg::section, reg::entry_undoLogFormat, m_undoLogFormat );
	pApp->WriteProfileInt( reg::section, reg::entry_undoEditingCmds, m_undoEditingCmds );
	pApp->WriteProfileInt( reg::section, reg::entry_trimFname, m_trimFname );
	pApp->WriteProfileInt( reg::section, reg::entry_normalizeWhitespace, m_normalizeWhitespace );
}

void CGeneralOptions::PostApply( void ) const
{
	CFileItemsThumbnailStore& rThumbnailStore = CFileItemsThumbnailStore::Instance();
	rThumbnailStore.SetGlyphDimension( ui::SmallGlyph, m_smallIconDim );
	rThumbnailStore.SetGlyphDimension( ui::LargeGlyph, m_largeIconDim );
	rThumbnailStore.UpdateControls();

	SaveToRegistry();
}

bool CGeneralOptions::operator==( const CGeneralOptions& right ) const
{
	return
		m_smallIconDim == right.m_smallIconDim &&
		m_largeIconDim == right.m_largeIconDim &&
		m_useListThumbs == right.m_useListThumbs &&
		m_useListDoubleBuffer == right.m_useListDoubleBuffer &&
		m_highlightTextDiffsFrame == right.m_highlightTextDiffsFrame &&
		m_undoLogPersist == right.m_undoLogPersist &&
		m_undoLogFormat == right.m_undoLogFormat &&
		m_undoEditingCmds == right.m_undoEditingCmds &&
		m_trimFname == right.m_trimFname &&
		m_normalizeWhitespace == right.m_normalizeWhitespace;
}

void CGeneralOptions::ApplyToListCtrl( CReportListControl* pListCtrl ) const
{
	ASSERT_PTR( pListCtrl );

	pListCtrl->SetHighlightTextDiffsFrame( m_highlightTextDiffsFrame );
	pListCtrl->SetCustomFileGlyphDraw( m_useListThumbs );
	pListCtrl->ModifyListStyleEx( m_useListDoubleBuffer ? 0 : LVS_EX_DOUBLEBUFFER, m_useListDoubleBuffer ? LVS_EX_DOUBLEBUFFER : 0 );
}
