#ifndef Workspace_h
#define Workspace_h
#pragma once

#include "ModelSchema.h"
#include "ImageState.h"
#include "WindowPlacement.h"
#include "utl/Path.h"
#include "utl/UI/Image_fwd.h"		// ui::AutoImageSize


class CMainFrame;


namespace wf
{
	enum WorkspaceFlags
	{
		PersistOpenDocs				= BIT_FLAG( 0 ),		// automatic workspace save
		MdiMaximized				= BIT_FLAG( 1 ),		// open MDI childred maximized
		ShowToolBar					= BIT_FLAG( 2 ),		// show main toolbar
		ShowStatusBar				= BIT_FLAG( 3 ),		// show statusbar
		Old_InitStretchToFit		= BIT_FLAG( 4 ),		// opens each image in stretch to fit mode (obsolete, relaced by m_autoImageSize)
		AllowEmbeddedFileTransfers	= BIT_FLAG( 7 ),		// allows logical files to be temporary backed-up to physical files on file operations
		PersistAlbumImageState		= BIT_FLAG( 8 ),		// save zoom/scroll info in albums
		PrefixDeepStreamNames		= BIT_FLAG( 9 ),		// prefix image stream names with the relative path to original reference folder
		UseVistaStyleFileDialog		= BIT_FLAG( 16 ),		// Vista-style vs classic file open/save dialog

			DefaultFlags = ShowToolBar | ShowStatusBar | AllowEmbeddedFileTransfers |
						   PersistAlbumImageState | PrefixDeepStreamNames | UseVistaStyleFileDialog,
			WkspDialogMask = PersistOpenDocs | AllowEmbeddedFileTransfers
	};
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
	persist int m_wkspFlags;						// workspace flags
	persist int m_albumViewFlags;					// albume view inherited flags
	persist int m_mruCount;							// maximum count of files in the MRU list [0-10]
	persist int m_thumbListColCount;				// default count of columns in the thumb list
	persist int m_thumbBoundsSize;					// size of the square bounds of the thumbnails (app::Slider_v3_6+)
	persist ui::AutoImageSize m_autoImageSize;		// default auto image size (app::Slider_v4_0+)
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

	virtual void Serialize( CArchive& archive );

	bool LoadSettings( void );
	bool SaveSettings( void );

	// registry-based settings
	void LoadRegSettings( void );
	void SaveRegSettings( void );

	bool IsLoaded( void ) const { return m_isLoaded; }
	bool IsFullScreen( void ) const { return m_isFullScreen; }

	const CWindowPlacement* GetLoadedPlacement( void ) const { return m_isLoaded ? &m_mainPlacement : NULL; }

	COLORREF GetImageSelTextColor( void ) const { return m_data.GetImageSelTextColor(); }
	COLORREF GetImageSelColor( void ) const { return m_data.GetImageSelColor(); }
	CBrush& GetImageSelColorBrush( void ) const { return const_cast< CBrush& >( m_imageSelColorBrush ); }

	CImageState* GetLoadingImageState( void ) { return m_pLoadingImageState; }

	// operations
	void FetchSettings( void );
	bool LoadDocuments( void );
	void AdjustForcedBehaviour( void );

	void ToggleFullScreen( void );
private:
	void SetImageSelColor( COLORREF imageSelColor );
private:
	CMainFrame* m_pMainFrame;
	fs::CPath m_filePath;
	bool m_isLoaded;								// workspace loaded from an existing .slw file
	bool m_delayFullScreen;							// intermediate mirror flag for m_isFullScreen
	CImageState* m_pLoadingImageState;
	CBrush m_imageSelColorBrush;

	persist CWorkspaceData m_data;					// data subset edited in CWorkspaceDialog
	persist bool m_isFullScreen;					// if TRUE open main frame in full-screen mode
	persist CWindowPlacement m_mainPlacement;		// placement of the main frame
	persist DWORD m_reserved;
	persist std::vector< CImageState > m_imageStates;

	// registry-based options
	persist UINT m_defaultSlideDelay;			// in miliseconds
protected:
	// generated stuff
	afx_msg void CmSaveWorkspace( void );
	afx_msg void CmEditWorkspace( void );
	afx_msg void CmLoadWorkspaceDocs( void );
	afx_msg void OnToggle_SmoothingMode( void );
	afx_msg void OnUpdate_SmoothingMode( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // Workspace_h
