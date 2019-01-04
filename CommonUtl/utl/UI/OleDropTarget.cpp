
#include "stdafx.h"
#include "OleDropTarget.h"
#include "OleDragDrop_fwd.h"
#include "OleUtils.h"


namespace scroll
{
	inline bool IsScrollBarVisible( const SCROLLBARINFO& sbi ) { return STATE_SYSTEM_INVISIBLE != sbi.rgstate[ 0 ]; }
	inline bool IsScrollBarInvisible( const SCROLLBARINFO& sbi ) { return STATE_SYSTEM_INVISIBLE == sbi.rgstate[ 0 ]; }
	inline bool IsScrollBarActive( const SCROLLBARINFO& sbi ) { return 0 == sbi.rgstate[ 0 ]; }
	inline bool CanScrollUpRight( const SCROLLBARINFO& sbi ) { return 0 == sbi.rgstate[ 2 ]; }
	inline bool CanScrollDownLeft( const SCROLLBARINFO& sbi ) { return 0 == sbi.rgstate[ 4 ]; }

	// scrolling over the inset region
	inline bool CanScrollInsetUpRight( const SCROLLBARINFO& sbi, int autoScrollFlags )
	{
		return HasFlag( autoScrollFlags, auto_scroll::InsetBar )
			? ( IsScrollBarActive( sbi ) && CanScrollUpRight( sbi ) )
			: ( IsScrollBarInvisible( sbi ) || CanScrollUpRight( sbi ) );
	}

	inline bool CanScrollInsetDownLeft( const SCROLLBARINFO& sbi, int autoScrollFlags )
	{
		return HasFlag( autoScrollFlags, auto_scroll::InsetBar )
			? ( IsScrollBarActive( sbi ) && CanScrollDownLeft( sbi ) )
			: ( IsScrollBarInvisible( sbi ) || CanScrollDownLeft( sbi ) );
	}
}


/*
	To show drag images when dragging content over your app:
		Add a member of this class to the main frame class and call Register() from OnCreate().
		With dialog applications, do this for your main dialog and call Register() from OnInitDialog().

	To support drop for CView derived windows:
	- Add a member of this class to the view class.
	- Register the drop target from within OnInitialUpdate().
	- Override the virtual handler functions in the view class.

	To support drop for other windows: implement ole::IDropTargetEventsStub interface ovverides

	Using auto scrolling
		This class supports detection of auto scroll events when over scrollbars or on an inset region.
		Which events should be handled is specified by flags. The window class must only
		provide a callback function or accepting a user defined message to perform scrolling.
*/


namespace ole
{
	CDropTarget::CDropTarget( void )
		: COleDropTarget()
		, m_useDropTip( false )
		, m_autoScrollFlags( 0 )
		, m_pTargetEvents( NULL )
	{
		m_pDropTargetHelper.CoCreateInstance( CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER );		// IDropTargetHelper, different than IDragSourceHelper in shell::CDragImager
		m_canShowDropTips = m_pDropTargetHelper != NULL && ::IsAppThemed() != 0;		// Vista+

		ClearStateFlags();
	}

	CDropTarget::~CDropTarget()
	{
	}

	void CDropTarget::ClearStateFlags( void )
	{
		m_dropTipUpdated = false;
		m_entered = false;
		m_cachedDragImage = false;
		m_cachedDropTipText = false;
		m_cachedPreferredEffect = DROPEFFECT_NONE;
	}

	BOOL CDropTarget::Register( CWnd* pWnd )
	{
		m_pTargetEvents = dynamic_cast< ole::IDropTargetEventsStub* >( pWnd );
		return COleDropTarget::Register( pWnd );
	}

	bool CDropTarget::SetDropTipText( DROPIMAGETYPE dropImageType, const wchar_t* pMessage, const wchar_t* pInsertFmt, const wchar_t* pInsertText /*= NULL*/ )
	{
		// if any of the text args is NULL, the default text is used (Explorer)
		bool changed = false;
		if ( m_canShowDropTips )
		{
			changed = m_dropTip.StoreTypeField( dropImageType, pMessage, pInsertFmt );
			if ( changed && pInsertText != NULL )
				m_dropTip.SetInsert( pInsertText );
			m_useDropTip |= changed;					// drop description text is available
		}
		return changed;
	}

