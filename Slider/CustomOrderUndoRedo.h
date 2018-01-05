#ifndef CustomOrderUndoRedo_h
#define CustomOrderUndoRedo_h
#pragma once

#include <deque>
#include <vector>
#include "utl/Serialization.h"
#include "ArchiveImagesContext.h"


namespace custom_order
{
	enum ClearMode { CM_ClearAll, CM_ClearReorder, CM_ClearArchiveImages };


	struct COpStep
	{
		COpStep( void ) : m_fileOp( FOP_Reorder ), m_toDestIndex( INT_MAX ), m_newDestIndex( INT_MAX ) {}

		void Stream( CArchive& archive );

		bool IsReorderOperation( void ) const { return FOP_Reorder == m_fileOp; }
		bool IsArchivingOperation( void ) const { return m_fileOp != FOP_Reorder; }

		const std::tstring& GetOperationTag( void ) const;
	public:
		persist FileOp m_fileOp;
		persist int m_toDestIndex;					// original insertion point (drop dest index)
		persist int m_newDestIndex;					// destination index shifted after re-ordering
		persist std::vector< int > m_toMoveIndexes;	// indexes to be moved
		persist CArchiveImagesContext m_archivedImages;
	};


	struct COpStack : public std::deque< COpStep >, private utl::noncopyable
	{
		typedef std::deque< COpStep > Base;

		COpStack( void ) {}

		void Stream( CArchive& archive ) { serial::StreamItems( archive, *this ); }

		void ClearStack( ClearMode clearMode = CM_ClearAll );
	protected:
		using Base::clear;
	};

} //namespace custom_order


#endif // CustomOrderUndoRedo_h
