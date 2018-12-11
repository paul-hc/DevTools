#ifndef MemLeakCheck_h
#define MemLeakCheck_h
#pragma once


#ifdef _DEBUG
	#define MEM_LEAK_START( varName )	CMemLeakCheck varName( _T( #varName ), __FILE__, __LINE__ )

	#define MEM_LEAK_CHECK( varName )	varName.Report( __LINE__ )
	#define MEM_LEAK_MESSAGE( varName )	varName.FormatMessage( __LINE__ )

	#define ASSERT_NO_LEAKS( varName )\
		do { std::tstring msg; _ASSERT_EXPR( varName.AssertReport( msg ), msg.c_str() ); } while( false )
#else
	#define MEM_LEAK_START( varName )
	#define MEM_LEAK_CHECK( varName )
	#define MEM_LEAK_MESSAGE( varName )
	#define ASSERT_NO_LEAKS( varName ) ( (void)0 )
#endif


#ifdef _DEBUG


#if defined( _MFC_VER )
	#define MFC_BASED
#endif

#ifndef MFC_BASED
	#define _CRTDBG_MAP_ALLOC		// define first
	#include <cstdlib>
	#include <crtdbg.h>

	#ifndef new
		#define new new( _NORMAL_BLOCK, __FILE__, __LINE__ )
	#endif
#endif


class CMemLeakCheck : private utl::noncopyable
{
public:
	CMemLeakCheck( const TCHAR* pVarName, const char* pFilename, int lineNo );
	~CMemLeakCheck() { Report(); }

	void Reset( void ) { m_total = GetHeapTotal(); }
	void Report( int lineNo = 0 ) const;
	bool AssertReport( std::tstring& rMsg ) const;

	size_t GetLeakedBytes( void ) const { return GetHeapTotal() - m_total; }
	std::tstring FormatMessage( int lineNo = 0 ) const;
protected:
	size_t GetHeapTotal( void ) const;
	std::tstring FormatLeak( size_t leakedBytes, bool final ) const;
private:
	std::tstring m_varName;
	std::tstring m_filename;
	int m_lineNo;
	size_t m_total;
#ifdef MFC_BASED
	mutable CMemoryState m_heapState;
#endif
};


#endif //_DEBUG


#endif // MemLeakCheck_h