	bool CDropTarget::SetDropTipInsertText( const wchar_t* pInsertText )
	{
		if ( !m_canShowDropTips )
			return false;

		m_dropTip.SetInsert( pInsertText );
		return m_dropTip.HasInsert();
	}

	bool CDropTarget::SetDropTip( DROPIMAGETYPE imageType, const wchar_t* pText, bool create )
	{
		// if DROPIMAGE_INVALID == imageType the drop-tip texts are cleared
		ASSERT( m_lpDataObject );

		bool hasDescription = false;

		// source provides a drag image, Vista or later with enabled themes, and actually dragging (m_lpDataObject is NULL when not dragging)
		if ( m_cachedDragImage && m_canShowDropTips && m_lpDataObject != NULL )
		{
			STGMEDIUM stgMedium;
			FORMATETC formatEtc;
			if ( ole_utl::ExtractData( &formatEtc, &stgMedium, m_lpDataObject, CFSTR_DROPDESCRIPTION ) )
			{
				hasDescription = true;
				bool update = false;
				DROPDESCRIPTION* pDropTip = (DROPDESCRIPTION*)::GlobalLock( stgMedium.hGlobal );

				if ( DROPIMAGE_INVALID == imageType )
					update = shell::CDropTip::Clear( pDropTip );
				else if ( m_cachedDropTipText )			// no need to update the description text when the source hasn't enabled text
				{
					// if no text has been passed, use the stored text for the image type if present
					if ( NULL == pText )
						pText = m_dropTip.GetTypeField( imageType );
					update = m_dropTip.MakeDescription( pDropTip, pText );
				}

				if ( pDropTip->type != imageType )
				{
					update = true;
					pDropTip->type = imageType;
				}
				::GlobalUnlock( stgMedium.hGlobal );
				if ( update )
					update = HR_OK( m_lpDataObject->SetData( &formatEtc, &stgMedium, TRUE ) );
				if ( !update )							// nothing changed or setting data failed
					::ReleaseStgMedium( &stgMedium );
			}

			// DropTip data object does not exist yet.
			// So create it when create is true and image type is not invalid.
			// When m_cachedDropTipText is true, we can be sure that the source supports drop descriptions.
			// Otherwise, there is no need to create the object when the image type corresponds to the drop effects.
			// This avoids creating the description object when the source does not handle them resulting in old and new style cursors shown together.
			// However, when using special image types (> DROPIMAGE_LINK), the object is always created which may result in showing old and new style cursors when the
			// source does not support drop descriptions.
			if ( !hasDescription && create &&
				 imageType != DROPIMAGE_INVALID &&
				 ( m_cachedDropTipText || imageType > DROPIMAGE_LINK ) )
			{
				stgMedium.tymed = TYMED_HGLOBAL;
				stgMedium.hGlobal = ::GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof( DROPDESCRIPTION ) );
				stgMedium.pUnkForRelease = NULL;
				if ( stgMedium.hGlobal != NULL )
				{
					DROPDESCRIPTION* pDropTip = (DROPDESCRIPTION*)::GlobalLock( stgMedium.hGlobal );
					pDropTip->type = imageType;
					if ( m_cachedDropTipText )
					{
						if ( NULL == pText )
							pText = m_dropTip.GetTypeField( imageType );		// use the stored text for the image type
						m_dropTip.MakeDescription( pDropTip, pText );
					}
					::GlobalUnlock( stgMedium.hGlobal );
					hasDescription = HR_OK( m_lpDataObject->SetData( &formatEtc, &stgMedium, TRUE ) );
					if ( !hasDescription )
						::GlobalFree( stgMedium.hGlobal );
				}
			}
		}
		m_dropTipUpdated = true;						// don't call this again from OnDragEnter() or OnDragOver()
		return hasDescription;
	}

	bool CDropTarget::SetDropTip( DROPEFFECT effect )
	{
		bool hasDescription = false;
		if ( m_cachedDropTipText )						// drag image present, Vista or later, visual styles enabled, and text enabled
		{
			// filtering should have been already done by the window class
			DROPIMAGETYPE type = static_cast< DROPIMAGETYPE >( FilterDropEffect( effect & ~DROPEFFECT_SCROLL ) );
			if ( const wchar_t* pText = m_dropTip.GetTypeField( type ) )		// text is present (even if empty)
				hasDescription = SetDropTip( type, pText, true );
			else
				hasDescription = ClearDropTip();		// show default cursor and text
		}
		m_dropTipUpdated = true;						// don't call this again from OnDragEnter() or OnDragOver()
		return hasDescription;
	}


	DROPEFFECT CDropTarget::OnDragEnter( CWnd* pWnd, COleDataObject* pDataObject, DWORD keyState, CPoint point )
	{
		ClearStateFlags();

		DROPEFFECT effect = DROPEFFECT_NONE;

		m_cachedDragImage = ole_utl::GetValueDWord( m_lpDataObject, _T("DragWindow") ) != NULL;				// source provides a drag image?

		// drag source has display of drop tips text enabled? If disabled, no need to change the description text here
		if ( m_cachedDragImage && m_canShowDropTips )
			m_cachedDropTipText = HasFlag( ole_utl::GetValueDWord( m_lpDataObject, _T("DragSourceHelperFlags") ), DSH_ALLOWDROPDESCRIPTIONTEXT );

		m_cachedPreferredEffect = ole_utl::GetValueDWord( m_lpDataObject, CFSTR_PREFERREDDROPEFFECT );		// cache preferred drop effect
		if ( m_pTargetEvents != NULL )
			effect = m_pTargetEvents->Event_OnDragEnter( pDataObject, keyState, point );
		else
			effect = COleDropTarget::OnDragEnter( pWnd, pDataObject, keyState, point );						// default handling (CView support)

		effect = FilterDropEffect( effect );			// we need the correct effect here for the drop tip
		if ( m_useDropTip && !m_dropTipUpdated )		// set drop-tip text if available and not been already set by the control
			SetDropTip( effect );
		if ( m_pDropTargetHelper != NULL )
			m_pDropTargetHelper->DragEnter( pWnd->GetSafeHwnd(), m_lpDataObject, &point, effect );			// show drag image

		m_entered = true;								// entered the target window - used by OnDragScroll()
		return effect;
	}

	void CDropTarget::OnDragLeave( CWnd* pWnd )
	{
		// it may be necessary to perform a similar clean up at the end of OnDrop/OnDropEx
		if ( m_pTargetEvents != NULL )
			m_pTargetEvents->Event_OnDragLeave();
		else
			COleDropTarget::OnDragLeave( pWnd );

		// We should always set the image type of existing drop descriptions to DROPIMAGE_INVALID upon leaving even when the description hasn't been changed by this class.
		// This helps the drop source to detect changes of the description.
		ClearDropTip();

		if ( m_pDropTargetHelper != NULL )
			m_pDropTargetHelper->DragLeave();

		ClearStateFlags();								// next drag event may be from another source
	}

	DROPEFFECT CDropTarget::OnDragOver( CWnd* pWnd, COleDataObject* pDataObject, DWORD keyState, CPoint point )
	{
		// the main purpose of this is to change the cursor according to the key state
		DROPEFFECT effect = DROPEFFECT_NONE;			// default return value if not handled
		m_dropTipUpdated = false;						// set when description updated by target window
		if ( m_pTargetEvents != NULL )
			effect = m_pTargetEvents->Event_OnDragOver( pDataObject, keyState, point );
		else
			effect = COleDropTarget::OnDragOver( pWnd, pDataObject, keyState, point );		// default handling (CView support)

		effect = FilterDropEffect( effect );
		if ( m_useDropTip && !m_dropTipUpdated )
			SetDropTip( effect );
		if ( m_pDropTargetHelper != NULL )
			m_pDropTargetHelper->DragOver( &point, effect );								// show drag image
		return effect;
	}

	DROPEFFECT CDropTarget::OnDropEx( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point )
	{
		DROPEFFECT effect = DROPEFFECT_NOT_IMPL;
		if ( m_pTargetEvents != NULL )
			effect = m_pTargetEvents->Event_OnDropEx( pDataObject, dropDefault, dropList, point );
		else
			effect = COleDropTarget::OnDropEx( pWnd, pDataObject, dropDefault, dropList, point );		// default handling

		if ( effect != DROPEFFECT_NOT_IMPL )
			HandlePostDrop( pDataObject, effect, point );

		// when effect == -1 and dropDefault != DROPEFFECT_NONE, OnDrop() is called now
		// when effect == -1 and dropDefault == DROPEFFECT_NONE, OnDragLeave() is called now
		return effect;
	}

	BOOL CDropTarget::OnDrop( CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point )
	{
		BOOL dropped = FALSE;
		if ( m_pTargetEvents != NULL )
			dropped = m_pTargetEvents->Event_OnDrop( pDataObject, dropEffect, point );
		else
			dropped = COleDropTarget::OnDrop( pWnd, pDataObject, dropEffect, point );

		HandlePostDrop( pDataObject, dropped ? dropEffect : DROPEFFECT_NONE, point );
		return dropped;
	}

	void CDropTarget::HandlePostDrop( COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point )
	{
		static const CLIPFORMAT cfPerformedDropEffect = ole_utl::RegisterFormat( CFSTR_PERFORMEDDROPEFFECT );

		// CFSTR_PERFORMEDDROPEFFECT should be set by the window class
		if ( m_cachedPreferredEffect != DROPEFFECT_NONE && !pDataObject->IsDataAvailable( cfPerformedDropEffect ) )
			ole_utl::SetValueDWord( m_lpDataObject, dropEffect, CFSTR_PERFORMEDDROPEFFECT );		// set the performed drop effect if not done by the control

		if ( m_pDropTargetHelper != NULL )				// release drag image
			m_pDropTargetHelper->Drop( m_lpDataObject, &point, dropEffect );
		ClearStateFlags();
	}

	/*
		implemented functions should return:
		- DROPEFFECT_NONE: when not scrolling or not handling the event
		- DROPEFFECT_SCROLL: when scrolling should be handled here according to the m_autoScrollFlags flags
		- DROPEFFECT_SCROLL | DROPEFFECT_COPY, DROPEFFECT_MOVE, DROPEFFECT_LINK: when scrolling is active
	*/
	DROPEFFECT CDropTarget::OnDragScroll( CWnd* pWnd, DWORD keyState, CPoint point )
	{
		DROPEFFECT effect = DROPEFFECT_NONE;

		if ( m_autoScrollFlags & auto_scroll::UseDefault )
		{
			if ( m_autoScrollFlags & auto_scroll::Mask )
				effect = DefaultAutoScroll( pWnd, keyState, point );					// use default scrolling (no OnDragScroll handler necessary)
		}
		else
		{
			if ( m_pTargetEvents != NULL )
				effect = m_pTargetEvents->Event_OnDragScroll( keyState, point );

			// PHC: fix for InfoList as drop target! - if COleDropTarget::OnDragScroll() doesn't get called InfoList does NOT behave as a valid drop target
			if ( HasFlag( effect, DROPEFFECT_SCROLL ) || DROPEFFECT_NONE == effect )
				effect = COleDropTarget::OnDragScroll( pWnd, keyState, point );			// default handling (CView support)

			if ( DROPEFFECT_SCROLL == effect && HasFlag( m_autoScrollFlags, auto_scroll::Mask ) )
				effect = DefaultAutoScroll( pWnd, keyState, point );					// call the default handler when DROPEFFECT_SCROLL is returned
		}

		if ( HasFlag( effect, DROPEFFECT_SCROLL ) && m_pDropTargetHelper != NULL )
		{
			// adjust drop effect to show correct drop description cursor (ole::CDropSource class handles these correctly)
			effect = FilterDropEffect( effect & ~DROPEFFECT_SCROLL );

			// the drag image must be updated here
			if ( m_entered )
				m_pDropTargetHelper->DragOver( &point, effect );
			else
				m_pDropTargetHelper->DragEnter( pWnd->GetSafeHwnd(), m_lpDataObject, &point, effect );

			effect |= DROPEFFECT_SCROLL;
		}
		return effect;
	}

	DROPEFFECT CDropTarget::DefaultAutoScroll( CWnd* pWnd, DWORD keyState, CPoint point )
	{
		// second part of the code is based on the MFC COleDropTarget::OnDragScroll code for CView windows.
		ASSERT_PTR( pWnd );

		int onScrollBar = 0;
		ole::ScrollDir vertDir = ole::NoScroll, horizDir = ole::NoScroll;

		SCROLLBARINFO sbHoriz = { sizeof( SCROLLBARINFO ) }, sbVert = { sizeof( SCROLLBARINFO ) };

		// also used with inset regions to check if scrolling in specific directions is possible
		if ( !pWnd->GetScrollBarInfo( OBJID_VSCROLL, &sbVert ) )
			sbVert.rgstate[ 0 ] = STATE_SYSTEM_INVISIBLE;
		if ( !pWnd->GetScrollBarInfo( OBJID_HSCROLL, &sbHoriz ) )
			sbHoriz.rgstate[ 0 ] = STATE_SYSTEM_INVISIBLE;

		// autoscroll on an arrow or not over the thumb
		CPoint ptScreen( point );
		pWnd->ClientToScreen( &ptScreen );
		if ( scroll::IsScrollBarVisible( sbVert ) && ::PtInRect( &sbVert.rcScrollBar, ptScreen ) )					// over a visible scroll bar?
		{
			onScrollBar = auto_scroll::VertBar;
			if ( HasFlag( m_autoScrollFlags, auto_scroll::VertBar ) && scroll::IsScrollBarActive( sbVert ) )		// scroll bar enabled (not grayed out)
			{
				int pos = ptScreen.y - sbVert.rcScrollBar.top;
				if ( scroll::CanScrollUpRight( sbVert ) && pos < sbVert.xyThumbTop )
					vertDir = ole::ScrollPrev;
				else if ( scroll::CanScrollDownLeft( sbVert ) && pos >= sbVert.xyThumbBottom )
					vertDir = ole::ScrollNext;
			}
		}
		else if ( scroll::IsScrollBarVisible( sbHoriz ) && ::PtInRect( &sbHoriz.rcScrollBar, ptScreen ) )
		{
			onScrollBar = auto_scroll::HorizBar;
			if ( HasFlag( m_autoScrollFlags, auto_scroll::HorizBar ) && scroll::IsScrollBarActive( sbHoriz ) )
			{
				int pos = ptScreen.x - sbHoriz.rcScrollBar.left;
				if ( scroll::CanScrollDownLeft( sbHoriz ) && pos < sbHoriz.xyThumbTop )
					horizDir = ole::ScrollPrev;
				else if ( scroll::CanScrollUpRight( sbHoriz ) && pos >= sbHoriz.xyThumbBottom )
					horizDir = ole::ScrollNext;
			}
		}
		else if ( m_autoScrollFlags & auto_scroll::Insets )															// inside the inset region?
		{
			CRect clientRect;
			pWnd->GetClientRect( &clientRect );
			CRect nonInsetRect = clientRect;

			nonInsetRect.InflateRect( -COleDropTarget::nScrollInset, -COleDropTarget::nScrollInset );		// default inset hot zone width is 11 pixels
			if ( clientRect.PtInRect( point ) && !nonInsetRect.PtInRect( point ) )
			{
				// Check if scrolling can be performed according to scroll bar info:
				//  auto_scroll::InsetBar: true if scroll bar visible and enabled, and can scroll into direction
				//  otherwise: True if scroll bar invisible or can scroll into direction
				if ( HasFlag( m_autoScrollFlags, auto_scroll::InsetHorz ) )
				{
					if ( point.x < nonInsetRect.left )
					{
						if ( scroll::CanScrollInsetDownLeft( sbHoriz, m_autoScrollFlags ) )
							horizDir = ole::ScrollPrev;
					}
					else if ( point.x >= nonInsetRect.right )
					{
						if ( scroll::CanScrollInsetUpRight( sbHoriz, m_autoScrollFlags ) )
							horizDir = ole::ScrollNext;
					}
				}
				if ( HasFlag( m_autoScrollFlags, auto_scroll::InsetVert ) )
				{
					if ( point.y < nonInsetRect.top )
					{
						if ( scroll::CanScrollInsetUpRight( sbVert, m_autoScrollFlags ) )
							vertDir = ole::ScrollPrev;
					}
					else if ( point.y >= nonInsetRect.bottom )
					{
						if ( scroll::CanScrollInsetDownLeft( sbVert, m_autoScrollFlags ) )
							vertDir = ole::ScrollNext;
					}
				}
			}
		}

		if ( ole::NoScroll == vertDir && ole::NoScroll == horizDir )			// no scrolling
		{
			if ( m_nTimerID != 0xffff )					// was scrolling, so stop it
			{
				COleDataObject dataObject;				// send fake OnDragEnter when changing from scroll to normal
				dataObject.Attach( m_lpDataObject, FALSE );
				OnDragEnter( pWnd, &dataObject, keyState, point );
				m_nTimerID = 0xffff;					// no more scrolling
			}
			return DROPEFFECT_NONE;
		}

		UINT timerId = MAKEWORD( horizDir, vertDir );		// store scroll state as WORD
		DWORD dwTick = ::GetTickCount();					// save tick count when timer ID changes
		if ( timerId != m_nTimerID )						// start scrolling or direction has changed
		{
			m_dwLastTick = dwTick;
			m_nScrollDelay = nScrollDelay;					// scroll start delay time (default is 50 ms)
		}
		if ( dwTick - m_dwLastTick > m_nScrollDelay )		// scroll if delay time expired
		{
			if ( m_pTargetEvents != NULL )
				m_pTargetEvents->Event_DoScroll( horizDir, vertDir );
			else
			{
				// try to access the scroll bar of the window
				if ( vertDir != ole::NoScroll && scroll::IsScrollBarVisible( sbVert ) )
					SetScrollPos( pWnd, vertDir, SB_VERT );
				if ( horizDir && scroll::IsScrollBarVisible( sbHoriz ) )
					SetScrollPos( pWnd, horizDir, SB_HORZ );
			}
			m_dwLastTick = dwTick;
			m_nScrollDelay = nScrollInterval;			// scrolling delay time (default is 50 ms)
		}
		if ( 0xffff == m_nTimerID && m_entered )		// scrolling started
			OnDragLeave( pWnd );						// send fake OnDragLeave when changing from normal to scroll

		m_nTimerID = timerId;							// save new scroll state

		return onScrollBar ? DROPEFFECT_SCROLL : GetDropEffect( keyState ) | DROPEFFECT_SCROLL;		// no drop over a scroll bar
	}

	void CDropTarget::SetScrollPos( CWnd* pWnd, int step, int barId ) const
	{
		ASSERT_PTR( pWnd );
		ASSERT( SB_HORZ == barId || SB_VERT == barId );

		if ( CScrollBar* pScrollCtrl = pWnd->GetScrollBarCtrl( barId ) )
			pScrollCtrl->SetScrollPos( pScrollCtrl->GetScrollPos() + step );
	}

	DROPEFFECT CDropTarget::GetDropEffect( DWORD keyState, DROPEFFECT defaultEffect /*= DROPEFFECT_MOVE*/ ) const
	{
		// get drop effect from key state and allowed effects

		if ( HasFlag( m_dropEffects, DROPEFFECT_LINK ) && EqFlag( keyState, MK_CONTROL | MK_SHIFT ) )		// linking allowed?
			return DROPEFFECT_LINK;			// force link

		if ( HasFlag( m_dropEffects, DROPEFFECT_MOVE ) && HasFlag( keyState, MK_SHIFT | MK_ALT ) )			// moving allowed?
			return DROPEFFECT_MOVE;			// force move

		// force copy, or force move when defaultEffect is copy
		if ( HasFlag( keyState, MK_CONTROL ) )
			return FilterDropEffect( DROPEFFECT_COPY == defaultEffect ? DROPEFFECT_MOVE : DROPEFFECT_COPY );

		return FilterDropEffect( DROPEFFECT_NONE == defaultEffect ? DROPEFFECT_MOVE : defaultEffect );
	}

	/*
		Adjust effect according to allowed effects.
		COleDropTarget neither passes nor stores the allowed effects.
		The effect is adjusted if necessary after all handlers has been called using _AfxFilterDropEffect().
		This may lead to wrong drop descriptions. So we have to store the allowed effects in this class to be used here.
	*/
	DROPEFFECT CDropTarget::FilterDropEffect( DROPEFFECT dropEffect ) const
	{
		DROPEFFECT scrollEffect = dropEffect & DROPEFFECT_SCROLL;
		dropEffect &= ~DROPEFFECT_SCROLL;
		if ( dropEffect )
		{
			dropEffect &= m_dropEffects;				// mask out allowed effects

			if ( dropEffect & m_cachedPreferredEffect )
				dropEffect &= m_cachedPreferredEffect;	// source may have set preferred drop effects

			if ( 0 == dropEffect )						// drop effect does not match the effects provided by the source
			{
				dropEffect = m_cachedPreferredEffect & m_dropEffects;
				if ( 0 == dropEffect )
					dropEffect = m_dropEffects;
			}

			// choose a drop effect (multiple or no matching effects)
			if ( HasFlag( dropEffect, DROPEFFECT_MOVE ) )
				dropEffect = DROPEFFECT_MOVE;
			else if ( HasFlag( dropEffect, DROPEFFECT_COPY ) )
				dropEffect = DROPEFFECT_COPY;
			else if ( HasFlag( dropEffect, DROPEFFECT_LINK ) )
				dropEffect = DROPEFFECT_LINK;
		}
		return dropEffect | scrollEffect;
	}


	// IDropTarget interface implementation

	BEGIN_INTERFACE_MAP( CDropTarget, COleDropTarget )
		INTERFACE_PART( CDropTarget, IID_IDropTarget, DropTargetEx )
	END_INTERFACE_MAP()

	STDMETHODIMP_(ULONG) CDropTarget::XDropTargetEx::AddRef( void )
	{
		METHOD_PROLOGUE( CDropTarget, DropTargetEx )
		return pThis->ExternalAddRef();
	}

	STDMETHODIMP_(ULONG) CDropTarget::XDropTargetEx::Release( void )
	{
		METHOD_PROLOGUE( CDropTarget, DropTargetEx )
		return pThis->ExternalRelease();
	}

	STDMETHODIMP CDropTarget::XDropTargetEx::QueryInterface( REFIID iid, void** ppvObj )
	{
		METHOD_PROLOGUE( CDropTarget, DropTargetEx )
		return (HRESULT)pThis->ExternalQueryInterface( &iid, ppvObj );
	}

	STDMETHODIMP CDropTarget::XDropTargetEx::DragEnter( THIS_ IDataObject* lpDataObject, DWORD keyState, POINTL pt, DWORD* pEffect )
	{
		METHOD_PROLOGUE( CDropTarget, DropTargetEx )
		pThis->m_dropEffects = *pEffect;			// store allowed effects (for drop tips)
		return pThis->m_xDropTarget.DragEnter(lpDataObject, keyState, pt, pEffect);
	}

	STDMETHODIMP CDropTarget::XDropTargetEx::DragOver( THIS_ DWORD keyState, POINTL pt, DWORD* pEffect )
	{
		METHOD_PROLOGUE( CDropTarget, DropTargetEx )
		pThis->m_dropEffects = *pEffect;			// store allowed effects
		return pThis->m_xDropTarget.DragOver( keyState, pt, pEffect );
	}

	STDMETHODIMP CDropTarget::XDropTargetEx::DragLeave( THIS )
	{
		METHOD_PROLOGUE( CDropTarget, DropTargetEx )
		return pThis->m_xDropTarget.DragLeave();
	}

	STDMETHODIMP CDropTarget::XDropTargetEx::Drop( THIS_ IDataObject* lpDataObject, DWORD keyState, POINTL pt, DWORD* pEffect )
	{
		METHOD_PROLOGUE( CDropTarget, DropTargetEx )
		return pThis->m_xDropTarget.Drop( lpDataObject, keyState, pt, pEffect );
	}

} //namespace ole
