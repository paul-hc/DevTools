#ifndef DragListCtrl_hxx
#define DragListCtrl_hxx

#include "ContainerUtilities.h"
#include "OleDataSource.h"
#include "OleDropTarget.h"
#include "Range.h"
#include "Resequence.hxx"
#include "ShellUtilities.h"
#include "Utilities.h"
#include "resource.h"


// CBaseDetailHostCtrl template code


template< typename BaseListCtrl >
CDragListCtrl< BaseListCtrl >::CDragListCtrl( UINT columnLayoutId /*= 0*/, DWORD listStyleEx /*= lv::DefaultStyleEx*/ )
	: BaseListCtrl( columnLayoutId, listStyleEx )
	, m_draggingMode( NoDragging )
	, m_pSrcDragging( NULL )
	, m_dropIndex( -1 )
{
	SetDraggingMode( InternalDragging );
}

template< typename BaseListCtrl >
CDragListCtrl< BaseListCtrl >::~CDragListCtrl()
{
}

template< typename BaseListCtrl >
void CDragListCtrl< BaseListCtrl >::SetupControl( void )
{
	BaseListCtrl::SetupControl();

	if ( m_pDropTarget.get() != NULL )
		m_pDropTarget->Register( this );			// register this list as drop target
}

template< typename BaseListCtrl >
void CDragListCtrl< BaseListCtrl >::SetDraggingMode( DraggingMode draggingMode )
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

template< typename BaseListCtrl >
bool CDragListCtrl< BaseListCtrl >::DragSelection( CPoint dragPos, ole::CDataSource* pDataSource /*= NULL*/, int sourceFlags /*= ListSourcesMask*/,
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

template< typename BaseListCtrl >
void CDragListCtrl< BaseListCtrl >::EndDragging( void )
{
	if ( !IsDragging() )
		return;

	HighlightDropMark( -1 );
	Invalidate();
}

template< typename BaseListCtrl >
bool CDragListCtrl< BaseListCtrl >::DropSelection( void )
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

template< typename BaseListCtrl >
void CDragListCtrl< BaseListCtrl >::HandleDragging( CPoint dragPos )
{
	HighlightDropMark( GetDropIndexAtPoint( dragPos ) );
}

template< typename BaseListCtrl >
void CDragListCtrl< BaseListCtrl >::HighlightDropMark( int dropIndex )
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

template< typename BaseListCtrl >
bool CDragListCtrl< BaseListCtrl >::DrawDropMark( void )
{
	if ( !IsValidDropIndex() )
		return false;

	CClientDC dc( this );
	CDropMark dropMark( this, m_dropIndex );
	dropMark.Draw( &dc );
	return true;
}

template< typename BaseListCtrl >
void CDragListCtrl< BaseListCtrl >::RedrawItem( int index )
{
	ASSERT( GetItemCount() != 0 );

	if ( index != -1 )
	{
		CDropMark dropMark( this, index );
		dropMark.Invalidate( this );
	}
}

template< typename BaseListCtrl >
bool CDragListCtrl< BaseListCtrl >::IsValidDropIndex( void ) const
{
	if ( m_pSrcDragging != NULL )
		return seq::ChangesDropSequenceAt( GetItemCount(), m_dropIndex, m_pSrcDragging->m_selIndexes );

	return m_dropIndex >= 0 && m_dropIndex <= GetItemCount();		// TODO: redirect to parent's callback interface
}


// ole::IDropTargetEventsStub interface

template< typename BaseListCtrl >
DROPEFFECT CDragListCtrl< BaseListCtrl >::Event_OnDragEnter( COleDataObject* pDataObject, DWORD keyState, CPoint point )
{
	// note: auto-scrolling drop targets call Event_OnDragEnter() and Event_OnDragLeave() repeatedly during the dragging operation
	return Event_OnDragOver( pDataObject, keyState, point );
}

template< typename BaseListCtrl >
DROPEFFECT CDragListCtrl< BaseListCtrl >::Event_OnDragOver( COleDataObject* pDataObject, DWORD keyState, CPoint point )
{
	pDataObject, keyState, point;

	DROPEFFECT dropEffect = DROPEFFECT_NONE;
	HandleDragging( point );
	if ( IsValidDropIndex() )
		dropEffect = DROPEFFECT_MOVE;

	return m_pDropTarget->FilterDropEffect( dropEffect );
}

template< typename BaseListCtrl >
DROPEFFECT CDragListCtrl< BaseListCtrl >::Event_OnDropEx( COleDataObject* pDataObject, DROPEFFECT dropEffect, DROPEFFECT dropList, CPoint point )
{
	pDataObject, dropList, point;

	if ( dropEffect != DROPEFFECT_NONE )
		DropSelection();
	return dropEffect;
}

template< typename BaseListCtrl >
void CDragListCtrl< BaseListCtrl >::Event_OnDragLeave( void )
{
	// don't call EndDragging() here, because auto-scroll drop targets call OnDragEnter() and OnDragLeave() repeatedly during the dragging operation
}


// message handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CDragListCtrl, BaseListCtrl, BaseListCtrl )
	ON_NOTIFY_REFLECT_EX( LVN_BEGINDRAG, OnLvnBeginDrag_Reflect )
	ON_MESSAGE( LVM_ENSUREVISIBLE, OnLVmEnsureVisible )
END_MESSAGE_MAP()

template< typename BaseListCtrl >
BOOL CDragListCtrl< BaseListCtrl >::OnLvnBeginDrag_Reflect( NMHDR* pNmHdr, LRESULT* pResult )
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

template< typename BaseListCtrl >
LRESULT CDragListCtrl< BaseListCtrl >::OnLVmEnsureVisible( WPARAM wParam, LPARAM lParam )
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


#endif // DragListCtrl_hxx
