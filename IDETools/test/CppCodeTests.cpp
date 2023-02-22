
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "utl/StringUtilities.h"
#include "test/CppCodeTests.h"
#include "IterationSlices.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CCppCodeTests::CCppCodeTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CCppCodeTests& CCppCodeTests::Instance( void )
{
	static CCppCodeTests s_testCase;
	return s_testCase;
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
	{	// CObjectList (MFC)
		codeText = _T("\tCObjectList< CNmxObject* > myList;\n");

		slices.ParseCode( codeText );
		ASSERT_EQUAL( _T("CObjectList< CNmxObject* >"), slices.m_containerType.MakeToken( codeText ) );
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


void CCppCodeTests::Run( void )
{
	RUN_TEST( TestIterationSlices );
}


#endif //USE_UT
