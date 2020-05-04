#ifndef FileAttrAlgorithms_h
#define FileAttrAlgorithms_h
#pragma once

#include "utl/ComparePredicates.h"
#include "FileAttr.h"
#include <hash_map>


namespace fattr
{
	size_t FindPosWithPath( const std::vector< CFileAttr* >& fileAttributes, const fs::CPath& filePath );

	inline const CFileAttr* FindWithPath( const std::vector< CFileAttr* >& fileAttributes, const fs::CPath& filePath )
	{
		size_t foundPos = FindPosWithPath( fileAttributes, filePath );
		return foundPos != utl::npos ? fileAttributes[ foundPos ] : NULL;
	}

	template< typename IndexT >
	void QueryDisplayIndexSequence( std::vector< IndexT >* pDisplaySequence, const std::vector< CFileAttr* >& fileAttributes )
	{	// pDisplaySequence contains baseline positions in current order
		ASSERT_PTR( pDisplaySequence );

		pDisplaySequence->resize( fileAttributes.size() );

		for ( size_t pos = 0; pos != fileAttributes.size(); ++pos )
			pDisplaySequence->at( pos ) = fileAttributes[ pos ]->GetBaselinePos();
	}


	// Provide the ability to restore the existing file order, after searching again for files:
	//	 - if new image files are found, they are appended at the end.
	//
	class CRetainFileOrder
	{
	public:
		CRetainFileOrder( const std::vector< CFileAttr* >& origSequence );

		void RestoreOriginalOrder( std::vector< CFileAttr* >* pNewSequence );
	private:
		// original order
		stdext::hash_map< fs::CFlexPath, size_t > m_pathToOrigPosMap;
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

			return static_cast< size_t >( imageDim.cx ) * static_cast< size_t >( imageDim.cy );
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
		int operator()( const CFileAttr* pFileAttr ) const
		{
			ASSERT_PTR( pFileAttr );
			return pFileAttr->GetBaselinePos();
		}
	};


	struct AddFileSize
	{
		size_t operator()( size_t totalFileSize, const CFileAttr* pRight ) const
		{
			ASSERT_PTR( pRight );
			return totalFileSize + pRight->GetFileSize();
		}
	};
}


namespace pred
{
	typedef CompareAdapter< CompareNaturalPath, func::ToFilePath > CompareFileAttrPath;

	typedef CompareAdapter< CompareValue, func::ToFileSize > CompareFileAttrSize;

	typedef CompareAdapter< CompareValue, func::ToImageArea > CompareImageArea;
	typedef CompareAdapter< CompareValue, func::ToImageWidth > CompareImageWidth;
	typedef CompareAdapter< CompareValue, func::ToImageHeight > CompareImageHeight;
	
	typedef JoinCompare< CompareImageArea, JoinCompare< CompareImageWidth, CompareImageHeight > > CompareImageDimensions;		// area | width | height
	typedef JoinCompare< CompareImageDimensions, CompareFileAttrPath > Compare_ImageDimensions;									// area | width | height | path

	typedef CompareAdapter< CompareValue, func::ToBaselinePos > CompareBaselinePos;


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


#endif // FileAttrAlgorithms_h
