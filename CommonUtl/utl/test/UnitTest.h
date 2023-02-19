#ifndef UnitTest_h
#define UnitTest_h
#pragma once

#include "UnitTest_fwd.h"
#include "Test.h"
#include "utl/FileSystem_fwd.h"


#ifdef USE_UT		// no UT code in release builds


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
	bool Equals( const std::pair<T1, T2>& x, const std::pair<T1, T2>& y )
	{
		return
			Equals( x.first, y.first ) &&
			Equals( x.second, y.second );
	}

	template<>
	inline bool Equals<double>( const double& x, const double& y )
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


	namespace impl
	{
		template< typename CharT >
		std::basic_string<CharT> MakeString( const CharT* pText ) { return pText != nullptr ? std::basic_string<CharT>( pText ) : std::basic_string<CharT>(); }

		template< typename CharT >
		std::basic_string<CharT> MakePrefix( const CharT* pText, size_t prefixLen ) { return pText != nullptr ? std::basic_string<CharT>( pText, prefixLen ) : std::basic_string<CharT>(); }


		std::tstring MakeNotEqualMessage( const std::tstring& expectedValue, const std::tstring& actualValue, const wchar_t* pExpression );
		bool ReportMessage( bool succeeded, const std::tstring& msg, const char* pFilePath, int lineNumber );

		template< typename CharT >
		void TraceMessage( const CharT* pMessage );		// traces to debug output and std::clog (for console apps) - no '\n' written!
	}


	bool AssertTrue( bool succeeded, const wchar_t* pExpression, std::tstring& rMsg );

	template< typename ExpectedT, typename ActualT >
	bool AssertEquals( const ExpectedT& expected, const ActualT& actual, const wchar_t* pExpression, std::tstring& rMsg )
	{
		if ( Equals( static_cast<ActualT>( expected ), actual ) )
			return true;

		rMsg = impl::MakeNotEqualMessage( ToString( static_cast<ActualT>( expected ) ), ToString( actual ), pExpression );
		return false;
	}

	template< typename ExpectedT, typename ActualT >
	bool AssertEqualsIgnoreCase( const ExpectedT& expected, const ActualT& actual, const wchar_t* pExpression, std::tstring& rMsg )
	{
		if ( str::EqualString<str::IgnoreCase>( static_cast<ActualT>( expected ), actual ) )
			return true;

		rMsg = impl::MakeNotEqualMessage( ToString( static_cast<ActualT>( expected ) ), ToString( actual ), pExpression );
		return false;
	}

	template< typename ExpectedCharT, typename ActualCharT >
	bool AssertHasPrefix( const ExpectedCharT* pExpectedPrefix, const ActualCharT* pActual, const wchar_t* pExpression, std::tstring& rMsg )
	{
		return AssertEquals( pExpectedPrefix, impl::MakePrefix( pActual, str::GetLength( pExpectedPrefix ) ), pExpression, rMsg );
	}
}


// Unit Test assertions: assertion failures are traced in the output window, and printed to std::cerr.

