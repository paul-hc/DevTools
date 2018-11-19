
#include "stdafx.h"
#include "DragListCtrl.h"
#include "ContainerUtilities.h"
#include "GpUtilities.h"
#include "OleDataSource.h"
#include "OleDropTarget.h"
#include "Range.h"
#include "Resequence.hxx"
#include "ShellUtilities.h"
#include "UtilitiesEx.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CDragListCtrl::CDragListCtrl( UINT columnLayoutId /*= 0*/, DWORD listStyleEx /*= lv::DefaultStyleEx*/ )
	: CReportListControl( columnLayoutId, listStyleEx )
	, m_draggingMode( NoDragging )
	, m_pSrcDragging( NULL )
	, m_dropIndex( -1 )
{
	SetDraggingMode( InternalDragging );
}

CDragListCtrl::~CDragListCtrl()
{
}

void CDragListCtrl::SetupControl( void )
{
	CReportListControl::SetupControl();

	if ( m_pDropTarget.get() != NULL )
		m_pDropTarget->Register( this );			// register this list as drop target
}

void CDragListCtrl::SetDraggingMode( DraggingMode draggingMode )
{
	if ( m_pDropTarget.get() != NULL )
		m_pDropTarget->Revoke();					// unregister as drop target

	m_draggingMode = draggingMode;
	m_pDropTarget.reset( m_draggingMode != NoDragging ? new ole::CDropTarget : NULL );

	if ( m_pDropTarget.get() != NULL )
	{
		m_pDropTarget->SetScrollMode( auto_scroll::Bars /*| auto_scroll::UseDefault*/ );
		m_pDropTarget->SetDropTipText( DROPIMAGE_MOVE, _T("Reorder"), _T("Move to %1") );
	}

	if ( m_hWnd != NULL && m_pDropTarget.get() != NULL )
		m_pDropTarget->Register( this );			// register this list as drop target
}

bool CDragListCtrl::DragSelection( CPoint dragPos, ole::CDataSource* pDataSource /*= NULL*/, int sourceFlags /*= ListSourcesMask*/,
								   DROPEFFECT dropEffect /*= DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK*/ )
{
	CListSelectionData selData( this );									// this will query the selected indexes
	std::auto_ptr< ole::CDataSource > pNewDataSource;
	if ( NULL == pDataSource )
	{
		pNewDataSource.reset( GetDataSourceFactory()->NewDataSource() );
		pDataSource = pNewDataSource.get();								// use local data source
	}

	if ( !CacheSelectionData( pDataSource, sourceFlags, selData ) )
		return false;

	pDataSource->GetDragImager().SetFromWindow( m_hWnd, &dragPos );		// will send a DI_GETDRAGIMAGE message to this list ctrl
	CDragInfo dragInfo( sourceFlags, selData.m_selIndexes );
	m_pSrcDragging = &dragInfo;
	m_dropIndex = -1;

	CScopedDisableDropTarget disableParentTarget( GetParent() );		// prevent showing the "+" drag copy effect when in list with LVHT_NOWHERE hit test

	dropEffect = pDataSource->DragAndDrop( dropEffect );				// drag drop the selected valid files
	EndDragging();
	m_pSrcDragging = NULL;
	return dropEffect != DROPEFFECT_NONE;
}

void CDragListCtrl::EndDragging( void )
{
	if ( !IsDragging() )
		return;

	HighlightDropMark( -1 );
	Invalidate();
}

bool CDragListCtrl::DropSelection( void )
{
	if ( !IsDragging() || !IsValidDropIndex() )
		return false;

	int dropIndex = m_dropIndex;
	std::vector< int > selIndexes = m_pSrcDragging->m_selIndexes;

	EndDragging();
	{
		CScopedLockRedraw freeze( this );
		CScopedInternalChange internalChange( this );

		DropMoveItems( dropIndex, selIndexes );
	}

	int caretIndex = GetCaretIndex();
	if ( caretIndex != -1 )
		EnsureVisible( caretIndex, FALSE );

	ui::SendCommandToParent( m_hWnd, lv::LVN_ItemsReorder );
	return true;
}

void CDragListCtrl::HandleDragging( CPoint dragPos )
{
	HighlightDropMark( GetDropIndexAtPoint( dragPos ) );
}

void CDragListCtrl::HighlightDropMark( int dropIndex )
{
	if ( dropIndex != m_dropIndex )
	{
		RedrawItem( m_dropIndex );
		m_dropIndex = dropIndex;

		const Range< int > indexRange( 0, GetItemCount() - 1 );
		if ( indexRange.Contains( dropIndex ) )
		{
			int prevIndex = std::max( dropIndex - 1, 0 );
			int nextIndex = std::min( dropIndex + 1, indexRange.m_end );

			if ( indexRange.Contains( prevIndex ) )
				EnsureVisible( prevIndex, FALSE );
			if ( indexRange.Contains( nextIndex ) )
				EnsureVisible( nextIndex, FALSE );
		}

		UpdateWindow();
		DrawDropMark();
	}
}

