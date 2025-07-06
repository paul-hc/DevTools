#ifndef WndImageRepository_h
#define WndImageRepository_h
#pragma once

#include <unordered_map>


enum WndImage
{
	Image_Transparent, Image_Desktop, Image_Popup, Image_Overlapped, Image_Child, Image_Dialog, Image_PopupMenu,
	Image_Button, Image_CheckBox, Image_RadioButton, Image_Edit,
	Image_ComboBox, Image_ComboBoxEx, Image_ListBox, Image_GroupBox,
	Image_StaticText, Image_HyperLink, Image_Picture,
	Image_ScrollHoriz, Image_ScrollVert, Image_Slider, Image_SpinButton, Image_ProgressBar, Image_HotKey,
	Image_ListCtrl, Image_TreeCtrl, Image_TabCtrl, Image_HeaderCtrl,
	Image_AnimationCtrl, Image_RichEdit,
	Image_DateTimeCtrl, Image_CalendarCtrl, Image_IPAddress,
	Image_ToolBar, Image_StatusBar, Image_ToolTip,
	Image_NetAddress,
		Image_Count
};


class CWndImageRepository
{
	CWndImageRepository( void );
	~CWndImageRepository();

	void RegisterClasses( const TCHAR* pWndClasses, WndImage image );
public:
	static CWndImageRepository& Instance( void );

	CImageList* GetImageList( void ) { return &m_imageList; }

	WndImage LookupImage( HWND hWnd ) const;
private:
	std::unordered_map<std::tstring, WndImage> m_classToImageMap;
	CImageList m_imageList;
};


#endif // WndImageRepository_h
