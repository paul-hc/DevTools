#ifndef Workspace_h
#define Workspace_h
#pragma once

#include "ModelSchema.h"
#include "ImageState.h"
#include "utl/Path.h"
#include "utl/UI/Image_fwd.h"		// ui::ImageScalingMode
#include "utl/UI/WindowPlacement.h"


class CMainFrame;


namespace wf
{
	enum WorkspaceFlag
	{
		PersistOpenDocs				= BIT_FLAG( 0 ),		// automatic workspace save
		MdiMaximized				= BIT_FLAG( 1 ),		// open MDI childred maximized
		ShowToolBar					= BIT_FLAG( 2 ),		// show main toolbar
		ShowStatusBar				= BIT_FLAG( 3 ),		// show statusbar
		Old_InitStretchToFit		= BIT_FLAG( 4 ),		// opens each image in stretch to fit mode (obsolete, relaced by m_scalingMode)
		AllowEmbeddedFileTransfers	= BIT_FLAG( 7 ),		// allows logical files to be temporary backed-up to physical files on file operations
		PersistAlbumImageState		= BIT_FLAG( 8 ),		// save zoom/scroll info in albums
		DeepStreamPaths				= BIT_FLAG( 9 ),		// prefix image stream names with the relative path to original reference folder
		UseVistaStyleFileDialog		= BIT_FLAG( 16 ),		// Vista-style vs classic file open/save dialog
		UseThemedThumbListDraw		= BIT_FLAG( 17 ),		// display thumbs list selection using the "LISTVIEW" theme

			DefaultFlags =
				ShowToolBar | ShowStatusBar | AllowEmbeddedFileTransfers |
				PersistAlbumImageState | DeepStreamPaths |
				UseVistaStyleFileDialog | UseThemedThumbListDraw,

			WkspDialogMask =
				PersistOpenDocs | AllowEmbeddedFileTransfers
	};

	typedef int TWorkspaceFlags;
}


struct CWorkspaceData
{
	CWorkspaceData( void );

	void Save( CArchive& archive );
	app::ModelSchema Load( CArchive& archive );

	bool operator==( const CWorkspaceData& right ) const { return pred::Equal == memcmp( this, &right, sizeof( CWorkspaceData ) ); }
	bool operator!=( const CWorkspaceData& right ) const { return !operator==( right ); }

	CSize GetThumbBoundsSize( void ) const { return CSize( m_thumbBoundsSize, m_thumbBoundsSize ); }
	COLORREF GetImageSelColor( void ) const { return m_imageSelColor != color::Null ? m_imageSelColor : GetSysColor( COLOR_HIGHLIGHT ); }
	COLORREF GetImageSelTextColor( void ) const { return m_imageSelTextColor != color::Null ? m_imageSelTextColor : GetSysColor( COLOR_HIGHLIGHTTEXT ); }
public:
	persist bool m_autoSave;						// automatic saving the workspace
	persist wf::TWorkspaceFlags m_wkspFlags;		// workspace flags
	persist int m_albumViewFlags;					// albume view inherited flags
	persist int m_mruCount;							// maximum count of files in the MRU list [0-10]
	persist int m_thumbListColumnCount;				// default count of columns in the thumb list
	persist int m_thumbBoundsSize;					// size of the square bounds of the thumbnails (app::Slider_v3_6+)
	persist ui::ImageScalingMode m_scalingMode;		// default image scaling mode (app::Slider_v4_0+)
	persist COLORREF m_defBkColor;					// default background color
	persist COLORREF m_imageSelColor;				// image selection color
	persist COLORREF m_imageSelTextColor;			// image selection text color
};


// singleton that gets loaded just before main frame creation

class CWorkspace : public CCmdTarget
{
	CWorkspace( void );
	virtual ~CWorkspace();
public:
	static CWorkspace& Instance( void );
	static const CWorkspaceData& GetData( void ) { return Instance().m_data; }
	static CWorkspaceData& RefData( void ) { return Instance().m_data; }
	static int GetFlags( void ) { return Instance().m_data.m_wkspFlags; }

	static const CWorkspaceData& GetLiveData( void ) { return Instance().m_pEditingData != nullptr ? *Instance().m_pEditingData : Instance().m_data; }

	virtual void Serialize( CArchive& archive );

	// all settings (registry + binary file)
	bool LoadSettings( void );
	bool SaveSettings( void );

	// registry-based settings
	void LoadRegSettings( void );
	void SaveRegSettings( void );

	bool IsFullScreen( void ) const { return m_isFullScreen; }

	CWindowPlacement* GetLoadedPlacement( void ) { return !m_mainPlacement.IsEmpty() ? &m_mainPlacement : nullptr; }		// valid placement if it was loaded

	COLORREF GetImageSelTextColor( void ) const { return m_data.GetImageSelTextColor(); }
	COLORREF GetImageSelColor( void ) const { return m_data.GetImageSelColor(); }
	CBrush& GetImageSelColorBrush( void ) const { return const_cast<CBrush&>( m_imageSelColorBrush ); }

	CImageState* RefLoadingImageState( void ) { return m_pLoadingImageState; }

	UINT GetDefaultSlideDelay( void ) const { return m_defaultSlideDelay; }

	// operations
	void StoreMainFrame( CMainFrame* pMainFrame );
	void ApplySettings( void );
	void FetchSettings( void );
	bool LoadDocuments( void );

	void ToggleFullScreen( void );
private:
	void SetImageSelColor( COLORREF imageSelColor );
private:
	fs::CPath m_filePath;
	bool m_delayFullScreen;							// intermediate mirror flag for m_isFullScreen

	CMainFrame* m_pMainFrame;
	CImageState* m_pLoadingImageState;
	CBrush m_imageSelColorBrush;

	persist CWorkspaceData m_data;					// data subset edited in CWorkspaceDialog
	persist bool m_isFullScreen;					// if true switch main frame in full-screen mode
	persist CWindowPlacement m_mainPlacement;		// placement of the main frame
	persist DWORD m_reserved;
	persist std::vector<CImageState> m_imageStates;

	// registry-based options
	persist UINT m_defaultSlideDelay;				// in miliseconds
private:
	// transient
	const CWorkspaceData* m_pEditingData;			// temporary data while running CWorkspaceDialog
protected:
	// generated stuff
	afx_msg void CmSaveWorkspace( void );
	afx_msg void CmEditWorkspace( void );
	afx_msg void CmLoadWorkspaceDocs( void );
	afx_msg void OnToggleFullScreen( void );
	afx_msg void OnUpdateFullScreen( CCmdUI* pCmdUI );
	afx_msg void OnToggle_SmoothingMode( void );
	afx_msg void OnUpdate_SmoothingMode( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // Workspace_h