#undef ASSERT
#define ASSERT( expr )\
	do { std::tstring msg; bool succeeded = ut::AssertTrue( !!(expr), _CRT_WIDE(#expr), msg ); ut::impl::ReportMessage( succeeded, msg, __FILE__, __LINE__ );\
	_ASSERT_EXPR( succeeded, _CRT_WIDE(#expr) ); } while( false )
	//
	// Note: include the "test/*header.h" last in .cpp file, if getting compilation errors due to conflicts with some MFC macros (e.g. AFX_ISOLATIONAWARE_PROC)


#define ASSERT_EQUAL( expected, actual )\
	do { std::tstring msg; bool succeeded = ut::AssertEquals( (expected), (actual), _CRT_WIDE(#actual), msg ); ut::impl::ReportMessage( succeeded, msg, __FILE__, __LINE__ );\
	_ASSERT_EXPR( succeeded, msg.c_str() ); } while( false )


#define ASSERT_EQUAL_STR( pExpected, pActual )\
	do { std::tstring msg; bool succeeded = ut::AssertEquals( (pExpected), ut::impl::MakeString( (pActual) ), _CRT_WIDE(#pActual), msg ); ut::impl::ReportMessage( succeeded, msg, __FILE__, __LINE__ );\
	_ASSERT_EXPR( succeeded, msg.c_str() ); } while( false )


#define ASSERT_EQUAL_IGNORECASE( expected, actual )\
	do { std::tstring msg; bool succeeded = ut::AssertEqualsIgnoreCase( (expected), (actual), _CRT_WIDE(#actual), msg ); ut::impl::ReportMessage( succeeded, msg, __FILE__, __LINE__ );\
	_ASSERT_EXPR( succeeded, msg.c_str() ); } while( false )


#define ASSERT_HAS_PREFIX( pExpectedPrefix, pActual )\
	do { std::tstring msg; bool succeeded = ut::AssertHasPrefix( (pExpectedPrefix), (pActual), _CRT_WIDE(#pActual), msg ); ut::impl::ReportMessage( succeeded, msg, __FILE__, __LINE__ );\
	_ASSERT_EXPR( succeeded, msg.c_str() ); } while( false )


#define UT_TRACE( pMessage )  ut::impl::TraceMessage( (pMessage) )

#define UT_REPEAT_BLOCK( count )  for ( unsigned int i = count; i-- != 0; )


namespace str
{
	// FWD:

	template< typename CharT, typename ValueT >
	bool ParseValue( ValueT& rValue, const std::basic_string<CharT>& text );
}


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
	std::basic_string<CharType> ShuffleSortJoin( std::vector<StringT>& rItems, const CharType* pSep, LessPred lessPred )
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
					if ( str::ParseValue( value, std::basic_string<CharType>( itItemStart, std::distance( itItemStart, itItemEnd ) ) ) )
						rItems.push_back( value );
					else
						ASSERT( false );

					itItemStart = itItemEnd + sepLen;
				}
				else
				{
					if ( str::ParseValue( value, std::basic_string<CharType>( itItemStart ) ) )
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
std::wostream& operator<<( std::wostream& os, const std::pair<Type1, Type2>& rPair )
{
	return os << L"pair<" << rPair.first << L", " << rPair.second << L">";
}


#include "utl/FileSystem.h"


namespace ut
{
	bool SetFileText( const fs::CPath& filePath, const TCHAR* pText = NULL );				// set a line of thext (pass NULL for using "name.ext")
	bool ModifyFileText( const fs::CPath& filePath, const TCHAR* pAddText = NULL, bool retainModifyTime = false );	// add another line of thext (pass NULL for using "name.ext")

	void StoreFileTextSize( const fs::CPath& filePath, size_t fileSize );


	enum { DefaultRowByteCount = 16 };

	void HexDump( std::ostream& os, const fs::CPath& textPath, size_t rowByteCount = DefaultRowByteCount ) throws_( CRuntimeException );		// dump file binary contents
}


class CLogger;
namespace fs { struct CPathEnumerator; }


namespace ut
{
	CLogger& GetTestLogger( void );

	const fs::TDirPath& GetTestDataDirPath( void ) throws_( CRuntimeException );	// %UTL_TESTDATA_PATH%
	const fs::TDirPath& GetImageSourceDirPath( void );								// %UTL_THUMB_SRC_IMAGE_PATH%
	const fs::TDirPath& GetDestImagesDirPath( void );								// %UTL_TESTDATA_PATH%\images
	const fs::TDirPath& GetStdImageDirPath( void );									// %UTL_TESTDATA_PATH%\std_test_images
	const fs::TDirPath& GetStdTestFilesDirPath( void );								// %UTL_TESTDATA_PATH%\std_test_files

	const fs::TDirPath& GetTempUt_DirPath( void ) throws_( CRuntimeException );
	fs::TDirPath MakeTempUt_DirPath( const fs::TDirPath& subDirPath, bool createDir ) throws_( CRuntimeException );


	class CTempFilePool : private utl::noncopyable
	{
	public:
		CTempFilePool( const TCHAR* pFlatPaths = NULL );
		~CTempFilePool();

		bool IsValidDir( void ) const { return path::IsValid( m_poolDirPath.Get() ) && fs::IsValidDirectory( m_poolDirPath.GetPtr() ); }
		bool IsValidPool( void ) const { return IsValidDir() && !m_filePaths.empty() && !m_hasFileErrors; }
		const fs::TDirPath& GetPoolDirPath( void ) const { return m_poolDirPath; }

		const std::vector<fs::CPath>& GetFilePaths( void ) const { return m_filePaths; }
		fs::CPath QualifyPath( const TCHAR* pRelativePath ) const { return m_poolDirPath / fs::CPath( pRelativePath ); }

		size_t SplitQualifyPaths( std::vector<fs::CPath>& rFullPaths, const TCHAR relFilePaths[] ) const;

		bool DeleteAllFiles( void );
		bool CreateFiles( const TCHAR* pFlatPaths = NULL );		// can contain subdirectories

		static fs::TDirPath MakePoolDirPath( bool createDir = false );
	private:
		fs::TDirPath m_poolDirPath;								// temorary directory
		std::vector<fs::CPath> m_filePaths;
		bool m_hasFileErrors;									// file creation errors
	public:
		static const TCHAR m_sep[];
	};


	std::tstring JoinFiles( const fs::CPathEnumerator& enumerator );
	std::tstring JoinSubDirs( const fs::CPathEnumerator& enumerator );

	// enumeration with relative paths
	size_t EnumFilePaths( std::vector<fs::CPath>& rFilePaths, const fs::TDirPath& dirPath, SortType sortType = SortAscending, const TCHAR* pWildSpec = _T("*"), fs::TEnumFlags flags = fs::EF_Recurse );
	size_t EnumSubDirPaths( std::vector<fs::TDirPath>& rSubDirPaths, const fs::TDirPath& dirPath, SortType sortType = SortAscending, fs::TEnumFlags flags = fs::EF_Recurse );

	std::tstring EnumJoinFiles( const fs::TDirPath& dirPath, SortType sortType = SortAscending, const TCHAR* pWildSpec = _T("*"), fs::TEnumFlags flags = fs::EF_Recurse );
	std::tstring EnumJoinSubDirs( const fs::TDirPath& dirPath, SortType sortType = SortAscending, fs::TEnumFlags flags = fs::EF_Recurse );

	fs::CPath FindFirstFile( const fs::TDirPath& dirPath, const TCHAR* pWildSpec = _T("*.*"), fs::TEnumFlags flags = fs::TEnumFlags() );		// returns relative path

} //namespace ut


#endif //USE_UT


#endif // UnitTest_h
