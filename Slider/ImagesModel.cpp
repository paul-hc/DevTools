
#include "stdafx.h"
#include "ImagesModel.h"
#include "FileAttr.h"
#include "ImageArchiveStg.h"
#include "utl/ContainerUtilities.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CImagesModel::CImagesModel( void )
{
}

CImagesModel::~CImagesModel()
{
	Clear();
}

void CImagesModel::Clear( void )
{
	utl::ClearOwningContainer( m_fileAttributes );

	ReleaseStorages();
	m_storagePaths.clear();
}

void CImagesModel::ReleaseStorages( void )
{
	CImageArchiveStg::Factory().ReleaseStorages( m_storagePaths );
}

void CImagesModel::Stream( CArchive& archive )
{
	serial::StreamOwningPtrs( archive, m_fileAttributes );
	serial::SerializeValues( archive, m_storagePaths );

	std::vector< int > displaySequence;				// display indexes that points to a baseline position in m_fileAttributes

	if ( archive.IsLoading() )
	{
		serial::SerializeValues( archive, displaySequence );

		if ( !displaySequence.empty() )
		{
			ASSERT( displaySequence.size() == m_fileAttributes.size() );

			// basically un-persist CFileAttr::m_baselinePos
			for ( size_t pos = 0; pos != m_fileAttributes.size(); ++pos )
				m_fileAttributes[ pos ]->StoreBaselinePos( displaySequence[ pos ] );
		}
		else
		{	// backwards compatibility: displaySequence was saved as CAlbumModel::m_customOrder, but only for m_fileOrder == CustomOrder
			StoreBaselineSequence();		// assume current positions in m_fileAttributes as BASELINE
		}
	}
	else
	{
		fattr::QueryDisplayIndexSequence( &displaySequence, m_fileAttributes );
		serial::SerializeValues( archive, displaySequence );		// basically persist CFileAttr::m_baselinePos
	}
}

void CImagesModel::StoreBaselineSequence( void )
{
	for ( size_t pos = 0; pos != m_fileAttributes.size(); ++pos )
		m_fileAttributes[ pos ]->StoreBaselinePos( pos );
}

const CFileAttr* CImagesModel::FindFileAttrWithPath( const fs::CPath& filePath ) const
{
	return fattr::FindWithPath( m_fileAttributes, filePath );
}

bool CImagesModel::AddFileAttr( CFileAttr* pFileAttr )
{
	ASSERT_PTR( pFileAttr );
	ASSERT( !utl::Contains( m_fileAttributes, pFileAttr ) );		// add once?

	if ( const CFileAttr* pFound = FindFileAttrWithPath( pFileAttr->GetPath() ) )
	{
		delete pFileAttr;
		return false;
	}

	m_fileAttributes.push_back( pFileAttr );
	return true;
}

bool CImagesModel::AddStoragePath( const fs::CPath& storagePath )
{
	return utl::AddUnique( m_storagePaths, storagePath );
}
