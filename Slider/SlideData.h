#ifndef SlideData_h
#define SlideData_h
#pragma once

#include "ListViewState.h"


namespace af
{
	enum AlbumFlags				// persistent flags inherited by the CAlbumImageView objects
	{
		ShowThumbView			= BIT_FLAG( 0 ),	// visibility of the thumbnail list view
		ShowAlbumDialogBar		= BIT_FLAG( 1 ),	// visibility of the album dialog-bar
		SaveCustomOrderUndoRedo	= BIT_FLAG( 2 ),

			DefaultFlags = ShowThumbView | ShowAlbumDialogBar | SaveCustomOrderUndoRedo
	};
}


// used as argument for CAlbumImageView::OnFileListChanged(0 and CAlbumDoc::OnFileListChanged()
enum FileListChangeType
{
	FL_Init,				// normal initialization
	FL_Regeneration,		// file list was regenerated
	FL_CustomOrderChanged,	// file list was changed as a result of a custom order drag&drop operation
	FL_AutoDropOp			// file list was regenerated as a result of an auto-drop operation
};


class CSlideData
{
public:
	CSlideData( void );
	~CSlideData();

	typedef UINT TFirstDataMember;

	void Stream( CArchive& archive, TFirstDataMember* pExtracted_SlideDelay = NULL );

	const CListViewState& GetCurrListState( void ) const { return m_currListState; }
	CListViewState& RefCurrListState( void ) { return m_currListState; }

	int GetCurrentIndex( void ) const { return m_currListState.GetCaretIndex(); }
	bool SetCurrentIndex( int currIndex, bool resetListState = true );
public:
	// sliding
	persist TFirstDataMember m_slideDelay;		// in miliseconds
	persist bool m_dirForward;
	persist bool m_circular;

	persist int m_viewFlags;				// slider inherited flags
	persist int m_thumbListColCount;
private:
	persist CListViewState m_currListState;
};


#endif // SlideData_h
