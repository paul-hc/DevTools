
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "utl/StringUtilities.h"
#include "MethodPrototype.h"
#include "test/MethodPrototypeTests.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CMethodPrototypeTests::CMethodPrototypeTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CMethodPrototypeTests& CMethodPrototypeTests::Instance( void )
{
	static CMethodPrototypeTests s_testCase;
	return s_testCase;
}

void CMethodPrototypeTests::TestParse_GlobalFunction( code::CMethodPrototype& proto )
{
	std::tstring method;

	{	// global function
		method = _T("std::pair<int, int> Func( const CFileItem* pLeft, int depth = 5 ) const");

		proto.SplitMethod( method );
		ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_templateDecl, method ) );
		ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_inlineModifier, method ) );
		ASSERT_EQUAL( _T("std::pair<int, int>"), str::ExtractString( proto.m_returnType, method ) );
		ASSERT_EQUAL( _T("Func"), str::ExtractString( proto.m_functionName, method ) );
		ASSERT_EQUAL( _T("Func"), str::ExtractString( proto.m_qualifiedMethod, method ) );
		ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_classQualifier, method ) );
		ASSERT_EQUAL( _T("( const CFileItem* pLeft, int depth = 5 )"), str::ExtractString( proto.m_argList, method ) );
		ASSERT_EQUAL( _T(" const"), str::ExtractString( proto.m_postArgListSuffix, method ) );
	}

	{	// global operator
		method = _T("const TCHAR* operator()( int left, int right ) const");

		proto.SplitMethod( method );
		ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_templateDecl, method ) );
		ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_inlineModifier, method ) );
		ASSERT_EQUAL( _T("const TCHAR*"), str::ExtractString( proto.m_returnType, method ) );
		ASSERT_EQUAL( _T("operator()"), str::ExtractString( proto.m_functionName, method ) );
		ASSERT_EQUAL( _T("operator()"), str::ExtractString( proto.m_qualifiedMethod, method ) );
		ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_classQualifier, method ) );
		ASSERT_EQUAL( _T("( int left, int right )"), str::ExtractString( proto.m_argList, method ) );
		ASSERT_EQUAL( _T(" const"), str::ExtractString( proto.m_postArgListSuffix, method ) );

		method = _T("const TCHAR* operator()( void ) const");

		proto.SplitMethod( method );
		ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_templateDecl, method ) );
		ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_inlineModifier, method ) );
		ASSERT_EQUAL( _T("const TCHAR*"), str::ExtractString( proto.m_returnType, method ) );
		ASSERT_EQUAL( _T("operator()"), str::ExtractString( proto.m_functionName, method ) );
		ASSERT_EQUAL( _T("operator()"), str::ExtractString( proto.m_qualifiedMethod, method ) );
		ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_classQualifier, method ) );
		ASSERT_EQUAL( _T("( void )"), str::ExtractString( proto.m_argList, method ) );
		ASSERT_EQUAL( _T(" const"), str::ExtractString( proto.m_postArgListSuffix, method ) );
	}
}

void CMethodPrototypeTests::TestParse_ClassMethodImpl( code::CMethodPrototype& proto )
{
	std::tstring method;

	{	// class method implementation
		method = _T("std::pair<int, int> CPattern::Search( const CFileItem* pLeft, int depth = 5 ) const");

		proto.SplitMethod( method );
		ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_templateDecl, method ) );
		ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_inlineModifier, method ) );
		ASSERT_EQUAL( _T("std::pair<int, int>"), str::ExtractString( proto.m_returnType, method ) );
		ASSERT_EQUAL( _T("Search"), str::ExtractString( proto.m_functionName, method ) );
		ASSERT_EQUAL( _T("CPattern::Search"), str::ExtractString( proto.m_qualifiedMethod, method ) );
		ASSERT_EQUAL( _T("CPattern::"), str::ExtractString( proto.m_classQualifier, method ) );
		ASSERT_EQUAL( _T("( const CFileItem* pLeft, int depth = 5 )"), str::ExtractString( proto.m_argList, method ) );
		ASSERT_EQUAL( _T(" const"), str::ExtractString( proto.m_postArgListSuffix, method ) );
	}

	{	// class operator implementation
		method = _T("pred::CompareResult CComparator::operator!=( const CFileItem* pLeft, const CFileItem* pRight ) const");

		proto.SplitMethod( method );
		ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_templateDecl, method ) );
		ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_inlineModifier, method ) );
		ASSERT_EQUAL( _T("pred::CompareResult"), str::ExtractString( proto.m_returnType, method ) );
		ASSERT_EQUAL( _T("operator!="), str::ExtractString( proto.m_functionName, method ) );
		ASSERT_EQUAL( _T("CComparator::operator!="), str::ExtractString( proto.m_qualifiedMethod, method ) );
		ASSERT_EQUAL( _T("CComparator::"), str::ExtractString( proto.m_classQualifier, method ) );
		ASSERT_EQUAL( _T("( const CFileItem* pLeft, const CFileItem* pRight )"), str::ExtractString( proto.m_argList, method ) );
		ASSERT_EQUAL( _T(" const"), str::ExtractString( proto.m_postArgListSuffix, method ) );
	}

	if ( !is_a<code::CMethodPrototypeOld>( &proto ) )
	{
		{	// type conversion operator method
			method = _T("operator PCXSTR() const throw()");

			proto.SplitMethod( method );
			ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_templateDecl, method ) );
			ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_inlineModifier, method ) );
			ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_returnType, method ) );
			ASSERT_EQUAL( _T("operator PCXSTR"), str::ExtractString( proto.m_functionName, method ) );
			ASSERT_EQUAL( _T("operator PCXSTR"), str::ExtractString( proto.m_qualifiedMethod, method ) );
			ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_classQualifier, method ) );
			ASSERT_EQUAL( _T("()"), str::ExtractString( proto.m_argList, method ) );
			ASSERT_EQUAL( _T(" const throw()"), str::ExtractString( proto.m_postArgListSuffix, method ) );
		}

		{	// type conversion operator method
			method = _T("operator const char*() const throw()");

			proto.SplitMethod( method );
			ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_templateDecl, method ) );
			ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_inlineModifier, method ) );
			ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_returnType, method ) );
			ASSERT_EQUAL( _T("operator const char*"), str::ExtractString( proto.m_functionName, method ) );
			ASSERT_EQUAL( _T("operator const char*"), str::ExtractString( proto.m_qualifiedMethod, method ) );
			ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_classQualifier, method ) );
			ASSERT_EQUAL( _T("()"), str::ExtractString( proto.m_argList, method ) );
			ASSERT_EQUAL( _T(" const throw()"), str::ExtractString( proto.m_postArgListSuffix, method ) );
		}
	}
}

