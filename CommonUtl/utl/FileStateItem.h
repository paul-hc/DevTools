#ifndef FileStateItem_h
#define FileStateItem_h
#pragma once

#include "PathItemBase.h"
#include "FileState.h"


class CFileStateItem : public CPathItemBase
{
public:
	explicit CFileStateItem( const fs::CFileState& fileState );
	explicit CFileStateItem( const CFileFind& foundFile );

	const fs::CFileState& GetState( void ) const { return m_fileState; }
	fs::CFileState& RefState( void ) { return m_fileState; }

	// base overrides
	virtual void SetFilePath( const fs::CPath& filePath );		// also update m_fileState
protected:
	void Stream( CArchive& archive );
private:
	persist fs::CFileState m_fileState;
};


namespace func
{
	// accessors for order predicates

	struct AsFileSize
	{
		UINT64 operator()( const CFileStateItem* pItem ) const { return pItem->GetState().m_fileSize; }
	};

	struct AsCreationTime
	{
		const CTime& operator()( const CFileStateItem* pItem ) const { return pItem->GetState().m_creationTime; }
	};

	struct AsModifyTime
	{
		const CTime& operator()( const CFileStateItem* pItem ) const { return pItem->GetState().m_modifTime; }
	};

	struct AsAccessTime
	{
		const CTime& operator()( const CFileStateItem* pItem ) const { return pItem->GetState().m_accessTime; }
	};
}


#endif // FileStateItem_h
