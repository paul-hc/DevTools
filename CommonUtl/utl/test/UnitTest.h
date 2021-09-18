#ifndef UnitTest_h
#define UnitTest_h
#pragma once

#include "Test.h"


#ifdef _DEBUG		// no UT code in release builds


namespace numeric
{
	const double dEpsilon = 0.000000001;

	bool DoublesEqual( double left, double right );
}


namespace ut
{
	template< typename T >
	bool Equals( const T& x, const T& y )
	{
		return x == y;
	}

	template< typename T1, typename T2 >
	bool Equals( const std::pair< T1, T2 >& x, const std::pair< T1, T2 >& y )
	{
		return
			Equals( x.first, y.first ) &&
			Equals( x.second, y.second );
	}

	template<>
	inline bool Equals< double >( const double& x, const double& y )
	{
		return numeric::DoublesEqual( x, y );
	}


	template< typename T >
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
		if ( Equals( static_cast< ActualType >( expected ), actual ) )
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


#define UT_REPEAT_BLOCK( count )  for ( unsigned int i = count; i-- != 0; )


namespace ut
{
	template< typename PtrContainerT >
	std::string JoinPtrs( const PtrContainerT& itemPtrs, const TCHAR sep[] )
	{
		std::ostringstream oss;
		size_t count = 0;
		for ( typename PtrContainerT::const_iterator itItem = itemPtrs.begin(); itItem != itemPtrs.end(); ++itItem )
		{
			if ( count++ != 0 )
				oss << sep;

			oss << **itItem;
		}
		return oss.str();
	}

