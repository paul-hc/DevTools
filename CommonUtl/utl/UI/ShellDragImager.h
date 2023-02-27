#ifndef ShellDragImager_h
#define ShellDragImager_h
#pragma once

#include <shobjidl.h>


class COleDataSource;		// for IDataObject*


namespace shell
{
	extern UINT WM_DI_GETDRAGIMAGE;			// sent by Explorer to a window to create a drag image for the selection; CListCtrl and CTreeCtrl provide standard implementations by default


	HBITMAP GetDragImage( SHDRAGIMAGE& rOutDragImage, HWND hCtrl );			// works by default for CListCtrl, CTreeCtrl


	// creates drag images and enables drop-tips using IDragSourceHelper
	//
	class CDragImager
	{
	public:
		CDragImager( COleDataSource* pDataSource );

		IDragSourceHelper* GetHelper( void ) const { return m_pDragSourceHelper; }
		IDragSourceHelper2* GetHelper2( void ) const { return m_pDragSourceHelper2; }

		bool IsValid( void ) const { return m_pDragSourceHelper != nullptr; }

		bool SetFromWindow( HWND hWnd, const POINT* pOrigin = nullptr );		// window must handle DI_GETDRAGIMAGE message (built-in by CListCtrl, CTreeCtrl)

		// shell helper takes ownership of the bitmap;  transpColor: CLR_NONE for no transparent color, CLR_DEFAULT for COLOR_WINDOW
		bool SetFromBitmap( HBITMAP hBitmap, COLORREF transpColor, const POINT* pOrigin = nullptr );
		bool SetFromBitmap( CBitmap& rBitmap, COLORREF transpColor, const POINT* pOrigin = nullptr );
	private:
		IDataObject* m_pDataObject;								// has the cached data formats (for selection), coming from COleDataSource back-pointer
		CComPtr<IDragSourceHelper> m_pDragSourceHelper;		// drag image helper
		CComPtr<IDragSourceHelper2> m_pDragSourceHelper2;		// drag image helper 2 (SetFlags method, Vista+)
	};
}


#endif // ShellDragImager_h
