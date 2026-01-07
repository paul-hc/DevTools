
#include "pch.h"
#include "PathItemEdit.h"
#include "CustomDrawImager.h"
#include "PathItemListCtrl.h"		// for CPathItemListCtrl::GetStdPathListPopupMenu()
#include "MenuUtilities.h"
#include "resource.h"
#include "utl/TextClipboard.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CPathItemEdit::CPathItemEdit( void )
	: CImageEdit()
	, CObjectCtrlBase( this )
	, m_pathItem( fs::CPath() )
	, m_pCustomImager( new CFileGlyphCustomDrawImager( ui::SmallGlyph ) )
{
	SetSubjectAdapter( ui::GetFullPathAdapter() );
	SetImageList( m_pCustomImager->GetImageList() );
	SetImageIndex( m_pCustomImager->GetTranspImageIndex() );		// the one and only transparent image
}

CPathItemEdit::~CPathItemEdit()
{
}

void CPathItemEdit::SetFilePath( const fs::CPath& filePath )
{
	m_pathItem.SetFilePath( filePath );

	if ( m_hWnd != nullptr )
	{
		SetText( FormatCode( &m_pathItem ) );
		SelectAll();		// scroll to end to make deepest subfolder visible

		UpdateControl();
	}
}

bool CPathItemEdit::HasValidImage( void ) const override
{
	const fs::CPath& filePath = m_pathItem.GetFilePath();

	return !filePath.IsEmpty() && filePath.FileExist();
}

void CPathItemEdit::DrawImage( CDC* pDC, const CRect& imageRect ) override
{
	// don't bother drawing the transparent image
	//__super::DrawImage( pDC, imageRect );

	if ( ui::ICustomImageDraw* pRenderer = m_pCustomImager->GetRenderer() )
		pRenderer->DrawItemImage( pDC, &m_pathItem, imageRect );
}

CMenu* CPathItemEdit::GetPopupMenu( void )
{
	CMenu* pSrcPopupMenu = &CPathItemListCtrl::GetStdPathListPopupMenu( CReportListControl::OnSelection );

	if ( pSrcPopupMenu != nullptr )
	{
		std::vector<fs::CPath> filePaths( 1, m_pathItem.GetFilePath() );

		if ( CMenu* pContextPopup = MakeContextMenuHost( pSrcPopupMenu, filePaths ) )		// augment File Explorer context menu as sub-menu
			return pContextPopup;
	}

	return pSrcPopupMenu;
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
	ON_WM_CTLCOLOR_REFLECT()
	ON_COMMAND( ID_ITEM_COPY_FILENAMES, OnCopyFilename )
	ON_UPDATE_COMMAND_UI( ID_ITEM_COPY_FILENAMES, OnUpdateHasPath )
	ON_COMMAND( ID_ITEM_COPY_FOLDERS, OnCopyFolder )
	ON_UPDATE_COMMAND_UI( ID_ITEM_COPY_FOLDERS, OnUpdateHasPath )
	ON_COMMAND( ID_FILE_PROPERTIES, OnFileProperties )
	ON_UPDATE_COMMAND_UI( ID_FILE_PROPERTIES, OnUpdateHasPath )
END_MESSAGE_MAP()

void CPathItemEdit::OnContextMenu( CWnd* pWnd, CPoint screenPos )
{
	pWnd;
	if ( CMenu* pPopupMenu = GetPopupMenu() )
		DoTrackContextMenu( pPopupMenu, screenPos );
}

HBRUSH CPathItemEdit::CtlColor( CDC* pDC, UINT ctlColor )
{
	ctlColor;

	const fs::CPath& filePath = m_pathItem.GetFilePath();
	bool readOnly = IsReadOnly();
	COLORREF textColor = CLR_NONE;

	if ( !filePath.IsEmpty() && !filePath.FileExist() )
		textColor = color::ScarletRed;		// error text color: file does not exist
	//else if ( fs::IsValidDirectory( filePath.GetPtr() ) )
	//	textColor = ::GetSysColor( COLOR_HIGHLIGHT );	// directory text color

	if ( CLR_NONE == textColor && !readOnly )
		return nullptr;		// no color customization

	COLORREF bkColor = ::GetSysColor( readOnly ? COLOR_BTNFACE : COLOR_WINDOW );
	HBRUSH hBkBrush = ::GetSysColorBrush( readOnly ? COLOR_BTNFACE : COLOR_WINDOW );

	pDC->SetBkColor( bkColor );

	if ( textColor != CLR_NONE )
		pDC->SetTextColor( textColor );

	return hBkBrush;
}

void CPathItemEdit::OnUpdateHasPath( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( !m_pathItem.GetFilePath().IsEmpty() );
}

void CPathItemEdit::OnCopyFilename( void )
{
	if ( !CTextClipboard::CopyText( m_pathItem.GetFilePath().GetFilenamePtr(), m_hWnd ) )
		ui::BeepSignal( MB_ICONWARNING );
}

void CPathItemEdit::OnCopyFolder( void )
{
	fs::CPath dirPath = m_pathItem.GetFilePath();

	if ( !fs::IsValidDirectory( dirPath.GetPtr() ) )
		dirPath = dirPath.GetParentPath();

	if ( !CTextClipboard::CopyText( dirPath.Get(), m_hWnd ) )
		ui::BeepSignal( MB_ICONWARNING );
}

void CPathItemEdit::OnFileProperties( void )
{
	std::vector<fs::CPath> filePaths( 1, m_pathItem.GetFilePath() );

	ShellInvokeProperties( filePaths );
}
