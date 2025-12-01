#ifndef LayoutEngine_h
#define LayoutEngine_h
#pragma once

#include <unordered_map>
#include "CtrlInterfaces.h"
#include "Dialog_fwd.h"
#include "LayoutMetrics.h"


class CLayoutEngine
{
public:
	enum LayoutType { DialogLayout, PaneLayout };

	enum Flags
	{
		Auto				= BIT_FLAG( 0 ),		// adapt to dialog type default (Dialog, FormView, PropertyPage).
		Normal				= BIT_FLAG( 1 ),		// draw normal via DeferWindowPos - parent dialog uses WS_CLIPCHILDREN, group-boxes use WS_EX_TRANSPARENT.
		SmoothGroups		= BIT_FLAG( 2 ),		// draw group-boxes smoothly on WM_ERASEBKGND, clipping other controls; parent dialog uses WS_CLIPCHILDREN style.
		GroupsVisible		= BIT_FLAG( 3 ),		// visible group-boxes using WS_EX_TRANSPARENT; should also use SmoothGroups style drawing; prevents groups clipping issues in CDialog splitters, but not required in property pages.
			GroupsMask		= 0x0F,					// first 8 bits are reserved for group flags

		UseDoubleBuffer		= BIT_FLAG( 8 ),		// use double buffering when erasing background for smooth groups; some themed controls don't erase corner pixels as expected

		Smooth = SmoothGroups,
		SmoothVisibleGroups = SmoothGroups | GroupsVisible,
	};
protected:
	enum InternalFlags
	{
		InLayout = BIT_FLAG( 15 ),
		Erasing = BIT_FLAG( 16 )
	};
public:
	CLayoutEngine( void );
	~CLayoutEngine();

	int GetFlags( void ) const { return m_flags; }
	void SetFlags( int flags );
	bool ModifyFlags( int clearFlags, int setFlags );

	bool HasCtrlLayout( void ) const { return !m_controlStates.empty(); }
	void RegisterCtrlLayout( const CLayoutStyle layoutStyles[], UINT count );

	// some dialogs require different layouts for collapsed state (e.g. proportional expanded, full size collapsed)
	void RegisterDualCtrlLayout( const CDualLayoutStyle dualLayoutStyles[], UINT count );

	bool HasInitialSize( void ) const { return m_dlgTemplClientRect.Width() != 0 && m_dlgTemplClientRect.Height() != 0; }
	void StoreInitialSize( CWnd* pDialog );		// called before Initialize() by dialogs, property pages, form views, etc

	bool IsInitialized( void ) const { return m_pDialog->GetSafeHwnd() != nullptr; }
	void Initialize( CWnd* pDialog );
	virtual void Reset( void );					// un-initialize: for form view recreation

	layout::CResizeGripper* GetResizeGripperBox( void ) const;
	void CreateResizeGripperBox( const CSize& offset = CSize( 1, 0 ) );

	bool IsLayoutEnabled( void ) const { return m_layoutEnabled; }
	void SetLayoutEnabled( bool layoutEnabled = true );

	bool IsCollapsible( void ) const { return m_collapsedDelta.cx != 0 || m_collapsedDelta.cy != 0; }
	bool IsCollapsed( void ) const { return m_collapsed; }
	bool SetCollapsed( bool collapsed );
	void ToggleCollapsed( void ) { SetCollapsed( !IsCollapsed() ); }

	bool LayoutControls( void );
public:
	// message handlers:
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

	layout::TStyle FindLayoutStyle( UINT ctrlId ) const;
	layout::TStyle FindLayoutStyle( HWND hCtrl ) const { ASSERT( IsWindow( hCtrl ) ); return FindLayoutStyle( ::GetDlgCtrlID( hCtrl ) ); }

	ui::ILayoutFrame* FindControlLayoutFrame( HWND hCtrl ) const;
	void RegisterBuddyCallback( UINT buddyId, ui::ILayoutFrame* pCallback );

	// advanced control layout: use with care
	layout::CControlState* LookupControlState( UINT ctrlId );
	layout::CControlState* LookupControlState( HWND hCtrl );
	bool HasControlState( UINT ctrlId ) const { return m_controlStates.find( ctrlId ) != m_controlStates.end(); }
	bool RefreshControlHandle( UINT ctrlId );			// call after control with same ID gets recreated
	void AdjustControlInitialPosition( UINT ctrlId, const CSize& deltaOrigin, const CSize& deltaSize );		// when stretching content to fit: to retain original layout behaviour
protected:
	void SetupControlStates( void );

	// overridables
	virtual void GetClientRectangle( OUT CRect* pClientRect ) const;
private:
	void SetupCollapsedState( HWND hCtrl, layout::TStyle layoutStyle );
	void SetupGroupBoxState( HWND hGroupBox );
	size_t MakeGroupBoxesTransparent( void );

	bool LayoutControls( const CRect& clientRect );
	bool LayoutSmoothly( const layout::CDelta& delta );
	bool LayoutNormal( const layout::CDelta& delta );
	bool _LayoutNormal_Old( const layout::CDelta& delta );

	void DrawBackground( CDC* pDC, const CRect& clientRect );
	bool CanClip( HWND hCtrl ) const;

	void _DbgBreak( const char* pDlgClassName ) const;
protected:
	LayoutType m_layoutType;
	int m_flags;
	bool m_layoutEnabled;
	std::unordered_map<UINT, layout::CControlState> m_controlStates;
	std::unordered_map<UINT, ui::ILayoutFrame*> m_buddyCallbacks;		// called back when buddy windows are moved or resized

	CWnd* m_pDialog;
	CRect m_dlgTemplClientRect;			// original client rect of the dialog template - its size is the minimum size
	CSize m_maxClientSize;				// cx/cy: 0 for no resize, otherwise max size
	CSize m_nonClientSize;

	CRect m_prevClientRect;				// stored for delta offset and size evaluation

	CSize m_collapsedDelta;
	bool m_collapsed;

	std::auto_ptr<layout::CResizeGripper> m_pGripperBox;	// bottom-right resize box
	std::vector<HWND> m_hiddenGroups;	// hidden groups, drawn smoothly on WM_ERASEBKGND
};


// Nested layout engine, use by panes implementing ui::ILayoutFrame interface
//
class CPaneLayoutEngine : public CLayoutEngine
{
public:
	CPaneLayoutEngine( void );
	~CPaneLayoutEngine();

	void InitializePane( ui::ILayoutFrame* pPaneLayoutFrame );

	bool ShowPaneControls( bool show = true );

	// base overrides
	virtual void Reset( void ) override;
protected:
	virtual void GetClientRectangle( OUT CRect* pClientRect ) const override;
private:
	using CLayoutEngine::Initialize;	// hidden base method

	void StoreInitialPaneSize( void );
private:
	ui::ILayoutFrame* m_pLayoutFrame;	// not null for child layout engines
	CLayoutEngine* m_pMasterLayout;		// master layout engine for the dialog
};


#endif // LayoutEngine_h
