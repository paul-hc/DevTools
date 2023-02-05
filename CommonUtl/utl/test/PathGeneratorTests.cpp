
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/PathGeneratorTests.h"
#include "test/TempFilePairPool.h"
#include "PathUniqueMaker.h"
#include "PathGenerator.h"
#include "FlexPath.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	bool GeneratePathPairs( ut::CPathPairPool& rPool, const std::tstring& format, UINT seqCount = 1, bool ignoreExtension = false )
	{
		CPathGenerator gen( &rPool.m_pathPairs, CPathFormatter( format, ignoreExtension ), seqCount );
		return gen.GeneratePairs();
	}

	UINT FindNextAvailSeqCount( const ut::CPathPairPool& pool, const std::tstring& format, bool ignoreExtension = false )
	{
		CPathGenerator gen( pool.m_pathPairs, CPathFormatter( format, ignoreExtension ), 1, false );
		return gen.FindNextAvailSeqCount();
	}
}


CPathGeneratorTests::CPathGeneratorTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CPathGeneratorTests& CPathGeneratorTests::Instance( void )
{
	static CPathGeneratorTests s_testCase;
	return s_testCase;
}

void CPathGeneratorTests::TestPathUniqueMaker( void )
{
	CPathUniqueMaker uniquePathMaker;

	ASSERT_EQUAL( _T("C:\\Tools\\storage.doc>a.txt"), uniquePathMaker.MakeUnique( fs::CFlexPath( _T("C:\\Tools\\storage.doc>a.txt") ) ) );

	ASSERT_EQUAL( _T("C:\\Tools\\storage.doc>B\\b.txt"), uniquePathMaker.MakeUnique( fs::CFlexPath( _T("C:\\Tools\\storage.doc>B\\b.txt") ) ) );
	ASSERT_EQUAL( _T("C:\\Tools\\storage.doc>B\\b_[2].txt"), uniquePathMaker.MakeUnique( fs::CFlexPath( _T("C:\\Tools\\storage.doc>B\\b.txt") ) ) );
	ASSERT_EQUAL( _T("C:\\Tools\\storage.doc>B/b_[3].txt"), uniquePathMaker.MakeUnique( fs::CFlexPath( _T("C:\\Tools\\storage.doc>B/b.txt") ) ) );
	ASSERT_EQUAL( _T("C:\\Tools\\storage.doc>B\\b_[470].txt"), uniquePathMaker.MakeUnique( fs::CFlexPath( _T("C:\\Tools\\storage.doc>B\\b_[470].txt") ) ) );	// force a jump to higher sequence number
	ASSERT_EQUAL( _T("C:\\Tools\\storage.doc>B\\b_[471].txt"), uniquePathMaker.MakeUnique( fs::CFlexPath( _T("C:\\Tools\\storage.doc>B\\b.txt") ) ) );

	ASSERT_EQUAL( _T("C:\\Tools\\storage.doc>a_[2].txt"), uniquePathMaker.MakeUnique( fs::CFlexPath( _T("C:\\Tools\\storage.doc>a.txt") ) ) );
	ASSERT_EQUAL( _T("C:\\Tools\\storage.doc>a_[3].txt"), uniquePathMaker.MakeUnique( fs::CFlexPath( _T("C:\\Tools\\storage.doc>a.txt") ) ) );

	ASSERT_EQUAL( _T("C:\\Tools\\storage.doc>a_[10].txt"), uniquePathMaker.MakeUnique( fs::CFlexPath( _T("C:\\Tools\\storage.doc>a_[10].txt") ) ) );		// force a jump to higher sequence number
	ASSERT_EQUAL( _T("C:\\Tools\\storage.doc>a_[11].txt"), uniquePathMaker.MakeUnique( fs::CFlexPath( _T("C:\\Tools\\storage.doc>a.txt") ) ) );

	// batch uniquify
	std::vector<fs::CPath> paths;
	str::Split( paths, _T("x|y|z|z|x|y_[15]|y|x|y"), _T("|") );

	ASSERT_EQUAL( 5, uniquePathMaker.UniquifyPaths( paths ) );			// number of duplicates uniquified
	ASSERT_EQUAL( _T("x|y|z|z_[2]|x_[2]|y_[15]|y_[16]|x_[3]|y_[17]"), str::Join( paths, _T("|") ) );
}

