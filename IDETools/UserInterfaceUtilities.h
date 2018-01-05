#ifndef UserInterfaceUtilities_h
#define UserInterfaceUtilities_h
#pragma once


namespace ui
{
//	CString GetWindowText( const CWnd* pWnd );
//	void SetWindowText( CWnd* pWnd, const TCHAR* pText );

//	CString GetControlText( const CWnd* pWnd, UINT ctrlId );
//	void SetControlText( CWnd* pWnd, UINT ctrlId, const TCHAR* pText );

	HICON smartLoadIcon( LPCTSTR iconID, bool asLarge = true, HINSTANCE hResInst = AfxGetResourceHandle() );

	inline HICON smartLoadIcon( UINT iconId, bool asLarge = true, HINSTANCE hResInst = AfxGetResourceHandle() )
	{
		return smartLoadIcon( MAKEINTRESOURCE( iconId ), asLarge, hResInst );
	}

	bool smartEnableWindow( CWnd* pWnd, bool enable = true );
	void smartEnableControls( CWnd* pDlg, const UINT* pCtrlIds, size_t ctrlCount, bool enable = true );

	void commitMenuEnabling( CWnd& targetWnd, CMenu& popupMenu );
	void updateControlsUI( CWnd& targetWnd );
	CString GetCommandText( CCmdUI& rCmdUI );


	enum HorzAlign { Horz_NoAlign, Horz_AlignLeft, Horz_AlignCenter, Horz_AlignRight };
	enum VertAlign { Vert_NoAlign, Vert_AlignTop, Vert_AlignCenter, Vert_AlignBottom };

	CRect& alignRect( CRect& restDest, const CRect& rectFixed, HorzAlign horzAlign = Horz_AlignCenter,
					  VertAlign vertAlign = Vert_AlignCenter, bool limitDest = false );

	CRect& centerRect( CRect& restDest, const CRect& rectFixed, bool horizontally = true, bool vertically = true,
					   bool limitDest = false, const CSize& addOffset = CSize( 0, 0 ) );

	bool ensureVisibleRect( CRect& restDest, const CRect& rectFixed, bool horizontally = true, bool vertically = true );
	bool ensurePointInRect( POINT& point, const RECT& rect );

	inline CPoint MulDiv_Size( const CPoint& point, int multiplyBy, int divideBy = 100 )
	{
		return CPoint( MulDiv( point.x, multiplyBy, divideBy ), MulDiv( point.y, multiplyBy, divideBy ) );
	}

	inline CSize MulDiv_Size( const CSize& size, int multiplyBy, int divideBy = 100 )
	{
		return CSize( MulDiv( size.cx, multiplyBy, divideBy ), MulDiv( size.cy, multiplyBy, divideBy ) );
	}

} // namespace ui


#endif // UserInterfaceUtilities_h