bool CDragListCtrl::DrawDropMark( void )
{
	if ( !IsValidDropIndex() )
		return false;

	CClientDC dc( this );
	CDropMark dropMark( this, m_dropIndex );
	dropMark.Draw( &dc );
	return true;
}

void CDragListCtrl::RedrawItem( int index )
{
	ASSERT( GetItemCount() != 0 );

	if ( index != -1 )
	{
		CDropMark dropMark( this, index );
		dropMark.Invalidate( this );
	}
}

bool CDragListCtrl::IsValidDropIndex( void ) const
{
	if ( m_pSrcDragging != NULL )
		return seq::ChangesDropSequenceAt( GetItemCount(), m_dropIndex, m_pSrcDragging->m_selIndexes );

	return m_dropIndex >= 0 && m_dropIndex <= GetItemCount();		// TODO: redirect to parent's callback interface
}


// ole::IDropTargetEventsStub interface

DROPEFFECT CDragListCtrl::Event_OnDragEnter( COleDataObject* pDataObject, DWORD keyState, CPoint point )
{
	// note: auto-scrolling drop targets call Event_OnDragEnter() and Event_OnDragLeave() repeatedly during the dragging operation
	return Event_OnDragOver( pDataObject, keyState, point );
}

DROPEFFECT CDragListCtrl::Event_OnDragOver( COleDataObject* pDataObject, DWORD keyState, CPoint point )
{
	pDataObject, keyState, point;

	DROPEFFECT dropEffect = DROPEFFECT_NONE;
	HandleDragging( point );
	if ( IsValidDropIndex() )
		dropEffect = DROPEFFECT_MOVE;

	return m_pDropTarget->FilterDropEffect( dropEffect );
}

DROPEFFECT CDragListCtrl::Event_OnDropEx( COleDataObject* pDataObject, DROPEFFECT dropEffect, DROPEFFECT dropList, CPoint point )
{
	pDataObject, dropList, point;

	if ( dropEffect != DROPEFFECT_NONE )
		DropSelection();
	return dropEffect;
}

void CDragListCtrl::Event_OnDragLeave( void )
{
	// don't call EndDragging() here, because auto-scroll drop targets call OnDragEnter() and OnDragLeave() repeatedly during the dragging operation
}


// message handlers

BEGIN_MESSAGE_MAP( CDragListCtrl, CReportListControl )
	ON_NOTIFY_REFLECT_EX( LVN_BEGINDRAG, OnLvnBeginDrag_Reflect )
	ON_MESSAGE( LVM_ENSUREVISIBLE, OnLVmEnsureVisible )
END_MESSAGE_MAP()

BOOL CDragListCtrl::OnLvnBeginDrag_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
{
	NMLISTVIEW* pNmListView = (NMLISTVIEW*)pNmHdr;
	pNmListView;

	*pResult = 0;
	switch ( m_draggingMode )
	{
		default: ASSERT( false );
		case ExternalDragging:
			return FALSE;				// allow handling by the parent, so that it can start dragging
		case InternalDragging:
			DragSelection( pNmListView->ptAction );			// pNmListView->iItem, pNmListView->ptAction
			// fall-through
		case NoDragging:
			return TRUE;				// supress notification to the parent
	}
}

LRESULT CDragListCtrl::OnLVmEnsureVisible( WPARAM wParam, LPARAM lParam )
{
	int index = static_cast< int >( wParam );
	bool partialOk = lParam != FALSE; partialOk;

	if ( IsDragging() && !ui::IsKeyPressed( VK_CONTROL ) )
	{
		// LVM_ENSUREVISIBLE is used by list-ctrl for auto-scrolling while dragging
		// slow-down auto-scrolling while dragging (unless CTRL is pressed)
		//
		if ( !IsItemFullyVisible( index ) )
			Sleep( 100 );
	}

	return Default();
}


// CDropMark implementation

CDropMark::CDropMark( const CListCtrl* pListCtrl, int dropIndex )
	: m_placement( dropIndex < pListCtrl->GetItemCount() ? BeforeItem : AfterItem )
	, m_orientation( GetOrientation( pListCtrl ) )
{
	ASSERT( pListCtrl->GetItemCount() != 0 );
	ASSERT( dropIndex != -1 );

	pListCtrl->GetItemRect( BeforeItem == m_placement ? dropIndex : ( dropIndex - 1 ), &m_markRect, LVIR_BOUNDS );
	if ( BeforeItem == m_placement )
		if ( HorizMark == m_orientation )
			m_markRect.bottom = m_markRect.top;				// fold to top
		else
			m_markRect.right = m_markRect.left;				// fold to left
	else
		if ( HorizMark == m_orientation )
			m_markRect.top = --m_markRect.bottom;			// fold to bottom
		else
			m_markRect.left = --m_markRect.right;			// fold to right

	if ( HorizMark == m_orientation )
		m_markRect.InflateRect( 0, PenWidth + ArrowExtent + 1 );
	else
		m_markRect.InflateRect( PenWidth + ArrowExtent + 1, 0 );

	CRect clientRect;
	pListCtrl->GetClientRect( &clientRect );
	ui::EnsureVisibleRect( m_markRect, clientRect );
}

