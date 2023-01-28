#ifndef DragListCtrl_h
#define DragListCtrl_h
#pragma once

#include "GdiPlus_fwd.h"
#include "OleDragDrop_fwd.h"
#include "ReportListControl.h"


namespace ole { class CDropTarget; class CDataSource; }


// A list control capable of OLE drag & drop with visual selected items feedback.
// It can initiate drag-drop operation internally, or let the owner window do it on LVN_BEGINDRAG notification.
//
template< typename BaseListCtrl >
class CDragListCtrl : public BaseListCtrl
					, public ole::IDropTargetEventsStub
{
public:
	CDragListCtrl( UINT columnLayoutId = 0, DWORD listStyleEx = lv::DefaultStyleEx );
	virtual ~CDragListCtrl();

	enum DraggingMode { NoDragging, InternalDragging, ParentDragging };

	DraggingMode GetDraggingMode( void ) const { return m_draggingMode; }
	void SetDraggingMode( DraggingMode draggingMode );

	ole::CDropTarget* GetDropTarget( void ) const { return safe_ptr( m_pDropTarget.get() ); }

	bool DragSelection( CPoint dragPos, ole::CDataSource* pDataSource = NULL, int sourceFlags = ListSourcesMask,
						DROPEFFECT dropEffect = DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK );
protected:
	// ole::IDropTargetEventsStub interface
	virtual DROPEFFECT Event_OnDragEnter( COleDataObject* pDataObject, DWORD keyState, CPoint point );
	virtual DROPEFFECT Event_OnDragOver( COleDataObject* pDataObject, DWORD keyState, CPoint point );
	virtual DROPEFFECT Event_OnDropEx( COleDataObject* pDataObject, DROPEFFECT dropEffect, DROPEFFECT dropList, CPoint point );
	virtual void Event_OnDragLeave( void );

	// base overrides
	virtual void SetupControl( void );
private:
	bool UseExternalDropFiles( void ) const { return this->GetAcceptDropFiles(); }		// drop external files from Explorer?

	bool IsDragging( void ) const { return m_pSrcDragging != NULL; }
	void EndDragging( void );
	bool DropSelection( void );							// for InternalDragging mode

	void HandleDragging( CPoint dragPos );
	void HighlightDropMark( int dropIndex );
	bool DrawDropMark( void );
	void RedrawItem( int index );
	bool IsValidDropIndex( void ) const;
private:
	DraggingMode m_draggingMode;
	std::auto_ptr<ole::CDropTarget> m_pDropTarget;	// list ctrl is registered as drop target to this data-member
	CScopedGdiPlusInit m_gdiPlus;						// used to render the drop marker

	struct CDragInfo;
	CDragInfo* m_pSrcDragging;							// created during a drag-drop operation when this is the drag source
	int m_dropIndex;									// current drop item index
private:
	struct CDragInfo
	{
		CDragInfo( int sourceFlags, const std::vector< int >& selIndexes ) : m_sourceFlags( sourceFlags ), m_selIndexes( selIndexes ) {}
	public:
		int m_sourceFlags;
		const std::vector< int >& m_selIndexes;
	};
protected:
	// generated stuff
	afx_msg BOOL OnLvnBeginDrag_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg LRESULT OnLVmEnsureVisible( WPARAM wParam, LPARAM lParam );

	DECLARE_MESSAGE_MAP()
};


struct CDropMark
{
	enum Placement { BeforeItem, AfterItem };
	enum Orientation { HorizMark, VertMark };

	CDropMark( const CListCtrl* pListCtrl, int dropIndex );

	void Draw( CDC* pDC );
	void DrawLine( CDC* pDC );				// old line with arrow caps
	void Invalidate( CListCtrl* pListCtrl );

	static Orientation GetOrientation( const CListCtrl* pListCtrl );
private:
	bool GetListItemRect( const CListCtrl* pListCtrl, int index, CRect* pRect ) const;
public:
	Placement m_placement;
	Orientation m_orientation;
	CRect m_markRect;

	enum Metrics { PenWidth = 3, ArrowExtent = 4, EdgePct = 5 };
};


#endif // DragListCtrl_h
