#ifndef Album_fwd_h
#define Album_fwd_h
#pragma once


namespace af
{
	enum AlbumFlags				// persistent flags inherited by the CAlbumImageView objects
	{
		ShowThumbView			= BIT_FLAG( 0 ),	// visibility of the thumbnail list view
		ShowAlbumDialogBar		= BIT_FLAG( 1 ),	// visibility of the album dialog-bar

		SaveCustomOrderUndoRedo	= BIT_FLAG( 2 ),

		AutoSeekAlbumImagePos	= BIT_FLAG( 4 ),		// auto-seek to image index (album bar spin-edit)

			DefaultFlags = ShowThumbView | ShowAlbumDialogBar | SaveCustomOrderUndoRedo,
			ShowMask = ShowThumbView | ShowAlbumDialogBar
	};

	typedef int TAlbumFlags;
}


// used as argument for CAlbumImageView::OnAlbumModelChanged(0 and CAlbumDoc::OnAlbumModelChanged()
enum AlbumModelChange
{
	AM_Init,				// normal initialization
	AM_Regeneration,		// file list was regenerated
	AM_CustomOrderChanged,	// file list was changed as a result of a custom order drag&drop operation
	AM_AutoDropOp			// file list was regenerated as a result of an auto-drop operation
};


#endif // Album_fwd_h
