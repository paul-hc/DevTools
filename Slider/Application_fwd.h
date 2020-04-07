#ifndef Application_fwd_h
#define Application_fwd_h
#pragma once


class CListSelectionData;
namespace ui { interface IUserReport; }


namespace app
{
	enum SliderVersion
	{
		Slider_v3_1 = 0,				// original workspace document format up to this version
		Slider_v3_2 = 0x32,				// persist CWorkspace::m_imageStates
		Slider_v3_5 = 0x35,				// persist the new CWorkspaceData struct
		Slider_v3_6 = 0x36,				// persist CWorkspaceData::m_thumbBoundsSize
		Slider_v3_7 = 0x37,				// persist CFileAttr::GetImageDim() - evaluate real image dimensions
		Slider_v3_8 = 0x38,				// persist CFileAttr::GetImageDim() - evaluate real image dimensions
		Slider_v4_0 = 0x40,				// persist CFileAttr::GetImageDim() - evaluate real image dimensions
			// * always update to the LATEST VERSION *
			Slider_LatestVersion = Slider_v4_0
	};

	std::tstring FormatSliderVersion( SliderVersion version );


	enum ContextPopup { ImagePopup, AlbumPopup, AlbumThumbsPopup, AlbumFoundListPopup, DropPopup };


	ui::IUserReport& GetUserReport( void );		// app::CInteractiveMode

	bool IsImageArchiveDoc( const TCHAR* pFilePath );


	class CScopedProgress;
}


enum UpdateViewHint
{
	Hint_ViewUpdate,
	Hint_RedrawAll,
	Hint_ToggleFullScreen,				// sent whenever full screen mode is toggled on or off
	Hint_FileListChanged,				// file list has changed; reason: (FileListChangeType)pHint
	Hint_ReloadImage,					// force reloading each image
	Hint_FileChanged,					// an image file has been changed (copied, moved, deleted, etc)
	Hint_DocSlideDataChanged,				// update navigation attribute from the document
	Hint_BackupCurrSelection,			// save the current selection (for further selection restore)
	Hint_BackupNearSelection,			// save the element near the current selection (for further selection restore)
	Hint_SmartBackupSelection,			// save the selection either current or near based on intersection between current selection and move operation (for further selection restore)
	Hint_RestoreSelection,				// restore selection to the one previously saved through Hint_BackupNearSelection
	Hint_ThumbBoundsResized,			// thumbnailer's bounds size has change
		Hint_Null = -1
};


namespace app
{
	template< typename ValueType >
	inline CObject* ToHintPtr( ValueType pHint ) { return reinterpret_cast< CObject* >( pHint ); }

	template< typename ValueType >
	inline ValueType FromHintPtr( CObject* pHint ) { return static_cast< ValueType >( (UINT_PTR)pHint ); }
}


namespace app
{
	enum CustomColors { ColorErrorBk = color::LightPastelPink, ColorErrorText = color::Red };
}


#endif // Application_fwd_h