void CPathGeneratorTests::TestPathMaker( void )
{
	{
		ut::CPathPairPool pool( _T("C:\\Tools\\My\\Batch\\a.txt"), true );
		CPathMaker gen( &pool.m_pathPairs );

		ASSERT_EQUAL( _T("C:\\Tools\\My\\Batch"), gen.FindSrcCommonPrefix() );

		ASSERT( !gen.MakeDestRelative( _T("C:\\Tools\\Other") ) );

		ASSERT( gen.MakeDestRelative( _T("C:\\Tools\\My\\Batch") ) );
		ASSERT_EQUAL( _T("a.txt"), pool.JoinDest() );

		ASSERT( gen.MakeDestRelative( _T("C:\\Tools\\My\\Batch\\") ) );
		ASSERT_EQUAL( _T("a.txt"), pool.JoinDest() );

		ASSERT( gen.MakeDestRelative( _T("C:/Tools/My") ) );
		ASSERT_EQUAL( _T("Batch\\a.txt"), pool.JoinDest() );

		ASSERT( gen.MakeDestStripCommonPrefix() );
		ASSERT_EQUAL( _T("a.txt"), pool.JoinDest() );
	}
	{
		ut::CPathPairPool pool( _T("C:/Tools/Other/a.txt|C:\\Tools\\My\\Batch\\b.txt|C:\\Tools\\My\\Utils\\c.txt"), true );
		CPathMaker gen( &pool.m_pathPairs );

		ASSERT_EQUAL( _T("C:\\Tools"), gen.FindSrcCommonPrefix() );

		ASSERT( !gen.MakeDestRelative( _T("C:\\Tools\\Other") ) );

		ASSERT( gen.MakeDestRelative( _T("C:\\") ) );
		ASSERT_EQUAL( _T("Tools/Other/a.txt|Tools\\My\\Batch\\b.txt|Tools\\My\\Utils\\c.txt"), pool.JoinDest() );

		ASSERT( gen.MakeDestStripCommonPrefix() );
		ASSERT_EQUAL( _T("Other/a.txt|My\\Batch\\b.txt|My\\Utils\\c.txt"), pool.JoinDest() );
	}
	{
		ut::CPathPairPool pool( _T("C:\\Tools\\a.txt|D:\\b.txt"), true );
		CPathMaker gen( &pool.m_pathPairs );

		ASSERT( !gen.MakeDestStripCommonPrefix() );
	}
}