void CDropMark::Draw( CDC* pDC )
{
	Pen pen( gp::MakeOpaqueColor( color::Red, 80 ), PenWidth );
	pen.SetAlignment( PenAlignmentCenter );

	GraphicsPath path( FillModeWinding );
	CPoint center = m_markRect.CenterPoint();
	Range< Point > core;						// the core line

	if ( HorizMark == m_orientation )
	{
		UINT edgeX = m_markRect.Width() * EdgePct / 100;
		edgeX += ArrowExtent;

		core.m_start.X = m_markRect.left + edgeX;
		core.m_end.X = m_markRect.right - edgeX;
		core.m_start.Y = core.m_end.Y = center.y;

		CRect bounds( core.m_start.X, core.m_start.Y, core.m_end.X, core.m_end.Y );
		bounds.InflateRect( ArrowExtent, ArrowExtent );

		path.StartFigure();			// LEFT inverted arrow
		path.AddLine( bounds.left, bounds.top, core.m_start.X, core.m_start.Y );
		path.AddLine( bounds.left, bounds.bottom, core.m_start.X, core.m_start.Y );

		path.StartFigure();			// CORE line
		path.AddLine( core.m_start, core.m_end );

		path.StartFigure();			// RIGHT inverted arrow
		path.AddLine( bounds.right, bounds.top, core.m_end.X, core.m_end.Y );
		path.AddLine( bounds.right, bounds.bottom, core.m_end.X, core.m_end.Y );
	}
	else
	{
		UINT edgeY = m_markRect.Height() * EdgePct / 100;
		edgeY += ArrowExtent;
		//edgeY = std::max( , edgeY );
		core.m_start.Y = m_markRect.top + edgeY;
		core.m_end.Y = m_markRect.bottom - edgeY;
		core.m_start.X = core.m_end.X = center.x;

		CRect bounds( core.m_start.X, core.m_start.Y, core.m_end.X, core.m_end.Y );
		bounds.InflateRect( ArrowExtent, ArrowExtent );

		path.StartFigure();			// TOP inverted arrow
		path.AddLine( bounds.left, bounds.top, core.m_start.X, core.m_start.Y );
		path.AddLine( bounds.right, bounds.top, core.m_start.X, core.m_start.Y );

		path.StartFigure();			// CORE line
		path.AddLine( core.m_start, core.m_end );

		path.StartFigure();			// BOTTOM inverted arrow
		path.AddLine( bounds.left, bounds.bottom, core.m_end.X, core.m_end.Y );
		path.AddLine( bounds.right, bounds.bottom, core.m_end.X, core.m_end.Y );
	}

	Graphics graphics( pDC->GetSafeHdc() );
	graphics.SetSmoothingMode( SmoothingModeAntiAlias );

	graphics.DrawPath( &pen, &path );
}

void CDropMark::DrawLine( CDC* pDC )
{
	Pen pen( gp::MakeOpaqueColor( color::Red, 80 ), PenWidth );
	pen.SetAlignment( PenAlignmentCenter );
	pen.SetStartCap( LineCapArrowAnchor );
	pen.SetEndCap( LineCapArrowAnchor );

	CPoint center = m_markRect.CenterPoint();

	Range< Point > line;
	if ( HorizMark == m_orientation )
	{
		UINT edgeX = m_markRect.Width() * EdgePct / 100;
		line.m_start.X = m_markRect.left + edgeX;
		line.m_end.X = m_markRect.right - edgeX;
		line.m_start.Y = line.m_end.Y = center.y;
	}
	else
	{
		UINT edgeY = m_markRect.Height() * EdgePct / 100;
		line.m_start.Y = m_markRect.top + edgeY;
		line.m_end.Y = m_markRect.bottom - edgeY;
		line.m_start.X = line.m_end.X = center.x;
	}

	Graphics graphics( pDC->GetSafeHdc() );
	graphics.SetSmoothingMode( SmoothingModeAntiAlias );

	graphics.DrawLine( &pen, line.m_start, line.m_end );
}

void CDropMark::Invalidate( CListCtrl* pListCtrl )
{
	CRect bounds = m_markRect;
	bounds.InflateRect( 2, 2 );
	pListCtrl->InvalidateRect( &bounds );
}

CDropMark::Orientation CDropMark::GetOrientation( const CListCtrl* pListCtrl )
{
	bool stacksVertically = EqFlag( pListCtrl->GetStyle(), LVS_ALIGNLEFT );
	switch ( pListCtrl->GetView() )
	{
		default: ASSERT( false );
		case LV_VIEW_LIST:
		case LV_VIEW_DETAILS:
			return HorizMark;		// always arranged top-to-bottom
		case LV_VIEW_ICON:
		case LV_VIEW_SMALLICON:
		case LV_VIEW_TILE:
			return stacksVertically ? HorizMark : VertMark;
	}
}
