#ifndef AlbumDoc_h
#define AlbumDoc_h
#pragma once

#include "utl/UI/ImagePathKey.h"
#include "DocumentBase.h"
#include "AlbumModel.h"
#include "SlideData.h"
#include "AutoDrop.h"
#include "CustomOrderUndoRedo.h"


class CAlbumImageView;
class CImageState;
interface ICatalogStorage;


class CAlbumDoc : public CDocumentBase
{
	DECLARE_DYNCREATE( CAlbumDoc )

	friend class CCatalogStorageTests;

	CAlbumDoc( void );
public:
	virtual ~CAlbumDoc();

	// base overrides
	virtual void Serialize( CArchive& archive );

	app::ModelSchema GetModelSchema( void ) const { return m_model.GetModelSchema(); }

	const CAlbumModel* GetModel( void ) const { return &m_model; }
	CAlbumModel* RefModel( void ) { return &m_model; }

	COLORREF GetBkColor( void ) const { return m_bkColor; }

	bool HasImages( void ) const { return m_model.AnyFoundFiles(); }
	size_t GetImageCount( void ) const { return m_model.GetFileAttrCount(); }
	bool IsValidIndex( size_t index ) const { return index < GetImageCount(); }
	const fs::ImagePathKey& GetImageFilePathAt( int index ) const;

	CImageState* GetImageState( void ) const;

	void CopyAlbumState( const CAlbumDoc* pSrcDoc );		// when saving as a new document (this)
	void FetchViewState( const fs::CPath& docPath );

	bool IsStorageAlbum( void ) const;
	ICatalogStorage* GetCatalogStorage( void );				// opened storage if album based on a catalog storage (compound document)

	static std::auto_ptr< CAlbumDoc > LoadCatalogStorageAlbum( const fs::CPath& docStgPath );			// load a new image catalog storage
public:
	bool EditAlbum( CAlbumImageView* pActiveView );
	bool AddExplicitFiles( const std::vector< std::tstring >& files, bool doUpdate = true );

	// custom order support
	bool DropCustomOrder( int& rDropIndex, std::vector< int >& rSelIndexes );

	// auto-drop support
	bool InitAutoDropRecipient( void );
	void ClearAutoDropContext( void );

	bool HandleDropRecipientFiles( HDROP hDropInfo, CAlbumImageView* pTargetAlbumView );
	bool ExecuteAutoDrop( void );

	// events
	void OnAlbumModelChanged( AlbumModelChange reason = FM_Init );
private:
	void OnAutoDropRecipientChanged( void );

	bool BuildAlbum( const fs::CPath& searchPath );

	bool LoadCatalogStorage( const fs::CPath& docStgPath );
	bool SaveAsCatalogStorage( const fs::CPath& newDocStgPath );		// save .sld -> .ias, .ias -> .ias

	void _SaveAlbumToArchiveStg( const fs::CPath& docStgPath ) throws_( CException* );		// save existing album to image archive as "_Album.sld" stream

	bool _InternalSaveAsArchiveStg( const fs::CPath& newDocStgPath );		// old style
	void RegenerateModel( AlbumModelChange reason = FM_Init );

	bool UndoRedoCustomOrder( custom_order::COpStack& rFromStack, custom_order::COpStack& rToStack, bool isUndoOp );
	void ClearCustomOrder( custom_order::ClearMode clearMode = custom_order::CM_ClearAll );

	void PrepareToSave( const fs::CPath& docPath );
	bool PromptSaveConvertModelSchema( void ) const;
private:
	CAlbumImageView* GetAlbumImageView( void ) const;
	CSlideData* GetActiveSlideData( void );
public:
	persist CSlideData m_slideData;						// always altered by CAlbumImageView::OnActivateView()
private:
	persist CAlbumModel m_model;						// image file list (search patterns + found files)
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
	std::tstring m_password;							// allow password edititng of any document (including .sld), in preparation for SaveAs .ias
	auto_drop::CContext m_autoDropContext;				// contains the dropped files, used during an auto-drop operation

	// generated stuff
public:
	virtual BOOL OnOpenDocument( LPCTSTR pPathName );
	virtual BOOL OnSaveDocument( LPCTSTR pPathName );
	virtual void DeleteContents( void );				// delete doc items etc
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
