#ifndef Application_fwd_h
#define Application_fwd_h
#pragma once

#include "utl/AppTools.h"
#include "utl/UI/UserReport.h"
#include "utl/UI/ShellDialogs_fwd.h"
#include "ModelSchema.h"


class CListSelectionData;
namespace ui { interface IUserReport; }


namespace app
{
	enum CustomColors { ColorErrorBk = color::LightPastelPink, ColorErrorText = color::Red };

	enum ContextPopup { ImagePopup, AlbumPopup, AlbumThumbsPopup, AlbumFoundListPopup, DropPopup };


	void LogLine( const TCHAR* pFormat, ... );		// in normal runtime log
	void LogEvent( const TCHAR* pFormat, ... );		// in the event log, that usually keeps track of major operations, detailed errors, etc.
	void HandleException( CException* pExc, UINT mbType = MB_ICONWARNING, bool doDelete = true );
	int HandleReportException( CException* pExc, UINT mbType = MB_ICONERROR, UINT msgId = 0, bool doDelete = true );

	ui::IUserReport& GetUserReport( void );		// app::CInteractiveMode

	bool IsSlideFile( const TCHAR* pFilePath );
	bool IsCatalogFile( const TCHAR* pFilePath );

	inline bool IsAlbumFile( const TCHAR* pFilePath ) { return IsSlideFile( pFilePath ) || IsCatalogFile( pFilePath ); }

	const std::tstring& GetAllSourcesWildSpecs( void );
	bool BrowseAlbumFile( fs::CPath& rFullPath, CWnd* pParentWnd, shell::BrowseMode browseMode = shell::FileOpen, DWORD flags = 0 );
	bool BrowseCatalogFile( fs::CPath& rFullPath, CWnd* pParentWnd, shell::BrowseMode browseMode = shell::FileOpen, DWORD flags = 0 );
}


enum UpdateViewHint
{
	Hint_ViewUpdate,
	Hint_RedrawAll,
	Hint_ToggleFullScreen,				// sent whenever full screen mode is toggled on or off
	Hint_AlbumModelChanged,				// file list has changed; reason: (FileListChangeType)pHint
	Hint_ReloadImage,					// force reloading each image
	Hint_FileChanged,					// an image file has been changed (copied, moved, deleted, etc)
	Hint_DocSlideDataChanged,			// update navigation attribute from the document
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


#endif // Application_fwd_h
