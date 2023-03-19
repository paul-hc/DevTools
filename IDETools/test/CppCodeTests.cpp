
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "utl/StringUtilities.h"
#include "test/CppCodeTests.h"
#include "CompoundTextParser.h"
#include "IterationSlices.h"
#include "CppParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/Language.hxx"
#include "CppParser.hxx"


namespace ut
{
	void ParseString( CCompoundTextParser* pParser, const std::string& text ) throws_( CRuntimeException )
	{
		ASSERT_PTR( pParser );
		std::istringstream is( text );

		pParser->ParseStream( is );
	}
}


CCppCodeTests::CCppCodeTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CCppCodeTests& CCppCodeTests::Instance( void )
{
	static CCppCodeTests s_testCase;
	return s_testCase;
}

void CCppCodeTests::TestCompoundTextParser( void )
{
	CCompoundTextParser parser( _T("\n") );

	{	// inline section content
		ut::ParseString( &parser, "\
ignore\n\
[[S1]]This <<CS2>> some <<CS3>>.[[EOS]]\n\
[[CS2]]IS[[EOS]]\n\
[[CS3]]TEXT[[EOS]][[CS4]]ignore[[EOS]]"
);
		ASSERT_EQUAL_SWAP( parser.ExpandSection( _T("S1") ), _T("This IS some TEXT.") );
	}

	{	// 1 line section content
		ut::ParseString( &parser, "\
[[S1]]\n\
Line1\n\
[[EOS]]"
);
		ASSERT_EQUAL_SWAP( parser.ExpandSection( _T("S1") ),
						   _T("\
Line1\n\
") );

		ut::ParseString( &parser, "\
[[S1]]\n\
Line1\n\
Line2[[EOS]]"
);
		ASSERT_EQUAL_SWAP( parser.ExpandSection( _T("S1") ),
						   _T("\
Line1\n\
Line2\
") );
	}

	{	// 2 lines section content
		ut::ParseString( &parser, "\
[[S1]]\n\
Line1\n\
Line2\n\
[[EOS]]"
);
		ASSERT_EQUAL_SWAP( parser.ExpandSection( _T("S1") ),
						   _T("\
Line1\n\
Line2\n\
") );
	}

	{	// 3 lines section content
		ut::ParseString( &parser, "\
[[S1]]\n\
Line1\n\
Line2\n\
\n\
[[EOS]]"
);
		ASSERT_EQUAL_SWAP( parser.ExpandSection( _T("S1") ),
						   _T("\
Line1\n\
Line2\n\
\n\
") );
	}

	{	// nested section multi-line section content
		ut::ParseString( &parser, "\
[[S1]]\n\
Line1\n\
<<LINE2>>\n\
[[EOS]]\n\
\n\
[[LINE2]]\n\
Line2\n\
[[EOS]]\
");
		ASSERT_EQUAL_SWAP( parser.ExpandSection( _T("S1") ),
						   _T("\
Line1\n\
Line2\n\
\n\
") );
	}

	{	// nested section multi-line section content using '$' suffix to eat line-end
		ut::ParseString( &parser, "\
[[S1]]\n\
Line1\n\
<<LINE2>>$\n\
[[EOS]]\n\
\n\
[[LINE2]]\n\
Line2\n\
[[EOS]]\
");
		ASSERT_EQUAL_SWAP( parser.ExpandSection( _T("S1") ),
						   _T("\
Line1\n\
Line2\n\
") );	// '\n' was eaten!
	}

	{	// multi-line section content
		ut::ParseString( &parser, "\
[[S1]]\n\
Line1[[EOS]]\
");
		ASSERT_EQUAL_SWAP( parser.ExpandSection( _T("S1") ),
						   _T("\
Line1\
") );
		ut::ParseString( &parser, "\
[[S1]]\n\
Line1\n\
[[EOS]]\
");
		ASSERT_EQUAL_SWAP( parser.ExpandSection( _T("S1") ),
						   _T("\
Line1\n\
") );

		ut::ParseString( &parser, "\
[[S1]]\n\
\n\
Line1\n\
[[EOS]]\
");
		ASSERT_EQUAL_SWAP( parser.ExpandSection( _T("S1") ),
						   _T("\
\n\
Line1\n\
") );
	}
}

