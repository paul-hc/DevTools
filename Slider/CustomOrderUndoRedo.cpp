
#include "stdafx.h"
#include "CustomOrderUndoRedo.h"
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
			archive << m_toDestIndex;
			archive << m_newDestIndex;
		}
		else
		{
			archive >> (int&)m_fileOp;
			archive >> m_toDestIndex;
			archive >> m_newDestIndex;

			enum { FOP_ReorderOld = -1 };

			if ( FOP_ReorderOld == (int)m_fileOp )
				m_fileOp = FOP_Reorder;						// backwards compat
		}

		serial::SerializeValues( archive, m_toMoveIndexes );
		m_archivedImages.Stream( archive );
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
				if ( ( CM_ClearReorder == clearMode && it->IsReorderOperation() ) ||
					 ( CM_ClearArchiveImages == clearMode && it->IsArchivingOperation() ) )
					it = erase( it );
				else
					++it;
		}
	}

} //namespace custom_order
