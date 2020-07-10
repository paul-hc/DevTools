#ifndef AlbumModel_h
#define AlbumModel_h
#pragma once

#include "utl/FlexPath.h"
#include "ModelSchema.h"
#include "SearchModel.h"
#include "ImagesModel.h"
#include "CatalogStorageHost.h"


class CFileAttr;
namespace ui { interface IProgressService; }

typedef int TCurrImagePos;


class CAlbumModel
{
public:
	CAlbumModel( void );
	~CAlbumModel();

	void Clear( void );

	void Stream( CArchive& archive );

	app::ModelSchema GetModelSchema( void ) const { return m_modelSchema; }
	void StoreModelSchema( app::ModelSchema modelSchema ) { m_modelSchema = modelSchema; }

	void StoreCatalogDocPath( const fs::CPath& docStgPath );

	CCatalogStorageHost* GetStorageHost( void ) { return &m_storageHost; }
	ICatalogStorage* GetCatalogStorage( void ) const;			// opened storage if model is based on a catalog storage

	const CSearchModel* GetSearchModel( void ) const { return &m_searchModel; }
	CSearchModel* RefSearchModel( void ) { return &m_searchModel; }

	const CImagesModel& GetImagesModel( void ) const { return m_imagesModel; }
	CImagesModel& RefImagesModel( void ) { return m_imagesModel; }

	bool SetupSingleSearchPattern( CSearchPattern* pSearchPattern );
	void SearchForFiles( CWnd* pParentWnd, bool reportEmpty = true ) throws_( CException* );

	void OpenAllStorages( void );
	void CloseAllStorages( void );
	void QueryEmbeddedStorages( std::vector< fs::CPath >& rSubStoragePaths ) const;
public:
	enum PersistFlag
	{
		AutoRegenerate		= BIT_FLAG( 0 ),		// auto re-generates after loading (de-serialization)
		UseDeepStreamPaths	= BIT_FLAG( 1 )			// in an archive doc m_fileAttributes has been encoded with flag wf::PrefixDeepStreamNames on
	};

	bool HasPersistFlag( PersistFlag persistFlag ) const { return HasFlag( m_persistFlags, persistFlag ); }
	void SetPersistFlag( PersistFlag persistFlag, bool on = true ) { SetFlag( m_persistFlags, persistFlag, on ); }

	static bool ShouldUseDeepStreamPaths( void );

	bool MustAutoRegenerate( void ) const { return HasPersistFlag( AutoRegenerate ) || IsAutoDropRecipient(); }

	// auto-drop file reorder & rename
	bool IsAutoDropRecipient( bool checkValidPath = true ) const;		// single search pattern
public:
	// found image files
	void SwapFileAttrs( std::vector< CFileAttr* >& rFileAttributes ) { m_imagesModel.RefFileAttrs().swap( rFileAttributes ); }

	bool AnyFoundFiles( void ) const { return !m_imagesModel.IsEmpty(); }
	size_t GetFileAttrCount( void ) const { return m_imagesModel.GetFileAttrs().size(); }
	const CFileAttr* GetFileAttr( size_t pos ) const { return m_imagesModel.GetFileAttrAt( pos ); }

	void QueryFileAttrsSequence( std::vector< CFileAttr* >& rSequence, const std::vector< int >& selIndexes ) const;

	int FindIndexFileAttrWithPath( const fs::CPath& filePath ) const;

	// image files order
	fattr::Order GetFileOrder( void ) const { return m_fileOrder; }
	bool ModifyFileOrder( fattr::Order fileOrder );				// stores and reorders images
	void StoreFileOrder( fattr::Order fileOrder ) { m_fileOrder = fileOrder; }

	// custom order
	bool IsCustomOrder( void ) const { return fattr::CustomOrder == GetFileOrder(); }
	void SetCustomOrderSequence( const std::vector< CFileAttr* >& customSequence );			// explicit custom order
public:
	/** SERVICE API **/

	// image file operations
	TCurrImagePos DeleteFromAlbum( const std::vector< fs::CFlexPath >& selFilePaths );

	// custom order Drag & Drop
	bool DropCustomOrderIndexes( int& rDropIndex, std::vector< int >& rSelIndexes );
	bool UndropCustomOrderIndexes( int droppedIndex, const std::vector< int >& origDragSelIndexes );
private:
	bool DoOrderImagesModel( CImagesModel* pImagesModel, ui::IProgressService* pProgressSvc );
private:
	// transient
	app::ModelSchema m_modelSchema;					// model schema of this album document (persisted by the album document)
	fs::CPath m_docStgPath;							// set only for image catalog docs, it replaces m_searchPatterns
	CCatalogStorageHost m_storageHost;				// for both album and single image documents that can open embedded images
private:
	typedef int TPersistFlags;

	// persistent
	persist CSearchModel m_searchModel;
	persist CImagesModel m_imagesModel;				// found image file attributes

	persist fattr::Order m_fileOrder;				// order of the image files
	persist int m_randomSeed;						// the value of the random seed currently used
	persist TPersistFlags m_persistFlags;			// additional persistent flags
};


#endif // AlbumModel_h
