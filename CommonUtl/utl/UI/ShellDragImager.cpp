
#include "stdafx.h"
#include "ShellDragImager.h"
#include "Utilities.h"
#include <afxole.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	UINT WM_DI_GETDRAGIMAGE = ::RegisterWindowMessage( DI_GETDRAGIMAGE );


	HBITMAP GetDragImage( SHDRAGIMAGE& rOutDragImage, HWND hCtrl )
	{
		rOutDragImage.hbmpDragImage = NULL;
		::SendMessage( hCtrl, shell::WM_DI_GETDRAGIMAGE, 0, (LPARAM)&rOutDragImage );
		return rOutDragImage.hbmpDragImage;
	}


	// CDragImager implementation

	CDragImager::CDragImager( COleDataSource* pDataSource )
		: m_pDataObject( NULL )
	{
		// create drag source helper to show drag images
		if ( HR_OK( m_pDragSourceHelper.CoCreateInstance( CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER ) ) )
			m_pDragSourceHelper.QueryInterface( &m_pDragSourceHelper2 );

		if ( pDataSource != NULL )
			m_pDataObject = static_cast<IDataObject*>( pDataSource->GetInterface( &IID_IDataObject ) );
	}

	/*
		Window must handle the registered DI_GETDRAGIMAGE message and fill the passed SHDRAGIMAGE structure (built-in with CListCtrl, CTreeViewCtrl).
		
		If hWnd is NULL, an empty bitmap is created by the shell. Ditto if the control does not handle the DI_GETDRAGIMAGE message.
		In this case the boolean data object "UsingDefaultDragImage" is created and set to true.
		
		IMP: when m_pDataObject contains a cached "Shell IDList Array", this is used to determine the drag image selection (like dragging files from Explorer).
	*/
	bool CDragImager::SetFromWindow( HWND hWnd, const POINT* pOrigin /*= NULL*/ )
	{
		if ( m_pDragSourceHelper != NULL )
		{
			CPoint origin = pOrigin != NULL ? *pOrigin : ui::GetCursorPos( hWnd );

			if ( HR_OK( m_pDragSourceHelper->InitializeFromWindow( hWnd, &origin, m_pDataObject ) ) )
				return true;

			TRACE( _T(" * CDragImager::SetFromWindow - InitializeFromWindow failed\n") );
		}
		return false;
	}

	bool CDragImager::SetFromBitmap( HBITMAP hBitmap, COLORREF transpColor, const POINT* pOrigin /*= NULL*/ )
	{
		ASSERT_PTR( hBitmap );

		if ( hBitmap != NULL )
		{
			if ( m_pDragSourceHelper != NULL )
			{
				BITMAP bm;
				VERIFY( ::GetObject( hBitmap, sizeof( bm ), &bm ) );
				CRect bmpRect( 0, 0, bm.bmWidth, bm.bmHeight );

				SHDRAGIMAGE shDragImage;
				shDragImage.sizeDragImage = bmpRect.Size();

				if ( pOrigin != NULL )
				{
					ASSERT( pOrigin->x >= 0 && pOrigin->y >= 0 );		// must be positive
					shDragImage.ptOffset = *pOrigin;
				}
				else
					shDragImage.ptOffset.x = shDragImage.ptOffset.y = 0;

				shDragImage.hbmpDragImage = hBitmap;
				shDragImage.crColorKey = CLR_DEFAULT == transpColor ? ::GetSysColor( COLOR_WINDOW ) : transpColor;
				if ( HR_OK( m_pDragSourceHelper->InitializeFromBitmap( &shDragImage, m_pDataObject ) ) )
					return true;

				TRACE( _T(" * CDragImager::SetFromBitmap: InitializeFromBitmap failed\n") );
			}
			::DeleteObject( hBitmap );					// avoid leaking bitmap
		}
		return false;
	}

	bool CDragImager::SetFromBitmap( CBitmap& rBitmap, COLORREF transpColor, const POINT* pOrigin /*= NULL*/ )
	{
		return SetFromBitmap( static_cast<HBITMAP>( rBitmap.Detach() ), transpColor, pOrigin );		// detach the HBITMAP handle
	}

} //namespace shell
