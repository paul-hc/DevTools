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
		size_t UniquifyAll( void );
	protected:
		// IEnumerator interface
		virtual void OnAddFileInfo( const CFileFind& foundFile );
		virtual void AddFoundFile( const TCHAR* pFilePath ) { __super::AddFoundFile( pFilePath ); }		// base method is pure & implemented
	public:
		std::vector< fs::CFileState > m_fileStates;
	};
}


class CFileStateItem;


namespace func
{
	template< typename FileStateItemT >
	struct CreateFoundItem
	{
		FileStateItemT* operator()( const CFileFind& foundFile ) const
		{
			return new FileStateItemT( foundFile );
		}
	};
}


namespace fs
{
	// Enumerates CFileStateItem objects, and sub-dirs from the base class.
	// FileStateItemT may be a subclass of either CFileStateItem, or any other class that defines a constructor of FileStateItemT( const CFileFind& ).
	//
	template< typename FileStateItemT = CFileStateItem, typename CreateFuncT = func::CreateFoundItem<FileStateItemT> >
	struct CFileStateItemEnumerator : public CFileStateEnumerator
	{
		CFileStateItemEnumerator( fs::TEnumFlags enumFlags = fs::TEnumFlags(), CreateFuncT createFunc = CreateFuncT(), IEnumerator* pChainEnum = NULL )
			: CFileStateEnumerator( enumFlags, pChainEnum )
			, m_createFunc( createFunc )
		{
		}

		~CFileStateItemEnumerator() { Clear(); }

		// base overrides
		virtual void Clear( void );
		virtual size_t GetFileCount( void ) const { return m_fileStates.size(); }

		void SortItems( void ); 		// sort by path key
		size_t UniquifyAll( void );
	protected:
		// IEnumerator interface overrides
		virtual void OnAddFileInfo( const CFileFind& foundFile );
	private:
		CreateFuncT m_createFunc;

		// hidden base data-member and methods
		using CFileStateEnumerator::m_fileStates;
		using CFileStateEnumerator::SortFileStates;
		using CFileStateEnumerator::UniquifyAll;
	public:
		std::vector< FileStateItemT* > m_fileItems;
	};
}


#endif // FileStateEnumerator_h