	template< typename ContainerT >
	std::string JoinKeys( const ContainerT& items, const TCHAR sep[] )
	{
		std::ostringstream oss;
		size_t count = 0;
		for ( typename ContainerT::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
		{
			if ( count++ != 0 )
				oss << sep;

			oss << itItem->first;
		}
		return oss.str();
	}

	template< typename ContainerT >
	std::string JoinValues( const ContainerT& items, const TCHAR sep[] )
	{
		std::ostringstream oss;
		size_t count = 0;
		for ( typename ContainerT::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
		{
			if ( count++ != 0 )
				oss << sep;

			oss << itItem->second;
		}
		return oss.str();
	}

	template< typename CharType, typename StringT, typename LessPred >
	std::basic_string< CharType > ShuffleSortJoin( std::vector< StringT >& rItems, const CharType* pSep, LessPred lessPred )
	{
		std::random_shuffle( rItems.begin(), rItems.end() );
		std::sort( rItems.begin(), rItems.end(), lessPred );
		return str::Join( rItems, pSep );
	}


	template< typename CharType, typename ContainerT >
	void SplitValues( ContainerT& rItems, const CharType* pSource, const CharType* pSep, bool append = false )
	{
		ASSERT( !str::IsEmpty( pSep ) );
		if ( !append )
			rItems.clear();

		if ( !str::IsEmpty( pSource ) )
		{
			const size_t sepLen = str::GetLength( pSep );
			typedef const CharType* const_iterator;
			typename ContainerT::value_type value;

			for ( const_iterator itItemStart = str::begin( pSource ), itEnd = str::end( pSource ); ; )
			{
				const_iterator itItemEnd = std::search( itItemStart, itEnd, pSep, pSep + sepLen );
				if ( itItemEnd != itEnd )
				{
					if ( str::ParseValue( value, std::basic_string< CharType >( itItemStart, std::distance( itItemStart, itItemEnd ) ) ) )
						rItems.push_back( value );
					else
						ASSERT( false );

					itItemStart = itItemEnd + sepLen;
				}
				else
				{
					if ( str::ParseValue( value, std::basic_string< CharType >( itItemStart ) ) )
						rItems.push_back( value );
					else
						ASSERT( false );

					break;			// last item
				}
			}
		}
	}

	template< typename CharType, typename ContainerT >
	inline void PushSplitValues( ContainerT& rItems, const CharType* pSource, const CharType* pSep ) { SplitValues( rItems, pSource, pSep, true ); }
}


template< typename Type1, typename Type2 >
std::wostream& operator<<( std::wostream& os, const std::pair< Type1, Type2 >& rPair )
{
	return os << L"pair<" << rPair.first << L", " << rPair.second << L">";
}


#include "utl/FileSystem.h"


namespace ut
{
	bool SetFileText( const fs::CPath& filePath, const TCHAR* pText = NULL );				// set a line of thext (pass NULL for using "name.ext")
	bool ModifyFileText( const fs::CPath& filePath, const TCHAR* pAddText = NULL, bool retainModifyTime = false );	// add another line of thext (pass NULL for using "name.ext")
}


class CLogger;
namespace fs { struct CEnumerator; }


namespace ut
{
	CLogger& GetTestLogger( void );

	const fs::CPath& GetTestDataDirPath( void ) throws_( CRuntimeException );		// %UTL_TESTDATA_PATH%
	const fs::CPath& GetImageSourceDirPath( void );									// %UTL_THUMB_SRC_IMAGE_PATH%
	const fs::CPath& GetDestImagesDirPath( void );									// %UTL_TESTDATA_PATH%\images
	const fs::CPath& GetStdImageDirPath( void );									// %UTL_TESTDATA_PATH%\std_test_images
	const fs::CPath& GetStdTestFilesDirPath( void );								// %UTL_TESTDATA_PATH%\std_test_files

	const fs::CPath& GetTempUt_DirPath( void ) throws_( CRuntimeException );
	fs::CPath MakeTempUt_DirPath( const fs::CPath& subPath, bool createDir ) throws_( CRuntimeException );


	class CTempFilePool : private utl::noncopyable
	{
	public:
		CTempFilePool( const TCHAR* pFlatPaths = NULL );
		~CTempFilePool();

		bool IsValidDir( void ) const { return path::IsValid( m_poolDirPath.Get() ) && fs::IsValidDirectory( m_poolDirPath.GetPtr() ); }
		bool IsValidPool( void ) const { return IsValidDir() && !m_filePaths.empty() && !m_hasFileErrors; }
		const fs::CPath& GetPoolDirPath( void ) const { return m_poolDirPath; }

		const std::vector< fs::CPath >& GetFilePaths( void ) const { return m_filePaths; }
		fs::CPath QualifyPath( const TCHAR* pRelativePath ) const { return m_poolDirPath / fs::CPath( pRelativePath ); }

		size_t SplitQualifyPaths( std::vector< fs::CPath >& rFullPaths, const TCHAR relFilePaths[] ) const;

		bool DeleteAllFiles( void );
		bool CreateFiles( const TCHAR* pFlatPaths = NULL );		// can contain subdirectories

		static fs::CPath MakePoolDirPath( bool createDir = false );
	private:
		fs::CPath m_poolDirPath;								// temorary directory
		std::vector< fs::CPath > m_filePaths;
		bool m_hasFileErrors;									// file creation errors
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


	std::tstring JoinFiles( const fs::CEnumerator& enumerator );
	std::tstring JoinSubDirs( const fs::CEnumerator& enumerator );

	// enumeration with relative paths
	size_t EnumFilePaths( std::vector< fs::CPath >& rFilePaths, const fs::CPath& dirPath, SortType sortType = SortAscending, const TCHAR* pWildSpec = _T("*"), RecursionDepth depth = Deep );
	size_t EnumSubDirPaths( std::vector< fs::CPath >& rSubDirPaths, const fs::CPath& dirPath, SortType sortType = SortAscending, RecursionDepth depth = Deep );

	std::tstring EnumJoinFiles( const fs::CPath& dirPath, SortType sortType = SortAscending, const TCHAR* pWildSpec = _T("*"), RecursionDepth depth = Deep );
	std::tstring EnumJoinSubDirs( const fs::CPath& dirPath, SortType sortType = SortAscending, RecursionDepth depth = Deep );

	fs::CPath FindFirstFile( const fs::CPath& dirPath, const TCHAR* pWildSpec = _T("*.*"), RecursionDepth depth = Shallow );		// returns relative path

} //namespace ut


#endif //_DEBUG


#endif // UnitTest_h
