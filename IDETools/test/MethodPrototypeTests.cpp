
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "utl/StringUtilities.h"
#include "MethodPrototype.h"
#include "CppImplementationFormatter.h"
#include "FormatterOptions.h"
//#include "Application_fwd.h"
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

		proto.ParseCode( method );
		ASSERT_EQUAL( _T(""), proto.m_templateDecl.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_inlineModifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("std::pair<int, int>"), proto.m_returnType.MakeToken( method ) );
		ASSERT_EQUAL( _T("Func"), proto.m_functionName.MakeToken( method ) );
		ASSERT_EQUAL( _T("Func"), proto.m_qualifiedMethod.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_classQualifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("( const CFileItem* pLeft, int depth = 5 )"), proto.m_argList.MakeToken( method ) );
		ASSERT_EQUAL( _T(" const"), proto.m_postArgListSuffix.MakeToken( method ) );
	}

	{	// global operator
		method = _T("const TCHAR* operator()( int left, int right ) const");

		proto.ParseCode( method );
		ASSERT_EQUAL( _T(""), proto.m_templateDecl.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_inlineModifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("const TCHAR*"), proto.m_returnType.MakeToken( method ) );
		ASSERT_EQUAL( _T("operator()"), proto.m_functionName.MakeToken( method ) );
		ASSERT_EQUAL( _T("operator()"), proto.m_qualifiedMethod.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_classQualifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("( int left, int right )"), proto.m_argList.MakeToken( method ) );
		ASSERT_EQUAL( _T(" const"), proto.m_postArgListSuffix.MakeToken( method ) );

		method = _T("const TCHAR* operator()( void ) const");

		proto.ParseCode( method );
		ASSERT_EQUAL( _T(""), proto.m_templateDecl.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_inlineModifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("const TCHAR*"), proto.m_returnType.MakeToken( method ) );
		ASSERT_EQUAL( _T("operator()"), proto.m_functionName.MakeToken( method ) );
		ASSERT_EQUAL( _T("operator()"), proto.m_qualifiedMethod.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_classQualifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("( void )"), proto.m_argList.MakeToken( method ) );
		ASSERT_EQUAL( _T(" const"), proto.m_postArgListSuffix.MakeToken( method ) );
	}

	// cast operator method (type conversion)
	{
		method = _T("operator PCXSTR() const throw()");

		proto.ParseCode( method );
		ASSERT_EQUAL( _T(""), proto.m_templateDecl.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_inlineModifier.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_returnType.MakeToken( method ) );
		ASSERT_EQUAL( _T("operator PCXSTR"), proto.m_functionName.MakeToken( method ) );
		ASSERT_EQUAL( _T("operator PCXSTR"), proto.m_qualifiedMethod.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_classQualifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("()"), proto.m_argList.MakeToken( method ) );
		ASSERT_EQUAL( _T(" const throw()"), proto.m_postArgListSuffix.MakeToken( method ) );
	}
	{
		method = _T("operator const char*() const throw()");

		proto.ParseCode( method );
		ASSERT_EQUAL( _T(""), proto.m_templateDecl.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_inlineModifier.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_returnType.MakeToken( method ) );
		ASSERT_EQUAL( _T("operator const char*"), proto.m_functionName.MakeToken( method ) );
		ASSERT_EQUAL( _T("operator const char*"), proto.m_qualifiedMethod.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_classQualifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("()"), proto.m_argList.MakeToken( method ) );
		ASSERT_EQUAL( _T(" const throw()"), proto.m_postArgListSuffix.MakeToken( method ) );
	}
}

