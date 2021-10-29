#ifndef FileStateItem_h
#define FileStateItem_h
#pragma once

#include "PathItemBase.h"
#include "FileState.h"


class CFileStateItem : public CPathItemBase
{
public:
	explicit CFileStateItem( const fs::CFileState& fileState );

	const fs::CFileState& GetState( void ) const { return m_fileState; }
	fs::CFileState& RefState( void ) { return m_fileState; }

	// base overrides
	virtual void SetFilePath( const fs::CPath& filePath );
protected:
	void Stream( CArchive& archive );
private:
	persist fs::CFileState m_fileState;
};


namespace func
{
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
