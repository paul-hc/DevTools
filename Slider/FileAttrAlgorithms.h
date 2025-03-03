#ifndef FileAttrAlgorithms_h
#define FileAttrAlgorithms_h
#pragma once

#include "utl/ComparePredicates.h"
#include "FileAttr.h"
#include <unordered_map>


namespace fattr
{
	size_t FindPosWithPath( const std::vector<CFileAttr*>& fileAttributes, const fs::CPath& filePath );

	inline const CFileAttr* FindWithPath( const std::vector<CFileAttr*>& fileAttributes, const fs::CPath& filePath )
	{
		size_t foundPos = FindPosWithPath( fileAttributes, filePath );
		return foundPos != utl::npos ? fileAttributes[ foundPos ] : nullptr;
	}

	template< typename FileAttrT >
	void StoreBaselineSequence( std::vector<FileAttrT*>& rFileAttributes )
	{
		for ( size_t pos = 0; pos != rFileAttributes.size(); ++pos )
			rFileAttributes[ pos ]->StoreBaselinePos( pos );
	}

	template< typename IndexT >
	void QueryDisplayIndexSequence( std::vector<IndexT>* pDisplaySequence, const std::vector<CFileAttr*>& fileAttributes )
	{	// pDisplaySequence contains baseline positions in current order
		ASSERT_PTR( pDisplaySequence );

		pDisplaySequence->resize( fileAttributes.size() );

		for ( size_t pos = 0; pos != fileAttributes.size(); ++pos )
			pDisplaySequence->at( pos ) = static_cast<IndexT>( fileAttributes[ pos ]->GetBaselinePos() );
	}


	// To restore the existing file order, after searching again for files. New image found are appended.
	//
	class CRetainFileOrder
	{
	public:
		CRetainFileOrder( const std::vector<CFileAttr*>& origSequence );

		void RestoreOriginalOrder( std::vector<CFileAttr*>* pNewSequence );
	private:
		// original order
		std::unordered_map<fs::CFlexPath, size_t> m_pathToOrigPosMap;
	};
}


namespace func
{
	struct ToFilePath
	{
		const fs::CFlexPath& operator()( const CFileAttr* pFileAttr ) const
		{
			return pFileAttr->GetPath();
		}
	};

	struct ToFileSize
	{
		UINT operator()( const CFileAttr* pFileAttr ) const
		{
			ASSERT_PTR( pFileAttr );
			return pFileAttr->GetFileSize();
		}
	};

	struct ToImageArea
	{
		size_t operator()( const CFileAttr* pFileAttr ) const
		{
			ASSERT_PTR( pFileAttr );
			CSize imageDim = pFileAttr->GetImageDim();
			if ( imageDim.cx < 0 || imageDim.cy < 0 )		// error accessing the image file
				return 0;

			return static_cast<size_t>( imageDim.cx ) * static_cast<size_t>( imageDim.cy );
		}
	};

	struct ToImageWidth
	{
		int operator()( const CFileAttr* pFileAttr ) const
		{
			ASSERT_PTR( pFileAttr );
			int imageWidth = pFileAttr->GetImageDim().cx;
			return imageWidth >= 0 ? imageWidth : 0;
		}
	};

	struct ToImageHeight
	{
		int operator()( const CFileAttr* pFileAttr ) const
		{
			ASSERT_PTR( pFileAttr );
			int imageHeight = pFileAttr->GetImageDim().cy;
			return imageHeight >= 0 ? imageHeight : 0;
		}
	};

	struct ToBaselinePos
	{
		size_t operator()( const CFileAttr* pFileAttr ) const
		{
			ASSERT_PTR( pFileAttr );
			return pFileAttr->GetBaselinePos();
		}
	};


	struct AddFileSize
	{
		UINT64 operator()( UINT64 totalFileSize, const CFileAttr* pRight ) const
		{
			ASSERT_PTR( pRight );
			return totalFileSize + pRight->GetFileSize();
		}
	};
}


namespace pred
{
	typedef CompareAdapter<CompareNaturalPath, func::ToFilePath> TCompareFileAttrPath;

	typedef CompareAdapter<CompareValue, func::ToFileSize> TCompareFileAttrSize;

	typedef CompareAdapter<CompareValue, func::ToImageArea> TCompareImageArea;
	typedef CompareAdapter<CompareValue, func::ToImageWidth> TCompareImageWidth;
	typedef CompareAdapter<CompareValue, func::ToImageHeight> TCompareImageHeight;
	
	typedef JoinCompare< TCompareImageArea, JoinCompare<TCompareImageWidth, TCompareImageHeight> > TCompareImageDimensions;		// area | width | height
	typedef JoinCompare<TCompareImageDimensions, TCompareFileAttrPath> TCompare_ImageDimensions;								// area | width | height | path

	typedef CompareAdapter<CompareValue, func::ToBaselinePos> TCompareBaselinePos;


	inline CompareResult CompareFileTime( const FILETIME& left, const FILETIME& right )
	{
		CompareResult result = Compare_Scalar( left.dwHighDateTime, right.dwHighDateTime );
		if ( Equal == result )
			result = Compare_Scalar( left.dwLowDateTime, right.dwLowDateTime );
		return result;
	}


	struct CompareFileAttr
	{
		CompareFileAttr( fattr::Order fileOrder, bool compareImageDim = false );

		pred::CompareResult operator()( const CFileAttr* pLeft, const CFileAttr* pRight ) const;
	public:
		fattr::Order m_fileOrder;
		bool m_ascendingOrder;
		bool m_compareImageDim;
		bool m_useSecondaryComparison;
	};
}


namespace func
{
	// CAlbumModel functors

	struct RefFilePath
	{
		fs::CFlexPath& operator()( CFileAttr* pFileAttr )
		{
			ASSERT_PTR( pFileAttr ); 
			return pFileAttr->RefPath();
		}
	};


	struct NormalizeComplexPath
	{
		void operator()( fs::CPath& rPath )
		{
			path::NormalizeComplexPath( rPath.Ref() );
		}
	};


	enum EmbeddedDepth { Flat, Deep };

	struct MakeComplexPath
	{
		MakeComplexPath( const fs::TStgDocPath& stgDocPath, EmbeddedDepth depth ) : m_stgDocPath( stgDocPath ), m_depth( depth ) {}

		void operator()( fs::CFlexPath& rPath ) const
		{
			rPath = fs::CFlexPath::MakeComplexPath( m_stgDocPath, Flat == m_depth ? rPath.GetFilename() : rPath.Get() );
		}

		void operator()( CFileAttr* pFileAttr ) const { ASSERT_PTR( pFileAttr ); operator()( pFileAttr->RefPath() ); }
	private:
		fs::TStgDocPath m_stgDocPath;
		EmbeddedDepth m_depth;
	};
}


#endif // FileAttrAlgorithms_h