void CMethodPrototypeTests::TestParse_ClassMethodImpl( code::CMethodPrototype& proto )
{
	std::tstring method;

	{	// class method implementation
		method = _T("std::pair<int, int> CPattern::Search( const CFileItem* pLeft, int depth = 5 ) const");

		proto.ParseCode( method );
		ASSERT_EQUAL( _T(""), proto.m_templateDecl.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_inlineModifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("std::pair<int, int>"), proto.m_returnType.MakeToken( method ) );
		ASSERT_EQUAL( _T("Search"), proto.m_functionName.MakeToken( method ) );
		ASSERT_EQUAL( _T("CPattern::Search"), proto.m_qualifiedMethod.MakeToken( method ) );
		ASSERT_EQUAL( _T("CPattern::"), proto.m_classQualifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("( const CFileItem* pLeft, int depth = 5 )"), proto.m_argList.MakeToken( method ) );
		ASSERT_EQUAL( _T(" const"), proto.m_postArgListSuffix.MakeToken( method ) );
	}

	{	// class operator implementation
		method = _T("pred::CompareResult CComparator::operator!=( const CFileItem* pLeft, const CFileItem* pRight ) const");

		proto.ParseCode( method );
		ASSERT_EQUAL( _T(""), proto.m_templateDecl.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_inlineModifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("pred::CompareResult"), proto.m_returnType.MakeToken( method ) );
		ASSERT_EQUAL( _T("operator!="), proto.m_functionName.MakeToken( method ) );
		ASSERT_EQUAL( _T("CComparator::operator!="), proto.m_qualifiedMethod.MakeToken( method ) );
		ASSERT_EQUAL( _T("CComparator::"), proto.m_classQualifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("( const CFileItem* pLeft, const CFileItem* pRight )"), proto.m_argList.MakeToken( method ) );
		ASSERT_EQUAL( _T(" const"), proto.m_postArgListSuffix.MakeToken( method ) );
	}
}

void CMethodPrototypeTests::TestParse_TemplateMethodImpl( code::CMethodPrototype& proto )
{
	std::tstring method;

	{	// template class method implementation
		method = _T("\
template< typename PathT, typename ObjectT >\r\n\
inline std::pair<ObjectT*, cache::TStatusFlags> CCacheLoader<PathT, ObjectT>::Acquire( const PathT& pathKey ) const throws(std::exception, std::runtime_error)");

		proto.ParseCode( method );
		ASSERT_EQUAL( _T("template< typename PathT, typename ObjectT >"), proto.m_templateDecl.MakeToken( method ) );
		ASSERT_EQUAL( _T("inline"), proto.m_inlineModifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("std::pair<ObjectT*, cache::TStatusFlags>"), proto.m_returnType.MakeToken( method ) );
		ASSERT_EQUAL( _T("Acquire"), proto.m_functionName.MakeToken( method ) );
		ASSERT_EQUAL( _T("CCacheLoader<PathT, ObjectT>::Acquire"), proto.m_qualifiedMethod.MakeToken( method ) );
		ASSERT_EQUAL( _T("CCacheLoader<PathT, ObjectT>::"), proto.m_classQualifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("( const PathT& pathKey )"), proto.m_argList.MakeToken( method ) );
		ASSERT_EQUAL( _T(" const throws(std::exception, std::runtime_error)"), proto.m_postArgListSuffix.MakeToken( method ) );
	}

	{	// template class method implementation placeholder: no method
		method = _T("\
template< typename PathT, typename ObjectT >\r\n\
inline std::pair<ObjectT*, cache::TStatusFlags> CCacheLoader<PathT, ObjectT>::");

		proto.ParseCode( method );
		ASSERT_EQUAL( _T("template< typename PathT, typename ObjectT >"), proto.m_templateDecl.MakeToken( method ) );
		ASSERT_EQUAL( _T("inline"), proto.m_inlineModifier.MakeToken( method ) );
		ASSERT_EQUAL( _T("std::pair<ObjectT*, cache::TStatusFlags>"), proto.m_returnType.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_functionName.MakeToken( method ) );
		ASSERT_EQUAL( _T("CCacheLoader<PathT, ObjectT>::"), proto.m_qualifiedMethod.MakeToken( method ) );
		ASSERT_EQUAL( _T("CCacheLoader<PathT, ObjectT>::"), proto.m_classQualifier.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_argList.MakeToken( method ) );
		ASSERT_EQUAL( _T(""), proto.m_postArgListSuffix.MakeToken( method ) );
	}
}


