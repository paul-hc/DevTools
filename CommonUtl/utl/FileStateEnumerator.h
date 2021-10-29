#ifndef FileStateEnumerator_h
#define FileStateEnumerator_h
#pragma once

#include "FileEnumerator.h"
#include "FileState.h"


namespace fs
{
	// file states and sub-dirs in existing file-system order
	//
	struct CFileStateEnumerator : public IEnumerator, private utl::noncopyable
	{
		CFileStateEnumerator( IEnumerator* pChainEnum = NULL ) : m_pChainEnum( pChainEnum ), m_maxFiles( utl::npos ) {}

		bool IsEmpty( void ) const { return m_fileStates.empty() && m_subDirPaths.empty(); }
		void Clear( void );

		void SetMaxFiles( size_t maxFiles ) { ASSERT( IsEmpty() ); m_maxFiles = maxFiles; }

		// post-search
		void SortFileStates( void ) { fs::SortPaths( m_fileStates ); }
		size_t UniquifyAll( void );
	protected:
		// IEnumerator interface
		virtual void OnAddFileInfo( const CFileFind& foundFile );
		virtual void AddFoundFile( const TCHAR* pFilePath );
		virtual bool AddFoundSubDir( const TCHAR* pSubDirPath );
		virtual bool MustStop( void ) const;
	protected:
		IEnumerator* m_pChainEnum;				// allows chaining for progress reporting
		size_t m_maxFiles;
	public:
		std::vector< fs::CFileState > m_fileStates;
		std::vector< fs::CPath > m_subDirPaths;
	};
}


class CFileStateItem;


namespace fs
{
	// Enumerates CFileStateItem objects, and sub-dirs from the base class.
	// FileStateItemT may be a subclass of either CFileStateItem, or any other class that defines a constructor of FileStateItemT( const CFileFind& ).
	//
	template< typename FileStateItemT >
	struct CFileStateItemEnumerator : public CFileStateEnumerator
	{
		CFileStateItemEnumerator( IEnumerator* pChainEnum = NULL ) : CFileStateEnumerator( pChainEnum ) {}

		void Clear( void );

		void SortItems( void ) { std::sort( m_fileItems.begin(), m_fileItems.end(), pred::TLess_PathItem() ); }		// sort by path key
	protected:
		// IEnumerator interface overrides
		virtual void OnAddFileInfo( const CFileFind& foundFile );
	private:
		// hidden base data-member and methods
		using CFileStateEnumerator::m_fileStates;
		using CFileStateEnumerator::SortFileStates;
	public:
		std::vector< FileStateItemT* > m_fileItems;
	};
}


#endif // FileStateEnumerator_h
