
#include "stdafx.h"

#ifdef USE_UT		// no UT code in release builds
#include "utl/StringUtilities.h"
#include "utl/StringParsing.h"
#include "CppMethodComponents.h"
#include "BraceParityStatus.h"
#include "test/CodeParserExTests.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CCodeParserExTests::CCodeParserExTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CCodeParserExTests& CCodeParserExTests::Instance( void )
{
	static CCodeParserExTests s_testCase;
	return s_testCase;
}

void CCodeParserExTests::TestBraceParityStatus( void )
{
	const std::tstring code = _T("std::pair<ObjectT*, cache::TStatusFlags> CCacheLoader< std::pair<PathT, size_t>, ObjectT >::Acquire( const PathT& pathKey )");

	TokenRange token( code.find( _T('<') ) );

	ASSERT( code::ide_tools::SkipBrace( &token.m_end, code.c_str(), token.m_start ) );
	++token.m_end;
	ASSERT_EQUAL( _T("<ObjectT*, cache::TStatusFlags>"), token.GetToken( code.c_str() ) );

	token.setEmpty( code.find( _T("::Acquire") ) );
	ASSERT( token.m_start != -1 );
	--token.m_start;
	ASSERT( code::ide_tools::SkipBraceBackwards( &token.m_start, code.c_str(), token.m_start ) );
	ASSERT_EQUAL( _T("< std::pair<PathT, size_t>, ObjectT >"), token.GetToken( code.c_str() ) );
}

void CCodeParserExTests::TestMethodComponents( void )
{
	const TCHAR methodPrototype[] = _T("\
template< typename PathT, typename ObjectT >\r\n\
std::pair<ObjectT*, cache::TStatusFlags> CCacheLoader<PathT, ObjectT>::Acquire( const PathT& pathKey )");

	code::CppMethodComponents mc( methodPrototype );

	mc.splitMethod();
	ASSERT_EQUAL( _T("template< typename PathT, typename ObjectT >"), mc.m_templateDecl.GetToken( methodPrototype ) );
	ASSERT_EQUAL( _T(""), mc.m_inlineModifier.GetToken( methodPrototype ) );
	ASSERT_EQUAL( _T("std::pair<ObjectT*, cache::TStatusFlags>"), mc.m_returnType.GetToken( methodPrototype ) );
	ASSERT_EQUAL( _T("CCacheLoader<PathT, ObjectT>::Acquire"), mc.m_methodName.GetToken( methodPrototype ) );
	ASSERT_EQUAL( _T("CCacheLoader<PathT, ObjectT>::"), mc.m_typeQualifier.GetToken( methodPrototype ) );
	ASSERT_EQUAL( _T("( const PathT& pathKey )"), mc.m_argList.GetToken( methodPrototype ) );
	ASSERT_EQUAL( _T(""), mc.m_postArgListSuffix.GetToken( methodPrototype ) );
}


void CCodeParserExTests::Run( void )
{
	__super::Run();

	TestBraceParityStatus();
	TestMethodComponents();
}


#endif //USE_UT
