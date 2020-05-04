
#include "stdafx.h"
#include "FileAttrAlgorithms.h"
#include "utl/ContainerUtilities.h"
#include "utl/EnumTags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fattr
{
	const CEnumTags& GetTags_Order( void )
	{
		static const CEnumTags s_tags
		(
			_T("Original order|User-defined order|Random shuffle|Random shuffle on same seed|")
			_T("File Name|(File Name)|Folder Name|(Folder Name)|File Size|(File Size)|")
			_T("File Date|(File Date)|Image Dimensions|(Image Dimensions)|")
			_T("Images with same file size|Images with same file size and dimensions|Corrupted files")
		);
		return s_tags;
	}


	size_t FindPosWithPath( const std::vector< CFileAttr* >& fileAttributes, const fs::CPath& filePath )
	{
		for ( size_t pos = 0; pos != fileAttributes.size(); ++pos )
			if ( fileAttributes[ pos ]->GetPath() == filePath )
				return pos;

		return utl::npos;
	}


	// CRetainFileOrder class

	CRetainFileOrder::CRetainFileOrder( const std::vector< CFileAttr* >& origSequence )
	{
		// backup the original mapping between file-paths and their positions
		for ( size_t pos = 0; pos != origSequence.size(); ++pos )
		{
			const fs::CFlexPath& filePath = origSequence[ pos ]->GetPath();

			m_pathToOrigPosMap[ filePath ] = pos;			// add file-path in original order
		}
	}

	void CRetainFileOrder::RestoreOriginalOrder( std::vector< CFileAttr* >* pNewSequence )
	{
		ASSERT_PTR( pNewSequence );
		size_t seqCount = pNewSequence->size();

		// restore what's left of the original order, and append new files at end

		std::vector< CFileAttr* > origFileAttributes;
		origFileAttributes.resize( m_pathToOrigPosMap.size() );			// initially all NULLs

		std::vector< CFileAttr* > newlyFoundFileAttributes;				// to be appended at the end

		for ( std::vector< CFileAttr* >::const_iterator itFileAttr = pNewSequence->begin(); itFileAttr != pNewSequence->end(); ++itFileAttr )
			if ( size_t* pOrigPos = utl::FindValuePtr( m_pathToOrigPosMap, ( *itFileAttr )->GetPath() ) )
				origFileAttributes[ *pOrigPos ] = *itFileAttr;			// place it at the original position
			else
				newlyFoundFileAttributes.push_back( *itFileAttr );

		// remove original NULLs: entries not found in the new sequence
		origFileAttributes.erase( std::remove( origFileAttributes.begin(), origFileAttributes.end(), (CFileAttr*)NULL ), origFileAttributes.end() );

		pNewSequence->clear();
		pNewSequence->assign( origFileAttributes.begin(), origFileAttributes.end() );										// assign remaining originals
		pNewSequence->insert( pNewSequence->end(), newlyFoundFileAttributes.begin(), newlyFoundFileAttributes.end() );		// append newly found

		ENSURE( seqCount == pNewSequence->size() );
	}

} //namespace fattr


namespace pred
{
	// CompareFileAttr implementation

	CompareFileAttr::CompareFileAttr( fattr::Order fileOrder, bool compareImageDim /*= false*/ )
		: m_fileOrder( fileOrder )
		, m_ascendingOrder( true )
		, m_compareImageDim( compareImageDim )
		, m_useSecondaryComparison( true )
	{
		switch ( m_fileOrder )
		{
			case fattr::ByFileNameDesc:
			case fattr::ByFullPathDesc:
			case fattr::BySizeDesc:
			case fattr::ByDateDesc:
			case fattr::ByDimensionDesc:
				m_ascendingOrder = false;
		}
	}

	CompareResult CompareFileAttr::operator()( const CFileAttr* pLeft, const CFileAttr* pRight ) const
	{
		static const std::tstring s_empty;

		CompareResult result = Equal;

		switch ( m_fileOrder )
		{
			case fattr::ByFullPathAsc:
			case fattr::ByFullPathDesc:
				result = CompareFileAttrPath()( pLeft, pRight );
				break;
			case fattr::ByFileNameAsc:
			case fattr::ByFileNameDesc:
				result = TCompareNameExt()( pLeft->GetPath(), pRight->GetPath() );
				break;
			case fattr::BySizeAsc:
			case fattr::BySizeDesc:
			case fattr::FilterFileSameSize:
			case fattr::FilterFileSameSizeAndDim:
				result = CompareFileAttrSize()( pLeft, pRight );
				if ( Equal == result )
					if ( fattr::FilterFileSameSizeAndDim == m_fileOrder && m_compareImageDim )
						result = CompareImageDimensions()( pLeft, pRight );

				// also ordonate by filepath (if secondary comparison not disabled)
				if ( result == Equal && m_useSecondaryComparison )
					result = CompareFileAttrPath()( pLeft, pRight );
				break;
			case fattr::ByDateAsc:
			case fattr::ByDateDesc:
				result = CompareFileTime( pLeft->GetLastModifTime(), pRight->GetLastModifTime() );
				break;
			case fattr::ByDimensionAsc:
			case fattr::ByDimensionDesc:
				result = Compare_ImageDimensions()( pLeft, pRight );
				break;
			default:
				ASSERT( false );
		}

		return GetResultInOrder( result, m_ascendingOrder );
	}
}
