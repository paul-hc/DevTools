
#include "stdafx.h"
#include "GeneralOptions.h"
#include "Application.h"
#include "utl/Image_fwd.h"
#include "utl/ReportListControl.h"
#include "utl/Thumbnailer.h"

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
	static const TCHAR entry_undoRedoLogPersist[] = _T("UndoRedoLogPersist");
	static const TCHAR entry_undoRedoLogFormat[] = _T("UndoRedoLogFormat");
}


CGeneralOptions::CGeneralOptions( void )
	: m_smallIconDim( CIconId::GetStdSize( SmallIcon ).cx )
	, m_largeIconDim( CIconId::GetStdSize( HugeIcon_48 ).cx )
	, m_useListThumbs( true )
	, m_useListDoubleBuffer( true )
	, m_undoRedoLogPersist( true )
	, m_undoRedoLogFormat( cmd::TextFormat )
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

void CGeneralOptions::LoadFromRegistry( void )
{
	CWinApp* pApp = AfxGetApp();

	m_smallIconDim = pApp->GetProfileInt( reg::section, reg::entry_smallIconDim, m_smallIconDim );
	m_largeIconDim = pApp->GetProfileInt( reg::section, reg::entry_largeIconDim, m_largeIconDim );
	m_useListThumbs = pApp->GetProfileInt( reg::section, reg::entry_useListThumbs, m_useListThumbs ) != FALSE;
	m_useListDoubleBuffer = pApp->GetProfileInt( reg::section, reg::entry_useListDoubleBuffer, m_useListDoubleBuffer ) != FALSE;
	m_undoRedoLogPersist = pApp->GetProfileInt( reg::section, reg::entry_undoRedoLogPersist, m_undoRedoLogPersist ) != FALSE;
	m_undoRedoLogFormat = static_cast< cmd::FileFormat >( pApp->GetProfileInt( reg::section, reg::entry_undoRedoLogFormat, m_undoRedoLogFormat ) );
}

void CGeneralOptions::SaveToRegistry( void ) const
{
	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileInt( reg::section, reg::entry_smallIconDim, m_smallIconDim );
	pApp->WriteProfileInt( reg::section, reg::entry_largeIconDim, m_largeIconDim );
	pApp->WriteProfileInt( reg::section, reg::entry_useListThumbs, m_useListThumbs );
	pApp->WriteProfileInt( reg::section, reg::entry_useListDoubleBuffer, m_useListDoubleBuffer );
	pApp->WriteProfileInt( reg::section, reg::entry_undoRedoLogPersist, m_undoRedoLogPersist );
	pApp->WriteProfileInt( reg::section, reg::entry_undoRedoLogFormat, m_undoRedoLogFormat );
}

const std::tstring& CGeneralOptions::GetCode( void ) const
{
	static const std::tstring s_code = _T("General");
	return s_code;
}

bool CGeneralOptions::operator==( const CGeneralOptions& right ) const
{
	return
		m_smallIconDim == right.m_smallIconDim &&
		m_largeIconDim == right.m_largeIconDim &&
		m_useListThumbs == right.m_useListThumbs &&
		m_useListDoubleBuffer == right.m_useListDoubleBuffer &&
		m_undoRedoLogPersist == right.m_undoRedoLogPersist &&
		m_undoRedoLogFormat == right.m_undoRedoLogFormat;
}

void CGeneralOptions::ApplyToListCtrl( CReportListControl* pListCtrl ) const
{
	ASSERT_PTR( pListCtrl );

	pListCtrl->ModifyListStyleEx( m_useListDoubleBuffer ? 0 : LVS_EX_DOUBLEBUFFER, m_useListDoubleBuffer ? LVS_EX_DOUBLEBUFFER : 0 );

	pListCtrl->SetCustomImageDraw( m_useListThumbs ? app::GetThumbnailer() : NULL,
		CSize( m_smallIconDim, m_smallIconDim ),
		CSize( m_largeIconDim, m_largeIconDim ) );
}
