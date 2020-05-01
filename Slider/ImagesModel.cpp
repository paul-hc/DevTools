
#include "stdafx.h"
#include "ImagesModel.h"
#include "FileAttr.h"
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
	ClearFileAttrs();
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
		fattr::QueryDisplaySequence( &displaySequence, m_fileAttributes );
		serial::SerializeValues( archive, displaySequence );		// basically persist CFileAttr::m_baselinePos
	}
}

void CImagesModel::ClearFileAttrs( void )
{
	utl::ClearOwningContainer( m_fileAttributes );
}

void CImagesModel::StoreBaselineSequence( void )
{
	for ( size_t pos = 0; pos != m_fileAttributes.size(); ++pos )
		m_fileAttributes[ pos ]->StoreBaselinePos( pos );
}
