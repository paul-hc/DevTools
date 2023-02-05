#ifndef FileStateEnumerator_h
#define FileStateEnumerator_h
#pragma once

#include "FileEnumerator.h"
#include "FileState.h"


namespace fs
{
	// file states and sub-dirs in existing file-system order
	//
	struct CFileStateEnumerator : public CBaseEnumerator
	{
		CFileStateEnumerator( fs::TEnumFlags enumFlags = fs::TEnumFlags(), IEnumerator* pChainEnum = NULL ) : CBaseEnumerator( enumFlags, pChainEnum ) {}

		// base overrides
		virtual void Clear( void ) { m_fileStates.clear(); __super::Clear(); }
		virtual size_t GetFileCount( void ) const { return m_fileStates.size(); }

		// post-search
		void SortFileStates( void ) { fs::SortPaths( m_fileStates ); }
	protected:
		// IEnumerator interface
		virtual void OnAddFileInfo( const fs::CFileState& fileState );
		virtual void AddFoundFile( const fs::CPath& filePath ) { __super::AddFoundFile( filePath ); }		// base method is pure & implemented
	public:
		std::vector<fs::CFileState> m_fileStates;
	};
}


namespace func
{
	template< typename FileStateItemT >
	struct CreateFoundItem
	{
		FileStateItemT* operator()( const fs::CFileState& fileState ) const
		{
			return new FileStateItemT( fileState );
		}
	};
}


class CFileStateItem;


namespace fs
{
	// Enumerates CFileStateItem objects, and sub-dirs from the base class.
	// FileStateItemT may be a subclass of either CFileStateItem, or any other class that defines a constructor of FileStateItemT( const CFileFind& ).
	//
	template< typename FileStateItemT = CFileStateItem, typename CreateFuncT = func::CreateFoundItem<FileStateItemT> >
	class CFileStateItemEnumerator : public CFileStateEnumerator
	{
	public:
		CFileStateItemEnumerator( fs::TEnumFlags enumFlags = fs::TEnumFlags(), IEnumerator* pChainEnum = NULL, CreateFuncT createFunc = CreateFuncT() )
			: CFileStateEnumerator( enumFlags, pChainEnum )
			, m_createFunc( createFunc )
		{
		}

		~CFileStateItemEnumerator() { Clear(); }

		// base overrides
		virtual void Clear( void );
		virtual size_t GetFileCount( void ) const { return m_fileStates.size(); }

		void SortItems( void ); 		// sort by path key
	protected:
		// IEnumerator interface overrides
		virtual void OnAddFileInfo( const fs::CFileState& fileState );
	private:
		CreateFuncT m_createFunc;

		// hidden base data-member and methods
		using CFileStateEnumerator::m_fileStates;
		using CFileStateEnumerator::SortFileStates;
	public:
		std::vector<FileStateItemT*> m_fileItems;
	};
}


#endif // FileStateEnumerator_h