void CMethodPrototypeTests::TestImplementMethodBlock( void )
{
	code::CFormatterOptions options;		// test with default options, not customized by user
	code::CppImplementationFormatter formatter( options );

	std::tstring methods = _T("\
\tstd::pair<int, int> Func( const CFileItem* pLeft = _T(\"END\"), int depth = 5 ) const;\r\n\
\tconst TCHAR* operator()( int left, int right ) const;\r\n\
");

	std::tstring typeDescriptor;
	{	// global functions
		//typeDescriptor = _T("\t");

		ASSERT_EQUAL_SWAP( formatter.ImplementMethodBlock( methods.c_str(), typeDescriptor.c_str(), false ),
						   _T("\
std::pair<int, int> Func( const CFileItem* pLeft /*= _T(\"END\")*/, int depth /*= 5*/ ) const\r\n\
{\r\n\
	return ;\r\n\
}\r\n\
\r\n\
const TCHAR* operator()( int left, int right ) const\r\n\
{\r\n\
	return ;\r\n\
}\r\n\
\r\n\
")
);
	}

	{	// class methods
		typeDescriptor = _T("CCmd::");

		ASSERT_EQUAL_SWAP( formatter.ImplementMethodBlock( methods.c_str(), typeDescriptor.c_str(), false ),
						   _T("\
std::pair<int, int> CCmd::Func( const CFileItem* pLeft /*= _T(\"END\")*/, int depth /*= 5*/ ) const\r\n\
{\r\n\
	return ;\r\n\
}\r\n\
\r\n\
const TCHAR* CCmd::operator()( int left, int right ) const\r\n\
{\r\n\
	return ;\r\n\
}\r\n\
\r\n\
")
		);
	}

	{	// template class methods
		typeDescriptor = _T("\
template< typename T, typename V >\r\n\
CCache<T, V>::");

		ASSERT_EQUAL_SWAP( formatter.ImplementMethodBlock( methods.c_str(), typeDescriptor.c_str(), false ),
						   _T("\
template< typename T, typename V >\r\n\
std::pair<int, int> CCache<T, V>::Func( const CFileItem* pLeft /*= _T(\"END\")*/, int depth /*= 5*/ ) const\r\n\
{\r\n\
	return ;\r\n\
}\r\n\
\r\n\
template< typename T, typename V >\r\n\
const TCHAR* CCache<T, V>::operator()( int left, int right ) const\r\n\
{\r\n\
	return ;\r\n\
}\r\n\
\r\n\
")
);

		// inline
		ASSERT_EQUAL_SWAP( formatter.ImplementMethodBlock( methods.c_str(), typeDescriptor.c_str(), true ),
						   _T("\
template< typename T, typename V >\r\n\
inline std::pair<int, int> CCache<T, V>::Func( const CFileItem* pLeft /*= _T(\"END\")*/, int depth /*= 5*/ ) const\r\n\
{\r\n\
	return ;\r\n\
}\r\n\
\r\n\
template< typename T, typename V >\r\n\
inline const TCHAR* CCache<T, V>::operator()( int left, int right ) const\r\n\
{\r\n\
	return ;\r\n\
}\r\n\
\r\n\
")
);
	}
}


void CMethodPrototypeTests::Run( void )
{
	{
		code::CMethodPrototype proto;

		RUN_TEST1( TestParse_GlobalFunction, proto );
		RUN_TEST1( TestParse_ClassMethodImpl, proto );
		RUN_TEST1( TestParse_TemplateMethodImpl, proto );
	}
	RUN_TEST( TestImplementMethodBlock );
}


#endif //USE_UT
