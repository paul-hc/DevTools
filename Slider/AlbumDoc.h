#ifndef AlbumDoc_h
#define AlbumDoc_h
#pragma once

#include "utl/UI/ImagePathKey.h"
#include "DocumentBase.h"
#include "FileList.h"
#include "SlideData.h"
#include "AutoDrop.h"
#include "CustomOrderUndoRedo.h"


class CAlbumImageView;
class CImageState;


class CAlbumDoc : public CDocumentBase
{
	DECLARE_DYNCREATE( CAlbumDoc )
public:
	CAlbumDoc( void );
	virtual ~CAlbumDoc();

	// base overrides
	virtual void Serialize( CArchive& archive );

	app::ModelSchema GetFileModelSchema( void ) const { return m_fileModelSchema; }

	COLORREF GetBkColor( void ) const { return m_bkColor; }

	bool HasImages( void ) const { return m_fileList.AnyFoundFiles(); }
	size_t GetImageCount( void ) const { return m_fileList.GetFileAttrCount(); }
	bool IsValidIndex( size_t index ) const { return index < GetImageCount(); }
	const fs::ImagePathKey& GetImageFilePathAt( int index ) const;

	void QueryNeighboringPathKeys( std::vector< fs::ImagePathKey >& rNeighbours, size_t index ) const;

	CImageState* GetImageState( void ) const;

	bool SaveAsArchiveStg( const fs::CPath& newStgPath );

	bool BuildAlbum( const fs::CPath& filePath );
	void RegenerateFileList( FileListChangeType reason = FL_Init );

	bool EditAlbum( CAlbumImageView* pActiveView );
	bool AddExplicitFiles( const std::vector< std::tstring >& files, bool doUpdate = true );

	// custom order support
	bool MakeCustomOrder( int& rToDestIndex, std::vector< int >& rToMoveIndexes );
	bool UndoRedoCustomOrder( custom_order::COpStack& rFromStack, custom_order::COpStack& rToStack, bool isUndoOp );
	void ClearCustomOrder( custom_order::ClearMode clearMode = custom_order::CM_ClearAll );

	// auto-drop support
	bool InitAutoDropRecipient( void );
	void ClearAutoDropContext( void );

	bool HandleDropRecipientFiles( HDROP hDropInfo, CAlbumImageView* pTargetAlbumView );
	bool ExecuteAutoDrop( void );

	// events
	void OnFileListChanged( FileListChangeType reason = FL_Init );
private:
	void OnAutoDropRecipientChanged( void );
private:
	CAlbumImageView* GetOwnActiveAlbumView( void ) const;
	CSlideData* GetActiveSlideData( void );
public:
	persist app::ModelSchema m_fileModelSchema;			// loaded model schema from file
	persist CFileList m_fileList;						// image file list (search specifiers + found files)
	persist CSlideData m_slideData;						// always altered by CAlbumImageView::OnActivateView()
private:
	persist COLORREF m_bkColor;							// album background color
	persist int m_docFlags;								// persistent document flags
	persist custom_order::COpStack m_customOrderUndoStack;	
	persist custom_order::COpStack m_customOrderRedoStack;
	persist auto_drop::COpStack m_dropUndoStack;		// contains the stack of performed move/copy operations to be undone on demand
	persist auto_drop::COpStack m_dropRedoStack;		// contains the stack of undone move/copy operations to be redone on demand
	persist std::auto_ptr< CImageState > m_pImageState;	// conditional on PresistImageState from v3.2+

	enum DocFlags
	{
		PresistImageState	= BIT_FLAG( 0 )
	};
	enum DocStatus { Clean, Dirty, DirtyOpening = -1 };
public:
	// transient
	auto_drop::CContext m_autoDropContext;				// contains the dropped files, used during an auto-drop operation
public:
	// generated stuff
	public:
	virtual BOOL OnNewDocument( void );
	virtual BOOL OnOpenDocument( LPCTSTR pPathName );
	virtual BOOL OnSaveDocument( LPCTSTR pPathName );
	virtual void OnCloseDocument( void );
protected:
	afx_msg void CmAutoDropDefragment( void );
	afx_msg void OnUpdateAutoDropDefragment( CCmdUI* pCmdUI );
	afx_msg void CmAutoDropUndo( void );
	afx_msg void OnUpdateAutoDropUndo( CCmdUI* pCmdUI );
	afx_msg void CmAutoDropRedo( void );
	afx_msg void OnUpdateAutoDropRedo( CCmdUI* pCmdUI );
	afx_msg void CmAutoDropClearUndoRedoStacks( void );
	afx_msg void OnUpdateAutoDropClearUndoRedoStacks( CCmdUI* pCmdUI );
	afx_msg void CmRegenerateAlbum( void );
	afx_msg void OnUpdateRegenerateAlbum( CCmdUI* pCmdUI );
	afx_msg void OnToggleIsAutoDrop( void );
	afx_msg void OnUpdateIsAutoDrop( CCmdUI* pCmdUI );
	afx_msg void CmCustomOrderUndo( void );
	afx_msg void OnUpdateCustomOrderUndo( CCmdUI* pCmdUI );
	afx_msg void CmCustomOrderRedo( void );
	afx_msg void OnUpdateCustomOrderRedo( CCmdUI* pCmdUI );
	afx_msg void CmClearCustomOrderUndoRedoStacks( void );
	afx_msg void OnUpdateClearCustomOrderUndoRedoStacks( CCmdUI* pCmdUI );
	afx_msg void CmArchiveImages( void );
	afx_msg void OnUpdateArchiveImages( CCmdUI* pCmdUI );
	afx_msg void OnEditArchivePassword( void );
	afx_msg void OnUpdateEditArchivePassword( CCmdUI* pCmdUI );
	afx_msg void OnToggleSaveCOUndoRedoBuffer( void );
	afx_msg void OnUpdateSaveCOUndoRedoBuffer( CCmdUI* pCmdUI );
	afx_msg void CmSelectAllThumbs( void );
	afx_msg void OnUpdateSelectAllThumbs( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // AlbumDoc_h