void CCppCodeTests::TestIterationSlices( void )
{
	code::CIterationSlices slices;
	std::tstring codeText;

	// MFC containers:
	{	// CArray (MFC)
		codeText = _T("\tCArray< const char* > myArray;\n");

		slices.ParseCode( codeText );
		ASSERT_EQUAL( _T("CArray< const char* >"), slices.m_containerType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("myArray"), slices.m_containerName.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("MyArray"), slices.m_iteratorName );
		ASSERT_EQUAL( _T("const char*"), slices.m_valueType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("\t"), slices.m_leadingWhiteSpace.MakeToken( codeText ) );
		ASSERT_EQUAL_STR( _T("."), slices.m_pObjSelOp );
		ASSERT( code::CIterationSlices::MFC == slices.m_libraryType );
	}
	{	// CObList (MFC)
		codeText = _T("\tCObList< CNmxObject* > myList;\n");

		slices.ParseCode( codeText );
		ASSERT_EQUAL( _T("CObList< CNmxObject* >"), slices.m_containerType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("myList"), slices.m_containerName.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("MyList"), slices.m_iteratorName );
		ASSERT_EQUAL( _T("CNmxObject*"), slices.m_valueType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("\t"), slices.m_leadingWhiteSpace.MakeToken( codeText ) );
		ASSERT_EQUAL_STR( _T("."), slices.m_pObjSelOp );
		ASSERT( code::CIterationSlices::MFC == slices.m_libraryType );
	}
	{	// CList (MFC)
		codeText = _T("\t  CList<CFile::CStatus> statusList;\n");

		slices.ParseCode( codeText );
		ASSERT_EQUAL( _T("CList<CFile::CStatus>"), slices.m_containerType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("statusList"), slices.m_containerName.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("StatusList"), slices.m_iteratorName );
		ASSERT_EQUAL( _T("CFile::CStatus"), slices.m_valueType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("\t  "), slices.m_leadingWhiteSpace.MakeToken( codeText ) );
		ASSERT_EQUAL_STR( _T("."), slices.m_pObjSelOp );
		ASSERT( code::CIterationSlices::MFC == slices.m_libraryType );
	}

	// STL containers:
	{	// std::vector
		codeText = _T("\tstd::vector< CString >rItems\n");

		slices.ParseCode( codeText );
		ASSERT_EQUAL( _T("std::vector< CString >"), slices.m_containerType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("rItems"), slices.m_containerName.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("Item"), slices.m_iteratorName );
		ASSERT_EQUAL( _T("CString"), slices.m_valueType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("\t"), slices.m_leadingWhiteSpace.MakeToken( codeText ) );
		ASSERT_EQUAL_STR( _T("."), slices.m_pObjSelOp );
		ASSERT( code::CIterationSlices::STL == slices.m_libraryType );
	}
	{	// std::vector
		codeText = _T("\tstd::vector< CString >rItems\n");

		slices.ParseCode( codeText );
		ASSERT_EQUAL( _T("std::vector< CString >"), slices.m_containerType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("rItems"), slices.m_containerName.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("Item"), slices.m_iteratorName );
		ASSERT_EQUAL( _T("CString"), slices.m_valueType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("\t"), slices.m_leadingWhiteSpace.MakeToken( codeText ) );
		ASSERT_EQUAL_STR( _T("."), slices.m_pObjSelOp );
		ASSERT( code::CIterationSlices::STL == slices.m_libraryType );
	}
	{	// std::vector
		codeText = _T("\tconst std::vector< CString >& rItems;\n");

		slices.ParseCode( codeText );
		ASSERT_EQUAL( _T("std::vector< CString >"), slices.m_containerType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("rItems"), slices.m_containerName.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("Item"), slices.m_iteratorName );
		ASSERT_EQUAL( _T("CString"), slices.m_valueType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("\t"), slices.m_leadingWhiteSpace.MakeToken( codeText ) );
		ASSERT_EQUAL_STR( _T("."), slices.m_pObjSelOp );
		ASSERT( code::CIterationSlices::STL == slices.m_libraryType );
		ASSERT( slices.m_isConst );
	}
	{	// std::vector
		codeText = _T("\tstd::vector< CString >* pItems;\n");

		slices.ParseCode( codeText );
		ASSERT_EQUAL( _T("std::vector< CString >"), slices.m_containerType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("pItems"), slices.m_containerName.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("Item"), slices.m_iteratorName );
		ASSERT_EQUAL( _T("CString"), slices.m_valueType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("\t"), slices.m_leadingWhiteSpace.MakeToken( codeText ) );
		ASSERT_EQUAL_STR( _T("->"), slices.m_pObjSelOp );
		ASSERT( code::CIterationSlices::STL == slices.m_libraryType );
	}
	{	// std::vector
		codeText = _T("\tstd::vector< CString >&const rItems\n");

		slices.ParseCode( codeText );
		ASSERT_EQUAL( _T("std::vector< CString >"), slices.m_containerType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("rItems"), slices.m_containerName.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("Item"), slices.m_iteratorName );
		ASSERT_EQUAL( _T("CString"), slices.m_valueType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("\t"), slices.m_leadingWhiteSpace.MakeToken( codeText ) );
		ASSERT_EQUAL_STR( _T("."), slices.m_pObjSelOp );
		ASSERT( code::CIterationSlices::STL == slices.m_libraryType );
		ASSERT( slices.m_isConst );
	}
	{	// std::vector
		codeText = _T("\tstd::vector< CString > & const rItems,;\n");

		slices.ParseCode( codeText );
		ASSERT_EQUAL( _T("std::vector< CString >"), slices.m_containerType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("rItems"), slices.m_containerName.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("Item"), slices.m_iteratorName );
		ASSERT_EQUAL( _T("CString"), slices.m_valueType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("\t"), slices.m_leadingWhiteSpace.MakeToken( codeText ) );
		ASSERT_EQUAL_STR( _T("."), slices.m_pObjSelOp );
		ASSERT( code::CIterationSlices::STL == slices.m_libraryType );
		ASSERT( slices.m_isConst );
	}
	{	// std::vector
		codeText = _T("\tconst std::vector<CString>* rObject.GetParent()->GetTags<TCHAR>() rDummy\n");

		slices.ParseCode( codeText );
		ASSERT_EQUAL( _T("std::vector<CString>"), slices.m_containerType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("rObject.GetParent()->GetTags<TCHAR>()"), slices.m_containerName.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("Tag"), slices.m_iteratorName );
		ASSERT_EQUAL( _T("CString"), slices.m_valueType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("\t"), slices.m_leadingWhiteSpace.MakeToken( codeText ) );
		ASSERT_EQUAL_STR( _T("->"), slices.m_pObjSelOp );
		ASSERT( code::CIterationSlices::STL == slices.m_libraryType );
		ASSERT( slices.m_isConst );
	}

	{	// std::map
		codeText = _T("\tstd::map< MyObject*, Class2& > &* myMap;\n");

		slices.ParseCode( codeText );
		ASSERT_EQUAL( _T("std::map< MyObject*, Class2& >"), slices.m_containerType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("myMap"), slices.m_containerName.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("MyMap"), slices.m_iteratorName );
		ASSERT_EQUAL( _T("MyObject*, Class2&"), slices.m_valueType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("\t"), slices.m_leadingWhiteSpace.MakeToken( codeText ) );
		ASSERT_EQUAL_STR( _T("."), slices.m_pObjSelOp );
		ASSERT( code::CIterationSlices::STL == slices.m_libraryType );
		ASSERT( !slices.m_isConst );
	}
	{	// std::map
		codeText = _T("\tconst std::map< MyObject*, std::pair<CPoint*, Depth> >& myMap ;\n");

		slices.ParseCode( codeText );
		ASSERT_EQUAL( _T("std::map< MyObject*, std::pair<CPoint*, Depth> >"), slices.m_containerType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("myMap"), slices.m_containerName.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("MyMap"), slices.m_iteratorName );
		ASSERT_EQUAL( _T("MyObject*, std::pair<CPoint*, Depth>"), slices.m_valueType.MakeToken( codeText ) );
		ASSERT_EQUAL( _T("\t"), slices.m_leadingWhiteSpace.MakeToken( codeText ) );
		ASSERT_EQUAL_STR( _T("."), slices.m_pObjSelOp );
		ASSERT( code::CIterationSlices::STL == slices.m_libraryType );
		ASSERT( slices.m_isConst );
	}
}

