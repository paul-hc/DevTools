#ifndef WindowClass_h
#define WindowClass_h
#pragma once

#include <sstream>


namespace wc
{
	std::tstring GetClassName( HWND hWnd );
	std::tstring GetDisplayClassName( HWND hWnd );
	std::tstring GetDisplayClassName( const TCHAR* pClassName );

	std::tstring FormatClassName( const TCHAR* pClassName );
	std::tstring FormatClassName( HWND hWnd );


	bool IsDialogBox( const TCHAR* pClassName );
	bool IsPopupMenu( const TCHAR* pClassName );

	bool IsButton( const TCHAR* pClassName );
	bool IsEdit( const TCHAR* pClassName );
	bool IsStatic( const TCHAR* pClassName );
	bool IsScrollBar( const TCHAR* pClassName );

	bool IsComboBox( const TCHAR* pClassName );
	bool IsComboBoxEx( const TCHAR* pClassName );
	bool IsListBox( const TCHAR* pClassName );
	bool IsHeaderCtrl( const TCHAR* pClassName );
	bool IsListCtrl( const TCHAR* pClassName );
	bool IsTreeCtrl( const TCHAR* pClassName );
	bool IsTabCtrl( const TCHAR* pClassName );
	bool IsRichEdit( const TCHAR* pClassName );
	bool IsMonthCalCtrl( const TCHAR* pClassName );
	bool IsDateTimePickerCtrl( const TCHAR* pClassName );
	bool IsToolbarCtrl( const TCHAR* pClassName );
	bool IsStatusBarCtrl( const TCHAR* pClassName );
	bool IsReBarCtrl( const TCHAR* pClassName );
	bool IsSpinButton( const TCHAR* pClassName );
	bool IsTrackBar( const TCHAR* pClassName );
	bool IsProgress( const TCHAR* pClassName );
	bool IsToolTipCtrl( const TCHAR* pClassName );
	bool IsAnimateCtrl( const TCHAR* pClassName );


	enum ContentType { CaptionText, StructuredText };
	ContentType FormatTextContent( std::tostringstream& ross, const CWnd* pWnd );
}


#endif // WindowClass_h
