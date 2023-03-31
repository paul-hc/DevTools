
#include "pch.h"
#include "OleDropSource.h"
#include "ShellDropTip.h"
#include "OleUtils.h"
#include "WndUtils.h"


#define DDWM_UPDATEWINDOW	( WM_USER + 3 )
#define DDWM_SETCURSOR		( WM_USER + 2 )		// undocumented message: if no drop description data exists, WPARAM specifies the cursor type and the system default text is shown


namespace ole
{
	CDropSource::CDropSource( void )
		: COleDropSource()
		, m_setCursor( true )
		, m_dragResult( 0 )
		, m_pDataObject( nullptr )
		, m_pDropTip( nullptr )
	{
	}

	void CDropSource::SetDataSource( COleDataSource* pDataSource )
	{
		if ( pDataSource != nullptr )
			m_pDataObject = static_cast<IDataObject*>( pDataSource->GetInterface( &IID_IDataObject ) );
		else
			m_pDataObject = nullptr;
	}

	bool CDropSource::SetDragImageCursor( DROPEFFECT dwEffect )
	{
		// set the drag image cursor and use the associated default text if no drop description present or it has the invalid image type
		// note: when a drop description object is present and has valid drop image type (not DROPIMAGE_INVALID), it's used regardless of the WPARAM value.

		HWND hWnd = (HWND)ULongToHandle( ole_utl::GetValueDWord( m_pDataObject, _T("DragWindow") ) );		// always a DWORD (also in 64-bit apps)
		if ( nullptr == hWnd )
			return false;

		enum CursorType { FromDropTip, NoDrop, MoveDrop, CopyDrop, LinkDrop };
		CursorType cursorType = FromDropTip;									// use DropTip to get type and optional text
		switch ( dwEffect & ~DROPEFFECT_SCROLL )
		{
			case DROPEFFECT_NONE: cursorType = NoDrop; break;
			case DROPEFFECT_COPY: cursorType = CopyDrop; break;		// the order is not as for DROPEEFECT values!
			case DROPEFFECT_MOVE: cursorType = MoveDrop; break;
			case DROPEFFECT_LINK: cursorType = LinkDrop; break;
		}
		::SendMessage( hWnd, DDWM_SETCURSOR, (WPARAM)cursorType, 0 );
		return true;
	}

	BOOL CDropSource::OnBeginDrag( CWnd* pWnd )
	{
		BOOL started = COleDropSource::OnBeginDrag( pWnd );		// this sets COleDropSource::m_bDragStarted

		// store the result flags
		if ( m_bDragStarted )
			m_dragResult = ole::DragStarted;				// drag has been started by leaving the start rect or after delay time has expired
		else if ( !ui::IsKeyPressed( HasFlag( m_dwButtonDrop, MK_LBUTTON ) ? VK_LBUTTON : VK_RBUTTON ) )
			m_dragResult = ole::DragLButtonReleased;		// other mouse button pressed
		else
			m_dragResult = ole::DragCancelled;				// ESC pressed or other mouse button activated

		return started;
	}

	SCODE CDropSource::QueryContinueDrag( BOOL escapePressed, DWORD keyState )
	{
		SCODE result = COleDropSource::QueryContinueDrag( escapePressed, keyState );

		if ( !HasFlag( keyState, m_dwButtonDrop ) )			// mouse button has been released
			m_dragResult |= ole::DragLButtonReleased;

		if ( DRAGDROP_S_CANCEL == result )					// ESC pressed, other mouse button activated
			m_dragResult |= ole::DragCancelled;				// mouse button released when not started

		return result;
	}

	SCODE CDropSource::GiveFeedback( DROPEFFECT dropEffect )
	{
		// when using drop descriptions, we must update the drag image window here to show the new style cursors and the optional description text

		SCODE result = COleDropSource::GiveFeedback( dropEffect );

		if ( m_bDragStarted && m_pDataObject != nullptr )
		{
			bool oldStyle = FALSE == ole_utl::GetValueDWord( m_pDataObject, _T("IsShowingLayered") );		// "IsShowingLayered": true when the target window shows the drag image (through IDropTargetHelper)

			if ( oldStyle ? ( !m_setCursor ) : ( m_pDropTip != nullptr ) )
			{
				FORMATETC formatEtc;
				STGMEDIUM stgMedium;
				if ( ole_utl::ExtractData( &formatEtc, &stgMedium, m_pDataObject, CFSTR_DROPDESCRIPTION ) )		// drop tip data present?
				{
					bool changeDropTip = false;
					DROPDESCRIPTION* pDropTip = static_cast<DROPDESCRIPTION*>( ::GlobalLock( stgMedium.hGlobal ) );

					if ( oldStyle )
						changeDropTip = shell::CDropTip::Clear( pDropTip );
					else if ( pDropTip->type <= DROPIMAGE_LINK )
					{
						DROPIMAGETYPE imageType = ole_utl::DropImageTypeFromEffect( dropEffect );

						if ( DROPIMAGE_INVALID != imageType && pDropTip->type != imageType )		// image type doesn't match the drop effect
							if ( m_pDropTip->HasTypeField( imageType ) )				// text tip available?
							{
								changeDropTip = true;
								pDropTip->type = imageType;
								m_pDropTip->MakeDescription( pDropTip, imageType );
							}
							else
								changeDropTip = shell::CDropTip::Clear( pDropTip );		// use system default text
					}
					::GlobalUnlock( stgMedium.hGlobal );

					if ( changeDropTip )
					{	// update the drop tip data object when image type or description text has been changed
						if ( !HR_OK( m_pDataObject->SetData( &formatEtc, &stgMedium, TRUE ) ) )
							changeDropTip = false;
					}
					if ( !changeDropTip )
						::ReleaseStgMedium( &stgMedium );
				}
			}

			if ( !oldStyle )
			{
				if ( m_setCursor )
					::SetCursor( GetNormalCursor() );

				SetDragImageCursor( dropEffect );			// show new style drag cursors
				result = S_OK;								// don't show default (old style) drag cursor
			}
			// When the old style drag cursor is actually used, the Windows cursor must be set
			//  to the default arrow cursor when showing new style drag cursors the next time.
			// If a new style drag cursor is actually shown, the cursor has been set above.
			// By using this flag, the cursor must be set only once when entering a window that
			//  supports new style drag cursors and not with each call when moving over it.
			m_setCursor = oldStyle;
		}
		return result;
	}

	HCURSOR CDropSource::GetNormalCursor( void )
	{
		// for OCR_NORMAL: add '#define OEMRESOURCE' on top of pch.h
		static HCURSOR hCursor = (HCURSOR)::LoadImage( nullptr, MAKEINTRESOURCE( OCR_NORMAL ), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED );
		return hCursor;
	}

} //namespace ole
