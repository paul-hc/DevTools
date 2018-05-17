#ifndef UnitTest_h
#define UnitTest_h
#pragma once


#ifdef _DEBUG		// no UT code in release builds


namespace ut
{
	interface ITestCase
	{
		virtual void Run( void ) = 0;
	};

	abstract class CConsoleTestCase : public ITestCase
	{
	public:
		// base overrides
		virtual void Run( void ) = 0;		// pure with implementation; must be called for tracing execution
	};

	abstract class CGraphicTestCase : public ITestCase
	{
	public:
		// base overrides
		virtual void Run( void ) = 0;		// pure with implementation; must be called for tracing execution
	};


	class CTestSuite
	{
		~CTestSuite();
	public:
		static CTestSuite& Instance( void );

		void RunUnitTests( void );

		bool IsEmpty( void ) const { return m_testCases.empty(); }
		bool RegisterTestCase( ut::ITestCase* pTestCase );
	private:
		std::vector< ITestCase* > m_testCases;
	};
}


namespace ut
{
	const std::tstring& GetTestDataDirPath( void );

	std::tstring CombinePath( const std::tstring& parentDirPath, const TCHAR* pSubPath );
}


template< typename Type1, typename Type2 >
std::wostream& operator<<( std::wostream& os, const std::pair< Type1, Type2 >& rPair )
{
	return os << L"pair<" << rPair.first << L", " << rPair.second << L">";
}


namespace numeric
{
	const double dEpsilon = 0.000000001;

	bool DoublesEqual( double left, double right );
}


namespace ut
{
	template< class T >
	bool Equal( const T& x, const T& y )
	{
		return x == y;
	}

	template<>
	inline bool Equal< double >( const double& x, const double& y )
	{
		return numeric::DoublesEqual( x, y );
	}


	template< class T >
	inline std::tstring ToString( const T& x )
	{
		std::tostringstream os;
		os << x;
		return os.str();
	}


	std::tstring MakeNotEqualMessage( const std::tstring& expectedValue, const std::tstring& actualValue );


	template< typename ExpectedType, typename ActualType >
	bool AssertEquals( const ExpectedType& expected, const ActualType& actual, std::tstring& rMsg )
	{
		if ( Equal( static_cast< ActualType >( expected ), actual ) )
			return true;

		rMsg = MakeNotEqualMessage( ToString( static_cast< ActualType >( expected ) ), ToString( actual ) );
		TRACE( _T("%s\n"), rMsg.c_str() );
		return false;
	}


	template< typename ExpectedType, typename ActualType >
	bool AssertEqualsIgnoreCase( const ExpectedType& expected, const ActualType& actual, std::tstring& rMsg )
	{
		if ( str::EqualString< str::IgnoreCase >( static_cast< ActualType >( expected ), actual ) )
			return true;

		rMsg = MakeNotEqualMessage( ToString( static_cast< ActualType >( expected ) ), ToString( actual ) );
		TRACE( _T("%s\n"), rMsg.c_str() );
		return false;
	}
}


#define ASSERT_EQUAL( expected, actual )\
	do { std::tstring msg; _ASSERT_EXPR( ( ut::AssertEquals( (expected), (actual), msg ) ), msg.c_str() ); } while( false )


#define ASSERT_EQUAL_STR( pExpected, pActual )\
	do { std::tstring msg; _ASSERT_EXPR( ( ut::AssertEquals( (pExpected), std::tstring( (pActual) ), msg ) ), msg.c_str() ); } while( false )


#define ASSERT_EQUAL_IGNORECASE( expected, actual )\
	do { std::tstring msg; _ASSERT_EXPR( ( ut::AssertEqualsIgnoreCase( (expected), (actual), msg ) ), msg.c_str() ); } while( false )


namespace ut
{
	template< typename Container >
	std::string JoinKeys( const Container& items, const TCHAR sep[] )
	{
		std::ostringstream oss;
		size_t count = 0;
		for ( typename Container::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
		{
			if ( count++ != 0 )
				oss << sep;

			oss << itItem->first;
		}
		return oss.str();
	}

	template< typename Container >
	std::string JoinValues( const Container& items, const TCHAR sep[] )
	{
		std::ostringstream oss;
		size_t count = 0;
		for ( typename Container::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
		{
			if ( count++ != 0 )
				oss << sep;

			oss << itItem->second;
		}
		return oss.str();
	}
}


#include "utl/FileSystem.h"


namespace ut
{
	class CTempFilePool : private utl::noncopyable
	{
	public:
		CTempFilePool( const TCHAR* pFlatPaths = NULL );
		~CTempFilePool();

		bool IsValidDir( void ) const { return path::IsValid( m_tempDirPath ) && fs::IsValidDirectory( m_tempDirPath.c_str() ); }
		bool IsValidPool( void ) const { return IsValidDir() && !m_filePaths.empty() && !m_hasFileErrors; }
		const std::tstring& GetTempDirPath( void ) const { return m_tempDirPath; }
		const std::vector< std::tstring >& GetFilePaths( void ) const { return m_filePaths; }
		std::tstring QualifyPath( const TCHAR* pRelativePath ) const { return path::Combine( m_tempDirPath.c_str(), pRelativePath ); }

		bool DeleteAllFiles( void );
		bool SplitCreateFiles( const TCHAR* pFlatPaths = NULL );			// can contain subdirectories
	private:
		static bool CreateFile( const TCHAR* pFilePath );
	private:
		std::tstring m_tempDirPath;
		std::vector< std::tstring > m_filePaths;
		bool m_hasFileErrors;								// file creation errors
	public:
		static const TCHAR m_sep[];
	};


	// no physical files
	//
	struct CPathPairPool
	{
		CPathPairPool( void ) : m_fullDestPaths( false ) {}
		CPathPairPool( const TCHAR* pSourceFilenames, bool fullDestPaths = false );

		std::tstring JoinDest( void );
		void CopySrc( void );
	public:
		fs::TPathPairMap m_pathPairs;
		bool m_fullDestPaths;				// format DEST in JoinDest using full paths
	};


	// physical files managed in a temporary _UT directory
	//
	struct CTempFilePairPool : public CTempFilePool, public CPathPairPool
	{
		CTempFilePairPool( const TCHAR* pSourceFilenames );
	};


	inline std::tstring JoinFiles( const fs::CEnumerator& enumerator ) { return str::Join( enumerator.m_filePaths, ut::CTempFilePool::m_sep ); }
	inline std::tstring JoinSubDirs( const fs::CEnumerator& enumerator ) { return str::Join( enumerator.m_subDirPaths, ut::CTempFilePool::m_sep ); }

} //namespace ut


#endif //_DEBUG


#endif // UnitTest_h
