#ifndef AlbumModel_h
#define AlbumModel_h
#pragma once

#include "utl/FlexPath.h"
#include "ModelSchema.h"
#include "SearchModel.h"
#include "FileAttr.h"


class CEnumTags;
namespace app { class CScopedProgress; }


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
		AutoRegenerate = BIT_FLAG( 0 ),			// auto re-generates after loading (de-serialization)
		UseDeepStreamPaths = BIT_FLAG( 1 )		// in an archive doc m_fileAttributes has been encoded with flag wf::PrefixDeepStreamNames on
	};

	bool HasPersistFlag( PersistFlag persistFlag ) const { return HasFlag( m_persistFlags, persistFlag ); }
	void SetPersistFlag( PersistFlag persistFlag, bool on = true ) { SetFlag( m_persistFlags, persistFlag, on ); }

	bool SetupSearchPath( const fs::CPath& searchPath );
	void SearchForFiles( bool reportEmpty = true ) throws_( CException* );

	bool InGeneration( void ) const { return m_inGeneration; }
	bool CancelGeneration( void );
	void CloseAssocImageArchiveStgs( void );

	bool MustAutoRegenerate( void ) const { return HasPersistFlag( AutoRegenerate ) || IsAutoDropRecipient(); }

	// auto-drop file reorder & rename
	bool IsAutoDropRecipient( bool checkValidPath = true ) const;		// single search spec
private:
	void ClearArchiveStgPaths( void );

	bool OrderFileList( void );
	bool FilterFileDuplicates( bool compareImageDim = false );
	size_t FilterCorruptFiles( void );
public:
	enum Order;

	// file array access
	Order GetFileOrder( void ) const { return m_fileOrder; }
	void SetFileOrder( Order fileOrder );

	// custom order
	bool IsCustomOrder( void ) const { return CustomOrder == GetFileOrder(); }
	void SetCustomOrder( const std::vector< CFileAttr* >& customOrder );			// explicit custom order
	bool MoveCustomOrderIndexes( int& rToDestIndex, std::vector< int >& rToMoveIndexes );
	bool MoveBackCustomOrderIndexes( int newDestIndex, const std::vector< int >& rToMoveIndexes );

	bool AnyFoundFiles( void ) const { return !m_fileAttributes.empty(); }
	size_t GetFileAttrCount( void ) const { return m_fileAttributes.size(); }
	const CFileAttr& GetFileAttr( size_t displayIndex ) const { return const_cast< CFileAttr& >( m_fileAttributes[ DisplayToTrueFileIndex( displayIndex ) ] ); }
	void QueryFileAttrs( std::vector< CFileAttr* >& rFileAttrs ) const;

	int FindFileAttr( const fs::CFlexPath& filePath, bool wantDisplayIndex = true ) const;
	void FetchFilePathsFromIndexes( std::vector< fs::CPath >& rFilePaths, const std::vector< int >& displayIndexes ) const;

	enum PersistOp { Loading, Saving };

	bool HasConsistentDeepStreams( void ) const;
	bool CheckReparentFileAttrs( const TCHAR* pDocPath, PersistOp op );					// if a stg doc path: reparent embedded image paths with current doc path (LOAD, SAVE AS)
private:
	bool ReparentStgFileAttrsImpl( const fs::CPath& stgDocPath, PersistOp op );			// post load/save: replace physical storage path with the current stg path

	size_t FindPosFileAttr( const CFileAttr* pFileAttr ) const;

	// display index is:
	//	- non-custom order: same with true index, that is index into m_fileAttributes vector
	//	- custom order: index into m_customOrder vector that points to the true index (in m_fileAttributes vector)
	int DisplayToTrueFileIndex( size_t displayIndex ) const;
public:
	enum Order
	{
		OriginalOrder,			// don't order files at all
		CustomOrder,			// custom order -> user-defined order through UI
		Shuffle,				// use different random seed on each generation process
		ShuffleSameSeed,		// use previous generated random seed for suffling
		ByFileNameAsc,
		ByFileNameDesc,
		ByFullPathAsc,
		ByFullPathDesc,
		BySizeAsc,
		BySizeDesc,
		ByDateAsc,
		ByDateDesc,
		ByDimensionAsc,
		ByDimensionDesc,
		FileSameSize,			// special filter for file duplicates based on file size
		FileSameSizeAndDim,		// special filter for file duplicates based on file size and dimensions
		CorruptedFiles			// special filter for detecting files with errors (same sort effect as OriginalOrder)
	};

	static const CEnumTags& GetTags_Order( void );
private:
	typedef int TPersistFlags;

	// transient
	app::ModelSchema m_modelSchema;							// model schema of this album document (persisted by the album document)
	fs::CPath m_stgDocPath;									// set only for image archive docs, replaces m_searchSpecs
	bool m_inGeneration;
private:
	// persistent
	persist CSearchModel m_searchModel;

	persist Order m_fileOrder;								// order of the generated files
	persist int m_randomSeed;								// the value of the random seed currently used
	persist TPersistFlags m_persistFlags;					// additional persistent flags

	// found files
	persist std::vector< CFileAttr > m_fileAttributes;		// found file attributes
	persist std::vector< fs::CPath > m_archiveStgPaths;		// compound files found during the search (for automatic release of storages)
	persist std::vector< int > m_customOrder;				// display indexes that points to a true index in m_fileAttributes
};


namespace pred
{
	inline CompareResult CompareFileTime( const FILETIME& left, const FILETIME& right )
	{
		CompareResult result = Compare_Scalar( left.dwHighDateTime, right.dwHighDateTime );
		if ( Equal == result )
			result = Compare_Scalar( left.dwLowDateTime, right.dwLowDateTime );
		return result;
	}


	struct CompareFileAttr
	{
		CompareFileAttr( CAlbumModel::Order fileOrder, bool compareImageDim = false, app::CScopedProgress* pProgress = NULL )
			: m_fileOrder( fileOrder )
			, m_ascendingOrder( true )
			, m_compareImageDim( compareImageDim )
			, m_useSecondaryComparison( true )
			, m_pProgress( pProgress )
		{
			switch ( m_fileOrder )
			{
				case CAlbumModel::ByFileNameDesc:
				case CAlbumModel::ByFullPathDesc:
				case CAlbumModel::BySizeDesc:
				case CAlbumModel::ByDateDesc:
				case CAlbumModel::ByDimensionDesc:
					m_ascendingOrder = false;
			}
		}

		pred::CompareResult operator()( const CFileAttr& left, const CFileAttr& right ) const;
	public:
		CAlbumModel::Order m_fileOrder;
		bool m_ascendingOrder;
		bool m_compareImageDim;
		bool m_useSecondaryComparison;
	private:
		app::CScopedProgress* m_pProgress;
	};
}


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

		void operator()( CFileAttr& rFileAttr ) const { operator()( rFileAttr.RefPath() ); }
	};


	enum EmbeddedDepth { Flat, Deep };

	struct MakeComplexPath
	{
		MakeComplexPath( const fs::CPath& stgDocPath, EmbeddedDepth depth ) : m_stgDocPath( stgDocPath ), m_depth( depth ) {}

		void operator()( fs::CFlexPath& rPath ) const
		{
			rPath = fs::CFlexPath::MakeComplexPath( m_stgDocPath.Get(), Flat == m_depth ? rPath.GetNameExt() : rPath.GetPtr() );
		}

		void operator()( CFileAttr& rFileAttr ) const { operator()( rFileAttr.RefPath() ); }
	private:
		fs::CPath m_stgDocPath;
		EmbeddedDepth m_depth;
	};
}


#endif // AlbumModel_h