void CPathGeneratorTests::TestPathFormatter( void )
{
	{
		bool syntaxOk;
		ASSERT_EQUAL( _T("foo 05"), CPathFormatter::FormatPart( _T("filename"), _T("foo ##"), 5, &syntaxOk ) );
		ASSERT( syntaxOk );

		ASSERT_EQUAL( _T("foo 17"), CPathFormatter::FormatPart( _T("filename"), _T("foo ##"), 17, &syntaxOk ) );
		ASSERT( syntaxOk );

		ASSERT_EQUAL( _T("foo_filename_end"), CPathFormatter::FormatPart( _T("filename"), _T("foo_*_end"), 5, &syntaxOk ) );
		ASSERT( syntaxOk );
	}
	{
		UINT seqCount;

		ASSERT( CPathFormatter::ParsePart( seqCount, _T("foo #"), _T("foo 5") ) );
		ASSERT_EQUAL( 5, seqCount );

		ASSERT( CPathFormatter::ParsePart( seqCount, _T("foo ###x"), _T("foo 73x") ) );
		ASSERT_EQUAL( 73, seqCount );

		ASSERT( CPathFormatter::ParsePart( seqCount, _T("foo #?xyz"), _T("foo 28 xyz") ) );
		ASSERT_EQUAL( 28, seqCount );

		ASSERT( CPathFormatter::ParsePart( seqCount, _T("* #"), _T("foo 19") ) );
		ASSERT_EQUAL( 19, seqCount );

		ASSERT( CPathFormatter::ParsePart( seqCount, _T("f*b #"), _T("fab 17") ) );
		ASSERT_EQUAL( 17, seqCount );
	}
	{
		// wildcard
		ASSERT_EQUAL( _T("foo.txt"), CPathFormatter().FormatPath( fs::CPath( _T("foo.txt") ), 1 ).Get() );

		ASSERT_EQUAL( _T("foo.txt"), CPathFormatter( _T("*"), false ).FormatPath( fs::CPath( _T("foo.txt") ), 1 ).Get() );
		ASSERT_EQUAL( _T("foo.txt"), CPathFormatter( _T("*.*"), false ).FormatPath( fs::CPath( _T("foo.txt") ), 1 ).Get() );
		ASSERT_EQUAL( _T("foo.txt"), CPathFormatter( _T("*.*"), false ).FormatPath( fs::CPath( _T("foo.txt") ), 1, 1 ).Get() );
		ASSERT_EQUAL( _T("foo"), CPathFormatter( _T("*."), false ).FormatPath( fs::CPath( _T("foo.txt") ), 1 ).Get() );
		ASSERT_EQUAL( _T(".txt"), CPathFormatter( _T(".*"), false ).FormatPath( fs::CPath( _T("foo.txt") ), 1 ).Get() );

		ASSERT_EQUAL( _T("foo.txt"), CPathFormatter( _T("*.txt"), false ).FormatPath( fs::CPath( _T("foo") ), 1 ).Get() );
		ASSERT_EQUAL( _T("foo.uk"), CPathFormatter( _T("*.uk"), false ).FormatPath( fs::CPath( _T("foo") ), 1 ).Get() );
		ASSERT_EQUAL( _T("foo.co.uk"), CPathFormatter( _T("*.co.uk"), false ).FormatPath( fs::CPath( _T("foo") ), 1 ).Get() );
		ASSERT_EQUAL( _T("foo.co.uk"), CPathFormatter( _T("??*.co.uk"), false ).FormatPath( fs::CPath( _T("foo") ), 1 ).Get() );
		ASSERT_EQUAL( _T("fxy.co.uk"), CPathFormatter( _T("?xy.co.uk"), false ).FormatPath( fs::CPath( _T("foo") ), 1 ).Get() );

		ASSERT_EQUAL( _T("foo_$(3).txt"), CPathFormatter( _T("*.*"), false ).FormatPath( fs::CPath( _T("foo.txt") ), 1, 3 ).Get() );

		ASSERT_EQUAL( _T("A foo.txt"), CPathFormatter( _T("A *"), true ).FormatPath( fs::CPath( _T("foo.txt") ), 1 ).Get() );
		ASSERT_EQUAL( _T("A foo.jpg.txt"), CPathFormatter( _T("A *.jpg"), true ).FormatPath( fs::CPath( _T("foo.txt") ), 1 ).Get() );

		// numeric
		ASSERT_EQUAL( _T("foo 1.txt"), CPathFormatter( _T("foo #.txt"), false ).FormatPath( fs::CPath( _T("fname.doc") ), 1 ).Get() );
		ASSERT_EQUAL( _T("foo 001.txt"), CPathFormatter( _T("foo ###.*"), false ).FormatPath( fs::CPath( _T("fname.txt") ), 1 ).Get() );
		ASSERT_EQUAL( _T("foo 010.txt"), CPathFormatter( _T("foo ###.*"), false ).FormatPath( fs::CPath( _T("fname.txt") ), 10 ).Get() );
		ASSERT_EQUAL( _T("foo 100.txt"), CPathFormatter( _T("foo ###.*"), false ).FormatPath( fs::CPath( _T("fname.txt") ), 100 ).Get() );
		ASSERT_EQUAL( _T("foo 100.txt"), CPathFormatter( _T("foo ###.*"), false ).FormatPath( fs::CPath( _T("fname.txt") ), 100, 1 ).Get() );
		ASSERT_EQUAL( _T("foo 100_$(2).txt"), CPathFormatter( _T("foo ###.*"), false ).FormatPath( fs::CPath( _T("fname.txt") ), 100, 2 ).Get() );

		ASSERT_EQUAL( _T("foo 050 fname_$(3).txt"), CPathFormatter( _T("foo ### *.*"), false ).FormatPath( fs::CPath( _T("fname.txt") ), 50, 3 ).Get() );
		ASSERT_EQUAL( _T("foo 050 fname_$(3).txt"), CPathFormatter( _T("foo %03d *.*"), false ).FormatPath( fs::CPath( _T("fname.txt") ), 50, 3 ).Get() );
		ASSERT_EQUAL( _T("foo 50 fname_$(3).txt"), CPathFormatter( _T("foo %d *.*"), false ).FormatPath( fs::CPath( _T("fname.txt") ), 50, 3 ).Get() );

		ASSERT_EQUAL( _T("foo 1-xy.doc"), CPathFormatter( _T("foo #-xy"), true ).FormatPath( fs::CPath( _T("fname.doc") ), 1 ).Get() );
		ASSERT_EQUAL( _T("foo 1-xy.jpg.doc"), CPathFormatter( _T("foo #-xy.jpg"), true ).FormatPath( fs::CPath( _T("fname.doc") ), 1 ).Get() );
	}
	{
		UINT seqCount;

		ASSERT( CPathFormatter( _T("foo ###x.jpg"), false ).ParseSeqCount( seqCount, fs::CPath( _T("C:\\my\\foo 73x.jpg") ) ) );
		ASSERT_EQUAL( 73, seqCount );

		ASSERT( !CPathFormatter( _T("foo ###Y.jpg"), false ).ParseSeqCount( seqCount, fs::CPath( _T("C:\\my\\foo 73X.jpg") ) ) );
	}
}

