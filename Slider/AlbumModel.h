#ifndef AlbumModel_h
#define AlbumModel_h
#pragma once

#include "utl/FlexPath.h"
#include "ModelSchema.h"
#include "SearchModel.h"
#include "ImagesModel.h"


class CFileAttr;
namespace ui { interface IProgressService; }


class CAlbumModel
{
public:
	CAlbumModel( void );
	~CAlbumModel();

	void Stream( CArchive& archive );

	app::ModelSchema GetModelSchema( void ) const { return m_modelSchema; }
	void StoreModelSchema( app::ModelSchema modelSchema ) { m_modelSchema = modelSchema; }

	const CSearchModel* GetSearchModel( void ) const { return &m_searchModel; }
	CSearchModel* RefSearchModel( void ) { return &m_searchModel; }

	enum PersistFlag
	{
		AutoRegenerate		= BIT_FLAG( 0 ),		// auto re-generates after loading (de-serialization)
		UseDeepStreamPaths	= BIT_FLAG( 1 )			// in an archive doc m_fileAttributes has been encoded with flag wf::PrefixDeepStreamNames on
	};

	bool HasPersistFlag( PersistFlag persistFlag ) const { return HasFlag( m_persistFlags, persistFlag ); }
	void SetPersistFlag( PersistFlag persistFlag, bool on = true ) { SetFlag( m_persistFlags, persistFlag, on ); }

	bool SetupSearchPath( const fs::CPath& searchPath );
	void SearchForFiles( CWnd* pParentWnd, bool reportEmpty = true ) throws_( CException* );

	void CloseAssocImageArchiveStgs( void );

	bool MustAutoRegenerate( void ) const { return HasPersistFlag( AutoRegenerate ) || IsAutoDropRecipient(); }

	// auto-drop file reorder & rename
	bool IsAutoDropRecipient( bool checkValidPath = true ) const;		// single search pattern

	enum PersistOp { Loading, Saving };

	bool HasConsistentDeepStreams( void ) const;
	bool CheckReparentFileAttrs( const TCHAR* pDocPath, PersistOp op );					// if a stg doc path: reparent embedded image paths with current doc path (LOAD, SAVE AS)
public:
	// found image files
	const CImagesModel& GetImagesModel( void ) const { return m_imagesModel; }
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

	// custom order Drag & Drop
	bool DropCustomOrderIndexes( int& rDropIndex, std::vector< int >& rSelIndexes );
	bool UndropCustomOrderIndexes( int droppedIndex, const std::vector< int >& origDragSelIndexes );
private:
	bool DoOrderImagesModel( CImagesModel* pImagesModel, ui::IProgressService* pProgressSvc );
	bool ReparentStgFileAttrsImpl( const fs::CPath& stgDocPath, PersistOp op );			// post load/save: replace physical storage path with the current stg path
private:
	typedef int TPersistFlags;

	// transient
	app::ModelSchema m_modelSchema;					// model schema of this album document (persisted by the album document)
	fs::CPath m_stgDocPath;							// set only for image archive docs, replaces m_searchPatterns
private:
	// persistent
	persist CSearchModel m_searchModel;
	persist CImagesModel m_imagesModel;				// found image file attributes

	persist fattr::Order m_fileOrder;				// order of the image files
	persist int m_randomSeed;						// the value of the random seed currently used
	persist TPersistFlags m_persistFlags;			// additional persistent flags
};


namespace func
{
	struct StripStgDocPrefix
	{
		void operator()( fs::CPath& rPath ) const
		{
			const TCHAR* pSrcEmbedded = path::GetEmbedded( rPath.GetPtr() );		// works for both fs::CPath and fs::CFlexPath
			if ( !str::IsEmpty( pSrcEmbedded ) )
				rPath.Set( pSrcEmbedded );
		}

		void operator()( CFileAttr* pFileAttr ) const;
	};


	enum EmbeddedDepth { Flat, Deep };

	struct MakeComplexPath
	{
		MakeComplexPath( const fs::CPath& stgDocPath, EmbeddedDepth depth ) : m_stgDocPath( stgDocPath ), m_depth( depth ) {}

		void operator()( fs::CFlexPath& rPath ) const
		{
			rPath = fs::CFlexPath::MakeComplexPath( m_stgDocPath.Get(), Flat == m_depth ? rPath.GetNameExt() : rPath.GetPtr() );
		}

		void operator()( CFileAttr* pFileAttr ) const;
	private:
		fs::CPath m_stgDocPath;
		EmbeddedDepth m_depth;
	};
}


#endif // AlbumModel_h