void CMethodPrototypeTests::TestParse_TemplateMethodImpl( code::CMethodPrototype& proto )
{
	std::tstring method;

	{	// template class method implementation
		method = _T("\
template< typename PathT, typename ObjectT >\r\n\
inline std::pair<ObjectT*, cache::TStatusFlags> CCacheLoader<PathT, ObjectT>::Acquire( const PathT& pathKey ) const throws(std::exception, std::runtime_error)");

		proto.SplitMethod( method );
		ASSERT_EQUAL( _T("template< typename PathT, typename ObjectT >"), str::ExtractString( proto.m_templateDecl, method ) );
		ASSERT_EQUAL( _T("inline"), str::ExtractString( proto.m_inlineModifier, method ) );
		ASSERT_EQUAL( _T("std::pair<ObjectT*, cache::TStatusFlags>"), str::ExtractString( proto.m_returnType, method ) );
		ASSERT_EQUAL( _T("Acquire"), str::ExtractString( proto.m_functionName, method ) );
		ASSERT_EQUAL( _T("CCacheLoader<PathT, ObjectT>::Acquire"), str::ExtractString( proto.m_qualifiedMethod, method ) );
		ASSERT_EQUAL( _T("CCacheLoader<PathT, ObjectT>::"), str::ExtractString( proto.m_classQualifier, method ) );
		ASSERT_EQUAL( _T("( const PathT& pathKey )"), str::ExtractString( proto.m_argList, method ) );
		ASSERT_EQUAL( _T(" const throws(std::exception, std::runtime_error)"), str::ExtractString( proto.m_postArgListSuffix, method ) );
	}

	{	// template class method implementation placeholder: no method
		method = _T("\
template< typename PathT, typename ObjectT >\r\n\
inline std::pair<ObjectT*, cache::TStatusFlags> CCacheLoader<PathT, ObjectT>::");

		proto.SplitMethod( method );
		ASSERT_EQUAL( _T("template< typename PathT, typename ObjectT >"), str::ExtractString( proto.m_templateDecl, method ) );
		ASSERT_EQUAL( _T("inline"), str::ExtractString( proto.m_inlineModifier, method ) );
		ASSERT_EQUAL( _T("std::pair<ObjectT*, cache::TStatusFlags>"), str::ExtractString( proto.m_returnType, method ) );
		ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_functionName, method ) );
		ASSERT_EQUAL( _T("CCacheLoader<PathT, ObjectT>::"), str::ExtractString( proto.m_qualifiedMethod, method ) );
		ASSERT_EQUAL( _T("CCacheLoader<PathT, ObjectT>::"), str::ExtractString( proto.m_classQualifier, method ) );
		ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_argList, method ) );
		ASSERT_EQUAL( _T(""), str::ExtractString( proto.m_postArgListSuffix, method ) );
	}
}


void CMethodPrototypeTests::Run( void )
{
	{
		code::CMethodPrototypeOld protoOld;

		RUN_TEST1( TestParse_GlobalFunction, protoOld );
		RUN_TEST1( TestParse_ClassMethodImpl, protoOld );
		RUN_TEST1( TestParse_TemplateMethodImpl, protoOld );
	}
	{
		code::CMethodPrototype proto;

		RUN_TEST1( TestParse_GlobalFunction, proto );
		RUN_TEST1( TestParse_ClassMethodImpl, proto );
		RUN_TEST1( TestParse_TemplateMethodImpl, proto );
	}
}


#endif //USE_UT