void CPathGeneratorTests::TestNumSeqGeneration( void )
{
	static const std::tstring numFmt = _T("foo ##.txt");
	{
		ut::CPathPairPool pool( _T("a.txt|b.txt|c.txt") );
		ut::GeneratePathPairs( pool, numFmt, 3 );
		ASSERT_EQUAL( _T("foo 03.txt|foo 04.txt|foo 05.txt"), pool.JoinDest() );
	}
	{
		ut::CPathPairPool pool( _T("foo 03.txt|foo 04.txt|foo 05.txt") );
		ut::GeneratePathPairs( pool, numFmt, 3 );
		ASSERT_EQUAL( _T("foo 03.txt|foo 04.txt|foo 05.txt"), pool.JoinDest() );
	}
	{
		ut::CPathPairPool pool( _T("foo 03.txt|foo 05.txt|foo 07.txt") );
		ut::GeneratePathPairs( pool, numFmt, 3 );
		ASSERT_EQUAL( _T("foo 03.txt|foo 04.txt|foo 05.txt"), pool.JoinDest() );
	}
}

void CPathGeneratorTests::TestNumSeqFileGeneration( void )
{
	static const std::tstring numFmt = _T("foo ##.txt");

	// test with physical files pool
	{
		ut::CTempFilePairPool pool( _T("a.txt|b.txt|c.txt") );
		ut::GeneratePathPairs( pool, numFmt, 3 );
		ASSERT_EQUAL( _T("foo 03.txt|foo 04.txt|foo 05.txt"), pool.JoinDest() );
	}
	{
		ut::CTempFilePairPool pool( _T("foo 03.txt|foo 04.txt|foo 05.txt") );
		ut::GeneratePathPairs( pool, numFmt, 3 );
		ASSERT_EQUAL( _T("foo 03.txt|foo 04.txt|foo 05.txt"), pool.JoinDest() );
	}
	{
		ut::CTempFilePairPool pool( _T("foo 04.txt|foo 06.txt|foo 08.txt") );
		ut::GeneratePathPairs( pool, numFmt, 3 );
		ASSERT_EQUAL( _T("foo 03.txt|foo 04.txt|foo 05.txt"), pool.JoinDest() );
	}
}

