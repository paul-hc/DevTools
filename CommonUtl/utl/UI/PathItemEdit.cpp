
#include "pch.h"
#include "PathItemEdit.h"
#include "CustomDrawImager.h"
#include "PathItemListCtrl.h"		// for CPathItemListCtrl::GetStdPathListPopupMenu()
#include "MenuUtilities.h"
#include "resource.h"
#include "utl/FileSystem.h"
#include "utl/TextClipboard.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CPathItemEdit::CPathItemEdit( bool useDirPath /*= false*/ )
	: CImageEdit()
	, CObjectCtrlBase( this )
	, m_useDirPath( false )
	, m_pCustomImager( new CFileGlyphCustomDrawImager( ui::SmallGlyph ) )
	, m_pathContent( ui::MixedPath )
{
	SetSubjectAdapter( ui::CPathPidlAdapter::InstanceUI() );
	SetImageList( m_pCustomImager->GetImageList() );
	SetImageIndex( m_pCustomImager->GetTranspImageIndex() );		// the one and only transparent image

	SetUseDirPath( useDirPath );
}

CPathItemEdit::~CPathItemEdit()
{
}

void CPathItemEdit::SetShellPath( const shell::TPath& shellPath )
{
	m_pathItem.SetShellPath( shellPath );
	UpdateControl();
}

void CPathItemEdit::SetPidl( const shell::CPidlAbsolute& pidl )
{
	m_pathItem.SetPidl( pidl );
	UpdateControl();
}

bool CPathItemEdit::HasValidImage( void ) const override
{
	return m_pathItem.ObjectExist();
}

void CPathItemEdit::UpdateControl( void ) override
{
	m_evalFilePath = GetShellPath().GetExpanded();

	if ( m_hWnd != nullptr )
	{
		SetText( FormatCode( &m_pathItem ) );
		SelectAll();		// scroll to end to make deepest subfolder visible
	}

	__super::UpdateControl();
}

void CPathItemEdit::DrawImage( CDC* pDC, const CRect& imageRect ) override
{
	// don't bother drawing the transparent image
	//__super::DrawImage( pDC, imageRect );

	if ( ui::ICustomImageDraw* pRenderer = m_pCustomImager->GetRenderer() )
		pRenderer->DrawItemImage( pDC, &m_pathItem, imageRect );
}

COLORREF CPathItemEdit::GetCustomTextColor( void ) const overrides(CTextEdit)
{
	if ( !m_pathItem.ObjectExist() )
		return color::ScarletRed;		// error text color: file does not exist
	else if ( m_pathItem.IsFilePath() )
	{
		if ( fs::IsValidDirectory( m_evalFilePath.GetPtr() ) )
			return ::GetSysColor( COLOR_HIGHLIGHT );	// directory text color
	}
	else if ( m_pathItem.IsSpecialPidl() )
		return color::Teal;			// special PIDL text color

	return __super::GetCustomTextColor();
}

void CPathItemEdit::QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const implement
{
	cmdId, pTooltip;

	if ( !m_pathItem.IsEmpty() )
		if ( m_pathItem.IsSpecialPidl() )
			rText = m_pathItem.GetPidl().GetName( SIGDN_DESKTOPABSOLUTEPARSING );		// display the GUID path in the tooltip
		else if ( !m_pathItem.ObjectExist() )
		{
			static const std::tstring s_fileNotFound = _T("File not found");

			if ( GetShellPath().HasEnvironVar() )
				rText = s_fileNotFound + _T(":  ") + m_evalFilePath.Get();
			else
				rText = s_fileNotFound + _T('!');
		}
		else if ( GetShellPath().HasEnvironVar() )
			rText = m_evalFilePath.Get();				// display the expanded path
}

CMenu* CPathItemEdit::GetPopupMenu( void )
{
	CMenu* pSrcPopupMenu = &CPathItemListCtrl::GetStdPathListPopupMenu( CReportListControl::OnSelection );

	if ( pSrcPopupMenu != nullptr && !m_pathItem.IsEmpty() )
	{
		if ( CMenu* pContextPopup = MakeContextMenuHost( pSrcPopupMenu, GetShellPath() ) )		// augment File Explorer context menu as sub-menu
			return pContextPopup;
	}

	return pSrcPopupMenu;
}

