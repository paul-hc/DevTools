
#include "stdafx.h"
#include "WndImageRepository.h"
#include "WindowClass.h"
#include "resource.h"
#include "utl/StringUtilities.h"
#include "utl/UI/Icon.h"
#include "utl/UI/WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CWndImageRepository::CWndImageRepository( void )
{
	// Avoid using PNG with alpha channel - it brakes rendering other windows' icons without alpha by drawing a black background;
	// Use BMP 24bpp, otherwise PNG breaks image list transparency in 24bpp.
	VERIFY( Image_Count == res::LoadImageListDIB( m_imageList, IDR_WND_TYPES_STRIP, color::ToolStripPink ) );

	RegisterClasses( _T("#32769"), Image_Desktop );
	RegisterClasses( _T("#32770"), Image_Dialog );
	RegisterClasses( _T("#32768"), Image_PopupMenu );

	RegisterClasses( _T("Button"), Image_Button );
	RegisterClasses( _T("Edit"), Image_Edit );
	RegisterClasses( _T("ComboBox"), Image_ComboBox );
	RegisterClasses( _T("ListBox|ComboLBox"), Image_ListBox );
	RegisterClasses( _T("Static"), Image_StaticText );
	RegisterClasses( _T("SysLink"), Image_HyperLink );
	RegisterClasses( _T("ScrollBar"), Image_ScrollHoriz );
	RegisterClasses( _T("msctls_TrackBar32|msctls_TrackBar"), Image_Slider );
	RegisterClasses( _T("msctls_UpDown32|msctls_UpDown"), Image_SpinButton );
	RegisterClasses( _T("msctls_Progress32|msctls_Progress"), Image_ProgressBar );
	RegisterClasses( _T("msctls_HotKey32|msctls_HotKey"), Image_HotKey );
	RegisterClasses( _T("SysListView32|SysListView"), Image_ListCtrl );
	RegisterClasses( _T("SysTreeView32|SysTreeView"), Image_TreeCtrl );
	RegisterClasses( _T("SysTabControl32|SysTabControl"), Image_TabCtrl );
	RegisterClasses( _T("SysHeader32|SysHeader"), Image_HeaderCtrl );
	RegisterClasses( _T("SysAnimate32"), Image_AnimationCtrl );
	RegisterClasses( _T("SysDateTimePick32"), Image_DateTimeCtrl );
	RegisterClasses( _T("SysMonthCal32"), Image_CalendarCtrl );
	RegisterClasses( _T("SysIPAddress32"), Image_IPAddress );
	RegisterClasses( _T("ComboBoxEx32"), Image_ComboBoxEx );
	RegisterClasses( _T("ToolbarWindow32|ToolbarWindow"), Image_ToolBar );
	RegisterClasses( _T("tooltips_class32|tooltips_class"), Image_ToolTip );
	RegisterClasses( _T("msctls_NetAddress"), Image_NetAddress );
}

CWndImageRepository::~CWndImageRepository()
{
}

CWndImageRepository& CWndImageRepository::Instance( void )
{
	static CWndImageRepository s_repository;
	return s_repository;
}

void CWndImageRepository::RegisterClasses( const TCHAR* pWndClasses, WndImage image )
{
	std::vector< std::tstring > wndClasses;
	str::Split( wndClasses, pWndClasses, _T("|") );

	for ( std::vector< std::tstring >::const_iterator itWndClass = wndClasses.begin(); itWndClass != wndClasses.end(); ++itWndClass )
		m_classToImageMap[ str::MakeUpper( *itWndClass ) ] = image;		// class name key in upper case
}

WndImage CWndImageRepository::LookupImage( HWND hWnd ) const
{
	if ( !ui::IsValidWindow( hWnd ) )
		return Image_Transparent;

	DWORD style = ui::GetStyle( hWnd );

	std::tstring wndClass = wc::GetClassName( hWnd );
	str::ToUpper( wndClass );

	std::unordered_map< std::tstring, WndImage >::const_iterator itFound = m_classToImageMap.find( wndClass );
	if ( itFound != m_classToImageMap.end() )
	{
		switch ( itFound->second )
		{
			case Image_Button:
				switch ( style & BS_TYPEMASK )
				{
					case BS_CHECKBOX:
					case BS_AUTOCHECKBOX:
					case BS_3STATE:
					case BS_AUTO3STATE:
						return Image_CheckBox;
					case BS_RADIOBUTTON:
					case BS_AUTORADIOBUTTON:
						return Image_RadioButton;
					case BS_GROUPBOX:
						return Image_GroupBox;
				}
				break;
			case Image_StaticText:
				switch ( style & SS_TYPEMASK )
				{
					case SS_ICON:
					case SS_BITMAP:
						return Image_Picture;
				}
				break;
			case Image_ScrollHoriz:
				if ( HasFlag( style, SBS_VERT ) )
					return Image_ScrollVert;
				break;
		}

		return itFound->second;
	}

	if ( wc::IsRichEdit( wndClass.c_str() ) )
		return Image_RichEdit;

	// fallback to default window types
	if ( HasFlag( style, WS_CHILD ) )
		return Image_Child;
	else if ( HasFlag( style, WS_POPUP ) )
		return Image_Popup;
	else
		return Image_Overlapped;
}