void CPathGeneratorTests::TestFindNextAvailSeqCount( void )
{
	static const std::tstring numFmt = _T("foo #.txt");
	{
		ut::CTempFilePairPool pool( _T("foo 03.txt|foo 04.txt|foo 05.txt") );
		ASSERT_EQUAL( 6, ut::FindNextAvailSeqCount( pool, numFmt ) );
	}
	{
		ut::CTempFilePairPool pool( _T("foo 04.txt|foo 06.txt|foo 08.txt") );
		ASSERT_EQUAL( 9, ut::FindNextAvailSeqCount( pool, numFmt ) );
	}
	{
		ut::CTempFilePairPool poolBase( _T("foo 3.txt|foo 4.txt|foo 5.txt") );
		ut::CTempFilePairPool poolMore( _T("foo 7.txt|foo 9.txt|foo 11.txt") );
		ASSERT_EQUAL( 12, ut::FindNextAvailSeqCount( poolBase, numFmt ) );			// should skip all existing files
	}
	{
		ut::CTempFilePairPool pool( _T("foo 04.txt|foo 06.txt|pix_500|Xfoo 08.txt") );
		ASSERT_EQUAL( 7, ut::FindNextAvailSeqCount( pool, numFmt ) );
	}
	{
		ut::CTempFilePairPool pool( _T("xFoo 04.txt|xFoo 06.txt|xFoo 08.txt") );
		ASSERT_EQUAL( 9, ut::FindNextAvailSeqCount( pool, _T("x* #.txt") ) );
	}
}

void CPathGeneratorTests::TestWildcardGeneration( void )
{
	{
		ut::CPathPairPool pool( _T("a.txt|b.txt|c.txt") );
		ut::GeneratePathPairs( pool, _T("foo *.*") );
		ASSERT_EQUAL( _T("foo a.txt|foo b.txt|foo c.txt"), pool.JoinDest() );
	}
	{
		ut::CPathPairPool pool( _T("a.txt|b.txt|c.txt") );
		ut::GeneratePathPairs( pool, _T("foo *.do?") );
		ASSERT_EQUAL( _T("foo a.dot|foo b.dot|foo c.dot"), pool.JoinDest() );
	}
	{
		ut::CPathPairPool pool( _T("a.txt|b.txt|c.txt") );
		ut::GeneratePathPairs( pool, _T("foo *.doc") );
		ASSERT_EQUAL( _T("foo a.doc|foo b.doc|foo c.doc"), pool.JoinDest() );
	}
	{
		ut::CPathPairPool pool( _T("a.txt|b.txt|c.txt") );
		ut::GeneratePathPairs( pool, _T("*.H") );
		ASSERT_EQUAL( _T("a.H|b.H|c.H"), pool.JoinDest() );
	}
}

void CPathGeneratorTests::TestWildcardFileGeneration( void )
{
	// test with physical files pool
	{
		ut::CTempFilePairPool pool( _T("a.txt|b.txt|c.txt") );
		ut::GeneratePathPairs( pool, _T("foo *.*") );
		ASSERT_EQUAL( _T("foo a.txt|foo b.txt|foo c.txt"), pool.JoinDest() );
	}
	{
		ut::CTempFilePairPool pool( _T("a.txt|b.txt|c.txt") );
		ut::GeneratePathPairs( pool, _T("foo *.do?") );
		ASSERT_EQUAL( _T("foo a.dot|foo b.dot|foo c.dot"), pool.JoinDest() );
	}
	{
		ut::CTempFilePairPool pool( _T("a.txt|b.txt|c.txt") );
		ut::GeneratePathPairs( pool, _T("foo *.doc") );
		ASSERT_EQUAL( _T("foo a.doc|foo b.doc|foo c.doc"), pool.JoinDest() );
	}
	{
		ut::CTempFilePairPool pool( _T("a.txt|b.txt|c.txt") );
		ut::GeneratePathPairs( pool, _T("*.H") );
		ASSERT_EQUAL( _T("a.H|b.H|c.H"), pool.JoinDest() );
	}
}


void CPathGeneratorTests::Run( void )
{
	__super::Run();

	TestPathUniqueMaker();
	TestPathMaker();
	TestPathFormatter();
	TestNumSeqGeneration();
	TestNumSeqFileGeneration();
	TestFindNextAvailSeqCount();
	TestWildcardGeneration();
	TestWildcardFileGeneration();
}


#endif //USE_UT
