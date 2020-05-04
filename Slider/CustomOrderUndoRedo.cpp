
#include "stdafx.h"
#include "CustomOrderUndoRedo.h"
#include "ModelSchema.h"
#include "utl/EnumTags.h"
#include "utl/Serialization.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace custom_order
{
	// COpStep implementation

	void COpStep::Stream( CArchive& archive )
	{
		if ( archive.IsStoring() )
		{
			archive << (int)m_fileOp;
			archive << m_dropIndex;
			archive << m_newDroppedIndex;
		}
		else
		{
			archive >> (int&)m_fileOp;
			archive >> m_dropIndex;
			archive >> m_newDroppedIndex;

			enum { FOP_ResequenceOld = -1 };

			if ( FOP_ResequenceOld == (int)m_fileOp )
				m_fileOp = FOP_Resequence;					// backwards-compatibility
		}

		serial::SerializeValues( archive, m_dragSelIndexes );
		m_archivingModel.Stream( archive );

		if ( archive.IsLoading() )
		{
			app::ModelSchema docModelSchema = app::GetLoadingSchema( archive );
			if ( docModelSchema <= app::Slider_v4_2 )
			{
				// backwards-compatibility: m_newDroppedIndex used to be PAST dropped selection (currently is AT dropped selection)
				m_newDroppedIndex -= m_dragSelIndexes.size();
				ENSURE( m_newDroppedIndex >= 0 );
			}
		}
	}

	const std::tstring& COpStep::GetOperationTag( void ) const
	{
		return GetTags_FileOp().GetUiTags()[ m_fileOp ];
	}


	// COpStack implementation

	void COpStack::ClearStack( ClearMode clearMode /*= CM_ClearAll*/ )
	{
		if ( CM_ClearAll == clearMode )
			clear();
		else
		{	// clear selectively
			std::deque< COpStep >::iterator it = begin();

			while ( it != end() )
				if ( ( CM_ClearReorder == clearMode && it->IsResequenceOperation() ) ||
					 ( CM_ClearArchiveImages == clearMode && it->IsArchivingOperation() ) )
					it = erase( it );
				else
					++it;
		}
	}

} //namespace custom_order