void CCppCodeTests::TestResolveDefaultParams( void )
{
	const CCppParser cppParser;

	{	// simple
		std::string proto = "\tstd::pair<int, bool> Func( UINT pos, size_t pos /*= utl::npos*/, int depth = 2 + 3, std::wstring const& text = L\"END\", const char delim = '|' );";

		ASSERT_EQUAL( "\tstd::pair<int, bool> Func( UINT pos, size_t pos /*= utl::npos*/, int depth /*= 2 + 3*/, std::wstring const& text /*= L\"END\"*/, const char delim /*= '|'*/ );",
					  cppParser.MakeRemoveDefaultParams( proto, true ) );		// comment-out default parameter values

		ASSERT_EQUAL( "\tstd::pair<int, bool> Func( UINT pos, size_t pos /*= utl::npos*/, int depth, std::wstring const& text, const char delim );",
					  cppParser.MakeRemoveDefaultParams( proto, false ) );		// remove default parameter values
	}
	{	// operator, no trailing ';'
		std::string proto = "\tinline CArchive& operator<<( CArchive& archive, const std::pair<FirstT, SecondT>& srcPair = std::make_pair( FirstT(), SecondT() ) )";

		ASSERT_EQUAL( "\tinline CArchive& operator<<( CArchive& archive, const std::pair<FirstT, SecondT>& srcPair /*= std::make_pair( FirstT(), SecondT() )*/ )",
					  cppParser.MakeRemoveDefaultParams( proto, true ) );		// comment-out default parameter values

		ASSERT_EQUAL( "\tinline CArchive& operator<<( CArchive& archive, const std::pair<FirstT, SecondT>& srcPair )",
					  cppParser.MakeRemoveDefaultParams( proto, false ) );		// remove default parameter values
	}
	{	// break protected words
		std::string proto = "char Filter( const char** ppSrc = defPtr() IN OUT, size_t* pLength = nullptr OUT )";

		ASSERT_EQUAL( "char Filter( const char** ppSrc /*= defPtr()*/ IN OUT, size_t* pLength /*= nullptr*/ OUT )",
					  cppParser.MakeRemoveDefaultParams( proto, true ) );		// comment-out default parameter values

		ASSERT_EQUAL( "char Filter( const char** ppSrc _in_out_, size_t* pLength _out_ )",
					  cppParser.MakeRemoveDefaultParams( proto, false ) );		// remove default parameter values

		proto = "char Filter( const char** ppSrc = defPtr() _in_out_, size_t* pLength = nullptr _out_ )";
		ASSERT_EQUAL( "char Filter( const char** ppSrc /*= defPtr()*/ _in_out_, size_t* pLength /*= nullptr*/ _out_ )",
					  cppParser.MakeRemoveDefaultParams( proto, true ) );		// comment-out default parameter values

		ASSERT_EQUAL( "char Filter( const char** ppSrc _in_out_, size_t* pLength _out_ )",
					  cppParser.MakeRemoveDefaultParams( proto, false ) );		// remove default parameter values
	}
	{	// more involved cast expressions, unicode strings
		std::tstring proto = _T("\tstd::pair<int, bool> Func( int depth = 5, TCHAR* pAtom = (TCHAR*)(const TCHAR*)str::GetEmpty().c_str(), const fs::CPath& item = _T(\"END\") );");

		ASSERT_EQUAL( _T("\tstd::pair<int, bool> Func( int depth /*= 5*/, TCHAR* pAtom /*= (TCHAR*)(const TCHAR*)str::GetEmpty().c_str()*/, const fs::CPath& item /*= _T(\"END\")*/ );"),
					  cppParser.MakeRemoveDefaultParams( proto, true ) );		// comment-out default parameter values

		ASSERT_EQUAL( _T("\tstd::pair<int, bool> Func( int depth, TCHAR* pAtom, const fs::CPath& item );"),
					  cppParser.MakeRemoveDefaultParams( proto, false ) );		// remove default parameter values
	}
}

