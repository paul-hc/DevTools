#ifndef OleDropTargetEx_h
#define OleDropTargetEx_h
#pragma once

#include <afxole.h>
#include "ShellDropTip.h"


namespace auto_scroll
{
	enum AutoScrollFlags
	{
		// auto-scroll mode flags
		HorizBar	= BIT_FLAG( 0 ),			// when over horizontal scroll bar
		VertBar		= BIT_FLAG( 1 ),			// when over vertical scroll bar
		InsetHorz	= BIT_FLAG( 2 ),			// when over horizontal inset region
		InsetVert	= BIT_FLAG( 3 ),			// when over vertical inset region
		InsetBar	= BIT_FLAG( 4 ),			// when over inset region and bar visible
		UseDefault	= BIT_FLAG( 16 ),			// use default handling (no OnDragScroll handler required)

			Bars = HorizBar | VertBar,
			Insets = InsetHorz | InsetVert,
			Mask = 0x0FF
	};
}


namespace ole
{
	interface IDropTargetEventsStub;


	// drop target supporting drag images and drop descriptions
	//
	class CDropTarget : public COleDropTarget
	{
	public:
		CDropTarget( void );
		virtual ~CDropTarget();

		BOOL Register( CWnd* pWnd );

		// base overrides
		virtual DROPEFFECT OnDragEnter( CWnd* pWnd, COleDataObject* pDataObject, DWORD keyState, CPoint point );
		virtual DROPEFFECT OnDragOver( CWnd* pWnd, COleDataObject* pDataObject, DWORD keyState, CPoint point );
		virtual DROPEFFECT OnDropEx( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point );
		virtual BOOL OnDrop( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point );
		virtual void OnDragLeave( CWnd* pWnd );
		virtual DROPEFFECT OnDragScroll( CWnd* pWnd, DWORD keyState, CPoint point );
	public:
		void SetScrollMode( int autoScrollFlags ) { m_autoScrollFlags = autoScrollFlags; }

		// target specific drop-tips
		bool SetDropTipText( DROPIMAGETYPE dropImageType, const wchar_t* pMessage, const wchar_t* pInsertFmt, const wchar_t* pInsertText = nullptr );
		bool SetDropTipInsertText( const wchar_t* pInsertText );
		bool SetDropTip( DROPIMAGETYPE imageType, const wchar_t* pText, bool create );
		bool ClearDropTip( void ) { return SetDropTip( DROPIMAGE_INVALID, nullptr, false ); }
	private:
		bool SetDropTip( DROPEFFECT effect );
	public:
		bool HasCachedDropTipText( void ) const { return m_cachedDropTipText; }

		DROPEFFECT GetDropEffect( DWORD keyState, DROPEFFECT defaultEffect = DROPEFFECT_MOVE ) const;
		DROPEFFECT FilterDropEffect( DROPEFFECT dwEffect ) const;
	private:
		void ClearStateFlags( void );
		void HandlePostDrop( COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point );
		DROPEFFECT DefaultAutoScroll( CWnd* pWnd, DWORD keyState, CPoint point );
		void SetScrollPos( CWnd* pWnd, int step, int barId ) const;
	private:
		int m_autoScrollFlags;								// auto scroll handling
		CComPtr<IDropTargetHelper> m_pDropTargetHelper;		// drag image helper (different interface than in shell::CDragImager)
		shell::CDropTip m_dropTip;							// user defined default drop description text
		ole::IDropTargetEventsStub* m_pTargetEvents;

		bool m_canShowDropTips;					// set when drop descriptions can be used (Vista or later)
		bool m_useDropTip;						// true: drop descriptions are updated by this class

		bool m_dropTipUpdated;					// internal flag to detect if drop description has been updated
		bool m_entered;							// set when entered the target

		// cached data from source
		bool m_cachedDragImage;					// cached "DragWindow" data object
		bool m_cachedDropTipText;				// cached "DragSourceHelperFlags" data object: drop description text is shown by the drag source
		DROPEFFECT m_cachedPreferredEffect;		// cached "Preferred DropEffect" data object

		DROPEFFECT m_dropEffects;				// stored drop effects passed to DoDragDrop
	public:
		BEGIN_INTERFACE_PART( DropTargetEx, IDropTarget )
			INIT_INTERFACE_PART( CDropTarget, DropTargetEx )
			STDMETHOD( DragEnter )( IDataObject*, DWORD, POINTL, DWORD* );
			STDMETHOD( DragOver )( DWORD, POINTL, DWORD* );
			STDMETHOD( DragLeave )( void );
			STDMETHOD( Drop )( IDataObject*, DWORD, POINTL, DWORD* );
		END_INTERFACE_PART( DropTargetEx )

		DECLARE_INTERFACE_MAP()
	};
}


#endif // OleDropTargetEx_h