void CPathItemEdit::PreSubclassWindow( void )
{
	__super::PreSubclassWindow();

	if ( !m_pathItem.IsEmpty() )
	{
		SetText( FormatCode( &m_pathItem ) );
		SelectAll();		// scroll to end to make deepest subfolder visible
	}
}

BOOL CPathItemEdit::OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( HandleCmdMsg( id, code, pExtra, pHandlerInfo ) )
		return true;

	return __super::OnCmdMsg( id, code, pExtra, pHandlerInfo );
}


// message handlers

BEGIN_MESSAGE_MAP( CPathItemEdit, CImageEdit )
	ON_WM_CONTEXTMENU()
	ON_COMMAND( ID_ITEM_COPY_FILENAMES, OnCopyFilename )
	ON_UPDATE_COMMAND_UI( ID_ITEM_COPY_FILENAMES, OnUpdateHasAny )
	ON_COMMAND( ID_ITEM_COPY_FOLDERS, OnCopyFolder )
	ON_UPDATE_COMMAND_UI( ID_ITEM_COPY_FOLDERS, OnUpdateHasAny )
	ON_COMMAND( ID_FILE_PROPERTIES, OnFileProperties )
	ON_UPDATE_COMMAND_UI( ID_FILE_PROPERTIES, OnUpdateHasAny )
	ON_COMMAND_RANGE( ID_BROWSE_FILE, ID_BROWSE_FOLDER, OnBrowsePath )
	ON_UPDATE_COMMAND_UI_RANGE( ID_BROWSE_FILE, ID_BROWSE_FOLDER, OnUpdateAlways )
END_MESSAGE_MAP()

void CPathItemEdit::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	pWnd;
	if ( CMenu* pPopupMenu = GetPopupMenu() )
		DoTrackContextMenu( pPopupMenu, screenPos );
}

void CPathItemEdit::OnEditCopy( void ) override
{
	if ( m_pathItem.IsSpecialPidl() )
		CTextClipboard::CopyText( GetShellPath().Get(), m_hWnd );	// copy the GUID path
	else
		__super::OnEditCopy();
}

void CPathItemEdit::OnCopyFilename( void )
{
	if ( !CTextClipboard::CopyText( ui::CPathPidlAdapter::InstanceDisplay()->FormatCode( &m_pathItem ), m_hWnd ) )
		ui::BeepSignal( MB_ICONWARNING );
}

void CPathItemEdit::OnCopyFolder( void )
{
	fs::CPath dirPath( m_pathItem.IsSpecialPidl() ? m_pathItem.GetPidl().GetName( SIGDN_DESKTOPABSOLUTEEDITING ) : GetShellPath().Get());

	if ( !m_useDirPath )		// !fs::IsValidDirectory( m_evalFilePath.GetPtr() )
		dirPath = dirPath.GetParentPath();

	if ( !CTextClipboard::CopyText( dirPath.Get(), m_hWnd ) )
		ui::BeepSignal( MB_ICONWARNING );
}

void CPathItemEdit::OnFileProperties( void )
{
	std::vector<fs::CPath> filePaths( 1, m_pathItem.FormatPhysical() );

	ShellInvokeProperties( filePaths );
}

void CPathItemEdit::OnBrowsePath( UINT cmdId )
{
	shell::TPath newShellPath = m_pathContent.EditItem( GetShellPath().GetPtr(), this, cmdId );
	bool modify = newShellPath != GetShellPath();

	if ( !newShellPath.IsEmpty() )
	{
		SetShellPath( newShellPath );

		if ( modify )
			SetModify();
	}
}

void CPathItemEdit::OnUpdateHasPath( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( m_pathItem.IsFilePath() );
}

void CPathItemEdit::OnUpdateHasAny( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_pathItem.IsEmpty() );
}

void CPathItemEdit::OnUpdateAlways( CCmdUI* pCmdUI )
{
	pCmdUI->Enable();
}
