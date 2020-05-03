#ifndef CustomOrderUndoRedo_h
#define CustomOrderUndoRedo_h
#pragma once

#include <deque>
#include <vector>
#include "ArchivingModel.h"


namespace custom_order
{
	enum ClearMode { CM_ClearAll, CM_ClearReorder, CM_ClearArchiveImages };


	class COpStep
	{
	public:
		COpStep( void ) : m_fileOp( FOP_Resequence ), m_dropIndex( INT_MAX ), m_newDroppedIndex( INT_MAX ) {}

		void Stream( CArchive& archive );

		bool IsResequenceOperation( void ) const { return FOP_Resequence == m_fileOp; }
		bool IsArchivingOperation( void ) const { return FOP_FileCopy == m_fileOp || FOP_FileMove == m_fileOp; }

		const std::tstring& GetOperationTag( void ) const;

		CArchivingModel& RefArchivingModel( void ) { return m_archivingModel; }
		void StoreArchivingModel( const CArchivingModel& archivingModel ) { m_archivingModel = archivingModel; }
	public:
		persist FileOp m_fileOp;
		persist int m_dropIndex;						// original insertion point (drop dest index)
		persist int m_newDroppedIndex;					// destination index shifted after re-ordering
		persist std::vector< int > m_dragSelIndexes;	// selected indexes to be dropped
	private:
		persist CArchivingModel m_archivingModel;
	};


	struct COpStack : public std::deque< COpStep >, private utl::noncopyable
	{
		typedef std::deque< COpStep > Base;

		COpStack( void ) {}

		void ClearStack( ClearMode clearMode = CM_ClearAll );
	protected:
		using Base::clear;
	};

} //namespace custom_order


#endif // CustomOrderUndoRedo_h