void CCppCodeTests::TestExtractTemplateInstance( void )
{
	const CCppParser cppParser;
	{
		std::string templateDecl;

		templateDecl = "template< typename Type, class MyClass, struct MyStruct, enum MyEnum = utl::GetEnum(), int level = 5 >";
		ASSERT_EQUAL( "<Type, MyClass, MyStruct, MyEnum, level>",
					  cppParser.ExtractTemplateInstance( templateDecl, code::TrimSpace ) );

		templateDecl = "template< typename Type, class MyClass, struct MyStruct, enum MyEnum = utl::GetEnum(), int level = 5 >";
		ASSERT_EQUAL( "< Type, MyClass, MyStruct, MyEnum, level >",
					  cppParser.ExtractTemplateInstance( templateDecl, code::AddSpace ) );

		templateDecl = "template<\t  typename Type, class MyClass, struct MyStruct, enum MyEnum = utl::GetEnum(), int level = 5>";
		ASSERT_EQUAL( "< Type, MyClass, MyStruct, MyEnum, level >",
					  cppParser.ExtractTemplateInstance( templateDecl, code::AddSpace ) );

		templateDecl = "template<\t  typename Type, class MyClass, struct MyStruct, enum MyEnum = utl::GetEnum(), int level = 5>";
		ASSERT_EQUAL( "<\t  Type, MyClass, MyStruct, MyEnum, level>",
					  cppParser.ExtractTemplateInstance( templateDecl, code::RetainSpace ) );

		// simple with non-type parameters
		templateDecl = "template< str::CaseType caseType, size_t size, typename CharT >";
		ASSERT_EQUAL( "<caseType, size, CharT>",
					  cppParser.ExtractTemplateInstance( templateDecl, code::TrimSpace ) );
	}
}


void CCppCodeTests::Run( void )
{
	RUN_TEST( TestCompoundTextParser );
	RUN_TEST( TestIterationSlices );
	RUN_TEST( TestResolveDefaultParams );
	RUN_TEST( TestExtractTemplateInstance );
}


#endif //USE_UT
