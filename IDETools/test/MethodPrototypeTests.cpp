
#include "stdafx.h"

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

void CMethodPrototypeTests::TestParse_GlobalFunction( void )
{
	std::tstring method;
	code::CMethodPrototype proto;

	{	// global function
		method = _T("std::pair<int, int> Func( const CFileItem* pLeft, int depth = 5 ) const");

		proto.SplitMethod( method );
		ASSERT_EQUAL( _T(""), proto.m_templateDecl.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_inlineModifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("std::pair<int, int>"), proto.m_returnType.MakeToken( method ) );
		ASSERT_EQUAL( _T("Func"), proto.m_functionName.MakeToken( method ) );
		ASSERT_EQUAL( _T("Func"), proto.m_methodQualifiedName.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_typeQualifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("( const CFileItem* pLeft, int depth = 5 )"), proto.m_argList.MakeToken( method ) );
		ASSERT_EQUAL( _T(" const"), proto.m_postArgListSuffix.MakeToken( method ) );
	}

	{	// global operator
		method = _T("const TCHAR* operator()( int left, int right ) const");

		proto.SplitMethod( method );
		ASSERT_EQUAL( _T(""), proto.m_templateDecl.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_inlineModifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("const TCHAR*"), proto.m_returnType.MakeToken( method ) );
		ASSERT_EQUAL( _T("operator()"), proto.m_functionName.MakeToken( method ) );
		ASSERT_EQUAL( _T("operator()"), proto.m_methodQualifiedName.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_typeQualifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("( int left, int right )"), proto.m_argList.MakeToken( method ) );
		ASSERT_EQUAL( _T(" const"), proto.m_postArgListSuffix.MakeToken( method ) );

		method = _T("const TCHAR* operator()( void ) const");

		proto.SplitMethod( method );
		ASSERT_EQUAL( _T(""), proto.m_templateDecl.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_inlineModifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("const TCHAR*"), proto.m_returnType.MakeToken( method ) );
		ASSERT_EQUAL( _T("operator()"), proto.m_functionName.MakeToken( method ) );
		ASSERT_EQUAL( _T("operator()"), proto.m_methodQualifiedName.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_typeQualifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("( void )"), proto.m_argList.MakeToken( method ) );
		ASSERT_EQUAL( _T(" const"), proto.m_postArgListSuffix.MakeToken( method ) );
	}
}

void CMethodPrototypeTests::TestParse_ClassMethodImpl( void )
{
	std::tstring method;
	code::CMethodPrototype proto;

	{	// class method implementation
		method = _T("std::pair<int, int> CPattern::Search( const CFileItem* pLeft, int depth = 5 ) const");

		proto.SplitMethod( method );
		ASSERT_EQUAL( _T(""), proto.m_templateDecl.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_inlineModifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("std::pair<int, int>"), proto.m_returnType.MakeToken( method ) );
		ASSERT_EQUAL( _T("Search"), proto.m_functionName.MakeToken( method ) );
		ASSERT_EQUAL( _T("CPattern::Search"), proto.m_methodQualifiedName.MakeToken( method ) );
		ASSERT_EQUAL( _T("CPattern::"), proto.m_typeQualifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("( const CFileItem* pLeft, int depth = 5 )"), proto.m_argList.MakeToken( method ) );
		ASSERT_EQUAL( _T(" const"), proto.m_postArgListSuffix.MakeToken( method ) );
	}

	{	// class operator implementation
		method = _T("pred::CompareResult CComparator::operator!=( const CFileItem* pLeft, const CFileItem* pRight ) const");

		proto.SplitMethod( method );
		ASSERT_EQUAL( _T(""), proto.m_templateDecl.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_inlineModifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("pred::CompareResult"), proto.m_returnType.MakeToken( method ) );
		ASSERT_EQUAL( _T("operator!="), proto.m_functionName.MakeToken( method ) );
		ASSERT_EQUAL( _T("CComparator::operator!="), proto.m_methodQualifiedName.MakeToken( method ) );
		ASSERT_EQUAL( _T("CComparator::"), proto.m_typeQualifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("( const CFileItem* pLeft, const CFileItem* pRight )"), proto.m_argList.MakeToken( method ) );
		ASSERT_EQUAL( _T(" const"), proto.m_postArgListSuffix.MakeToken( method ) );
	}
}

void CMethodPrototypeTests::TestParse_TemplateMethodImpl( void )
{
	std::tstring method;
	code::CMethodPrototype proto;

	{	// template class method implementation
		method = _T("\
template< typename PathT, typename ObjectT >\r\n\
inline std::pair<ObjectT*, cache::TStatusFlags> CCacheLoader<PathT, ObjectT>::Acquire( const PathT& pathKey ) const throws(std::exception, std::runtime_error)");

		proto.SplitMethod( method );
		ASSERT_EQUAL( _T("template< typename PathT, typename ObjectT >"), proto.m_templateDecl.MakeToken( method ) );
		ASSERT_EQUAL( _T("inline"), proto.m_inlineModifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("std::pair<ObjectT*, cache::TStatusFlags>"), proto.m_returnType.MakeToken( method ) );
		ASSERT_EQUAL( _T("Acquire"), proto.m_functionName.MakeToken( method ) );
		ASSERT_EQUAL( _T("CCacheLoader<PathT, ObjectT>::Acquire"), proto.m_methodQualifiedName.MakeToken( method ) );
		ASSERT_EQUAL( _T("CCacheLoader<PathT, ObjectT>::"), proto.m_typeQualifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("( const PathT& pathKey )"), proto.m_argList.MakeToken( method ) );
		ASSERT_EQUAL( _T(" const throws(std::exception, std::runtime_error)"), proto.m_postArgListSuffix.MakeToken( method ) );
	}
}


void CMethodPrototypeTests::Run( void )
{
	__super::Run();

	TestParse_GlobalFunction();
	TestParse_ClassMethodImpl();
	TestParse_TemplateMethodImpl();
}


#endif //USE_UT
