#ifndef OleDragDrop_fwd_h
#define OleDragDrop_fwd_h
#pragma once

#include <afxole.h>


#define DROPEFFECT_NOT_IMPL ( (DROPEFFECT)-1 )


namespace ole
{
	HCURSOR GetDropEffectCursor( DROPEFFECT dropEffect );


	enum ScrollDir { ScrollPrev = -1, NoScroll, ScrollNext };


	// extends windows not derived from CView with drop target events;
	// a stub interface, with default implementation similar with CView's OLE drag & drop event methods.
	//
	interface IDropTargetEventsStub
	{
		virtual DROPEFFECT Event_OnDragEnter( COleDataObject* pDataObject, DWORD keyState, CPoint point );
		virtual DROPEFFECT Event_OnDragOver( COleDataObject* pDataObject, DWORD keyState, CPoint point );
		virtual bool Event_OnDrop( COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point );
		virtual DROPEFFECT Event_OnDropEx( COleDataObject* pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point );
		virtual DROPEFFECT Event_OnDragScroll( DWORD keyState, CPoint point );
		virtual void Event_OnDragLeave( void );

		virtual void Event_DoScroll( ole::ScrollDir horzDir, ole::ScrollDir vertDir );
	};


	// obscure, not needed: render data for drag&drop operations (when not using cached data)
	//
	interface IRenderDataWnd
	{
		virtual CWnd* GetSrcWnd( void ) = 0;
		virtual bool HandleRenderData( FORMATETC* pFormatEtc, STGMEDIUM* pStgMedium ) = 0;
	};


	// OBSOLETE: replace with new ole::CDropTarget
	// An OLE drop target with auto-scrolling.
	// Works for CView, or as an adapter for other windows implementing ole::IDropTargetEventsStub.
	//
	class C_DropTargetAdapter : public COleDropTarget
	{
	public:
		C_DropTargetAdapter( void );
		virtual ~C_DropTargetAdapter();

		// base overrides
		virtual DROPEFFECT OnDragEnter( CWnd* pWnd, COleDataObject* pDataObject, DWORD keyState, CPoint point );
		virtual DROPEFFECT OnDragOver( CWnd* pWnd, COleDataObject* pDataObject, DWORD keyState, CPoint point );
		virtual BOOL OnDrop( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point );
		virtual DROPEFFECT OnDropEx( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point );
		virtual void OnDragLeave( CWnd* pWnd );

		// customize auto-scrolling
		virtual DROPEFFECT OnDragScroll( CWnd* pWnd, DWORD keyState, CPoint point );
	protected:
		enum ScrollBar { HorizontalBar, VerticalBar };

		static UINT ScrollHitTest( const CPoint& point, const CRect& nonClientRect, ScrollBar scrollBar );
	};
}


#endif // OleDragDrop_fwd_h
