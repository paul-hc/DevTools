
#include "stdafx.h"
#include "MemLeakCheck.h"


#ifdef _DEBUG


// CMemLeakCheck implementation

CMemLeakCheck::CMemLeakCheck( const TCHAR* pVarName, const char* pFilename, int lineNo )
	: m_varName( pVarName )
	, m_filename( str::FromAnsi( pFilename ) )
	, m_lineNo( lineNo )
	, m_total( 0 )
{
	Reset();
}

size_t CMemLeakCheck::GetHeapTotal( void ) const
{
#ifdef MFC_BASED
	m_heapState.Checkpoint();
	return m_heapState.m_lSizes[ _NORMAL_BLOCK ];
#else
	_CrtMemState heapState;
	_CrtMemCheckpoint( &heapState );
	return heapState.lSizes[ _NORMAL_BLOCK ];
#endif
}

void CMemLeakCheck::Report( int lineNo /*= 0*/ ) const
{
	std::tstring message = FormatMessage( lineNo );
	if ( !message.empty() )
		TRACE( _T("\n %s\n"), message.c_str() );
}

bool CMemLeakCheck::AssertReport( std::tstring& rMsg ) const
{
	if ( size_t leakedBytes = GetLeakedBytes() )
	{
		Report();
		rMsg = FormatLeak( leakedBytes, true );
		return false;
	}

	return true;
}

std::tstring CMemLeakCheck::FormatMessage( int lineNo /*= 0*/ ) const
{
	// output format:
	//	C:\dev\DevTools\CommonUtl\utl\Color.cpp(81): checkpoint '%s' - 12 bytes LEAKED.

	if ( size_t leakedBytes = GetLeakedBytes() )
		return str::Format( _T("%s(%d): %s."),
			m_filename.c_str(),
			lineNo != 0 ? lineNo : m_lineNo,
			FormatLeak( leakedBytes, 0 == lineNo ).c_str() );		// lineNo is 0 when called on destructor (final report)

	return std::tstring();
}

std::tstring CMemLeakCheck::FormatLeak( size_t leakedBytes, bool isFinal ) const
{
	if ( leakedBytes != 0 )
		return str::Format( _T("heap checkpoint '%s' - %lu bytes %s"),
			m_varName.c_str(),
			leakedBytes,
			isFinal ? _T("LEAKED") : _T("ALIVE") );

	return std::tstring();
}


#endif
