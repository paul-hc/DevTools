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
class CProgressService;
interface ICatalogStorage;


class CAlbumDoc : public CDocumentBase
{
	friend class CCatalogStorageTests;

	DECLARE_DYNCREATE( CAlbumDoc )

	CAlbumDoc( void );
public:
	virtual ~CAlbumDoc();

	// base overrides
	virtual void Serialize( CArchive& archive );

	app::ModelSchema GetModelSchema( void ) const { return m_model.GetModelSchema(); }

	const CAlbumModel* GetModel( void ) const { return &m_model; }
	CAlbumModel* RefModel( void ) { return &m_model; }

	COLORREF GetBkColor( void ) const { return m_bkColor; }
	utl::Ternary GetSmoothingMode( void ) const { return m_smoothingMode; }

	bool HasImages( void ) const { return m_model.AnyFoundFiles(); }
	size_t GetImageCount( void ) const { return m_model.GetFileAttrCount(); }
	bool IsValidIndex( size_t index ) const { return index < GetImageCount(); }
	const fs::ImagePathKey& GetImageFilePathAt( int index ) const;

	CImageState* GetImageState( void ) const;

	void CopyAlbumState( const CAlbumDoc* pSrcDoc );		// when saving as a new document (this)
	void FetchViewState( const fs::CPath& docPath );

	bool IsStorageAlbum( void ) const;
	ICatalogStorage* GetCatalogStorage( void );				// opened storage if album based on a catalog storage (compound document)

	static std::auto_ptr< CAlbumDoc > LoadAlbumDocument( const fs::CPath& docPath );			// load a new image album (slide or catalog storage)

	// events
	void OnAlbumModelChanged( AlbumModelChange reason = AM_Init );
public:
	bool EditAlbum( CAlbumImageView* pActiveView );
	bool AddExplicitFiles( const std::vector< fs::CPath >& filePaths, bool doUpdate = true );
	TCurrImagePos DeleteFromAlbum( const std::vector< fs::CFlexPath >& selFilePaths );

	// custom order support
	bool DropCustomOrder( int& rDropIndex, std::vector< int >& rSelIndexes );

	// auto-drop support
	bool InitAutoDropRecipient( void );
	void ClearAutoDropContext( void );

	bool HandleDropRecipientFiles( HDROP hDropInfo, CAlbumImageView* pTargetAlbumView );
	bool ExecuteAutoDrop( void );
protected:
	// base overrides
	virtual CWicImage* GetCurrentImage( void ) const;
	virtual bool QuerySelectedImagePaths( std::vector< fs::CFlexPath >& rSelImagePaths ) const;
private:
	bool BuildAlbum( const fs::CPath& searchPath );

	bool LoadCatalogStorage( const fs::CPath& docStgPath );
	bool SaveAsCatalogStorage( const fs::CPath& newDocStgPath );			// save .sld -> .ias, .ias -> .ias
	std::auto_ptr< CProgressService > MakeProgress( const TCHAR* pOperationLabel ) const;

	void PrepareToSave( const fs::CPath& docPath );
	bool PromptSaveConvertModelSchema( void ) const;

	void RegenerateModel( AlbumModelChange reason = AM_Init );

	bool UndoRedoCustomOrder( custom_order::COpStack& rFromStack, custom_order::COpStack& rToStack, bool isUndoOp );
	void ClearCustomOrder( custom_order::ClearMode clearMode = custom_order::CM_ClearAll );

	void OnAutoDropRecipientChanged( void );
private:
	CAlbumImageView* GetAlbumImageView( void ) const;
	CSlideData* GetActiveSlideData( void );
public:
	persist CSlideData m_slideData;						// always altered by CAlbumImageView::OnActivateView()
private:
	persist CAlbumModel m_model;						// image file list (search patterns + found files)
	persist COLORREF m_bkColor;							// album background color
	persist int m_docFlags;								// persistent document flags
	persist utl::Ternary m_smoothingMode;				// Slider_v5_7 (+)
	persist custom_order::COpStack m_customOrderUndoStack;	
	persist custom_order::COpStack m_customOrderRedoStack;
	persist auto_drop::COpStack m_dropUndoStack;		// contains the stack of performed move/copy operations to be undone on demand
	persist auto_drop::COpStack m_dropRedoStack;		// contains the stack of undone move/copy operations to be redone on demand
	persist std::auto_ptr< CImageState > m_pImageState;	// conditional on PresistImageState from v3.2+

	enum DocFlags
	{
		PresistImageState	= BIT_FLAG( 0 )
	};

	enum DocStatus { Clean, Dirty, DirtyOpening, DirtyMustRecreate };
public:
	// transient
	std::tstring m_password;							// allow password edititng of any document (including .sld), in preparation for SaveAs .ias
	auto_drop::CContext m_autoDropContext;				// contains the dropped files, used during an auto-drop operation

	// generated stuff
public:
	virtual void DeleteContents( void );				// delete doc items etc
	virtual BOOL OnOpenDocument( LPCTSTR pPathName );
	virtual BOOL OnSaveDocument( LPCTSTR pPathName );
protected:
	afx_msg void OnToggle_SmoothingMode( void );
	afx_msg void OnUpdate_SmoothingMode( CCmdUI* pCmdUI );
	afx_msg void OnExtractCatalog( void );
	afx_msg void OnUpdate_IsCatalogStorage( CCmdUI* pCmdUI );
	afx_msg void On_ImageSaveAs( void );
	afx_msg void OnUpdate_AnyCurrImage( CCmdUI* pCmdUI );
	afx_msg void OnUpdate_AllSelImagesRead( CCmdUI* pCmdUI );
	afx_msg void OnUpdate_AllSelImagesModify( CCmdUI* pCmdUI );
	afx_msg void On_ImageOpen( void );
	afx_msg void On_ImageDelete( void );
	afx_msg void On_ImageMove( void );

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
	afx_msg void OnEditCopyAlbumMap( void );
	afx_msg void OnUpdateEditCopyAlbumMap( CCmdUI* pCmdUI );
	afx_msg void OnToggleSaveCOUndoRedoBuffer( void );
	afx_msg void OnUpdateSaveCOUndoRedoBuffer( CCmdUI* pCmdUI );
	afx_msg void CmSelectAllThumbs( void );
	afx_msg void OnUpdateSelectAllThumbs( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // AlbumDoc_h
