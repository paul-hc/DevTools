#ifndef LayoutEngine_h
#define LayoutEngine_h
#pragma once

#include <hash_map>
#include "CtrlInterfaces.h"
#include "Dialog_fwd.h"
#include "LayoutMetrics.h"


class CLayoutEngine
{
public:
	enum Flags
	{
		SmoothGroups		= 1 << 0,			// draw groups smoothly on WM_ERASEBKGND, clipping other controls
		GroupsTransparent	= 1 << 1,			// receive WM_PAINT messages only after all sibling windows beneath it have been updated
		GroupsRepaint		= 1 << 2,			// prevents clipping issues in property pages

		UseDoubleBuffer		= 1 << 8,			// use double buffering when erasing background for smooth groups; some themed controls don't erase corner pixels as expected

		Smooth = SmoothGroups,
		Normal = GroupsTransparent | GroupsRepaint,
	};
public:
	CLayoutEngine( int flags = m_defaultFlags );
	~CLayoutEngine();

	int GetFlags( void ) const { return m_flags; }
	void SetFlags( int flags ) { m_flags = flags; }

	bool HasCtrlLayout( void ) const { return !m_controlStates.empty(); }
	void RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count );

	// some dialogs require different layouts for collapsed state (e.g. proportional expanded, full size collapsed)
	void RegisterDualCtrlLayout( const CDualLayoutStyle dualLayoutStyles[], unsigned int count );

	bool HasInitialSize( void ) const { return m_minClientSize.cx != 0 && m_minClientSize.cy != 0; }
	void StoreInitialSize( CWnd* pDialog );		// called before Initialize() by form views, etc

	bool IsInitialized( void ) const { return m_pDialog->GetSafeHwnd() != NULL; }
	void Initialize( CWnd* pDialog );
	void Reset( void );						// un-initialize: for form view recreation

	layout::CResizeGripper* GetResizeGripper( void ) const;
	void CreateResizeGripper( const CSize& offset = CSize( 1, 0 ) );

	bool IsLayoutEnabled( void ) const { return m_layoutEnabled; }
	void SetLayoutEnabled( bool layoutEnabled = true );

	bool IsCollapsible( void ) const { return m_collapsedDelta.cx != 0 || m_collapsedDelta.cy != 0; }
	bool IsCollapsed( void ) const { return m_collapsed; }
	bool SetCollapsed( bool collapsed );
	void ToggleCollapsed( void ) { SetCollapsed( !IsCollapsed() ); }

	bool LayoutControls( const CSize& clientSize );
	bool LayoutControls( void );

	void HandleGetMinMaxInfo( MINMAXINFO* pMinMaxInfo ) const;
	LRESULT HandleHitTest( LRESULT hitTest, const CPoint& screenPoint ) const;
	bool HandleEraseBkgnd( CDC* pDC );
	void HandlePostPaint( void );
public:
	CSize GetMinClientSize( bool collapsed ) const;
	CSize GetMinClientSize( void ) const { return GetMinClientSize( m_collapsed ); }
	CSize GetMaxClientSize( void ) const;

	CSize& MaxClientSize( void ) { ASSERT( !IsInitialized() ); return m_maxClientSize; }		// in default expanded state
	void DisableResizeVertically( void ) { MaxClientSize().cy = 0; }
	void DisableResizeHorizontally( void ) { MaxClientSize().cx = 0; }

	CPoint GetMinWindowSize( void ) const { return GetMinClientSize() + m_nonClientSize; }

	layout::Style FindLayoutStyle( UINT ctrlId ) const;
	layout::Style FindLayoutStyle( HWND hCtrl ) const { ASSERT( IsWindow( hCtrl ) ); return FindLayoutStyle( ::GetDlgCtrlID( hCtrl ) ); }

	ui::ILayoutFrame* FindControlLayoutFrame( HWND hCtrl ) const;
	void RegisterBuddyCallback( UINT buddyId, ui::ILayoutFrame* pCallback );

	// advanced control layout: use with care
	layout::CControlState* LookupControlState( UINT ctrlId );
	bool HasControlState( UINT ctrlId ) const { return m_controlStates.find( ctrlId ) != m_controlStates.end(); }
	bool RefreshControlHandle( UINT ctrlId );			// call after control with same ID gets recreated
	void AdjustControlInitialPosition( UINT ctrlId, const CSize& deltaOrigin, const CSize& deltaSize );		// when stretching content to fit: to retain original layout behaviour
private:
	void SetupControlStates( void );
	void SetupCollapsedState( UINT ctrlId, layout::Style style );
	void SetupGroupBoxState( HWND hGroupBox, layout::CControlState* pControlState );
	bool AnyRepaintCtrl( void ) const;
	bool LayoutSmoothly( const CSize& delta );
	bool LayoutNormal( const CSize& delta );
	void DrawBackground( CDC* pDC, const CRect& clientRect );
	bool CanClip( HWND hCtrl ) const;
private:
	enum InternalFlags { Erasing = 1 << 15 };

	int m_flags;
	bool m_layoutEnabled;
	stdext::hash_map< UINT, layout::CControlState > m_controlStates;
	stdext::hash_map< UINT, ui::ILayoutFrame* > m_buddyCallbacks;		// called back when buddy windows are moved or resized

	CWnd* m_pDialog;
	CSize m_minClientSize;
	CSize m_maxClientSize;									// cx/cy: 0 for no resize, otherwise max size
	CSize m_nonClientSize;
	CSize m_previousSize;

	CSize m_collapsedDelta;
	bool m_collapsed;

	std::auto_ptr<layout::CResizeGripper> m_pGripper;		// bottom-right resize box
	std::vector< HWND > m_hiddenGroups;						// hidden groups, drawn smoothly on WM_ERASEBKGND
public:
	static int m_defaultFlags;								// for debugging, testing
};


#endif // LayoutEngine_h
