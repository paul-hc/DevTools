
#include "pch.h"
#include "DebugTrace.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


bool CTracing::m_disabled = false;				// by default enabled in Debug builds
bool CTracing::m_hResultDisabled = false;		// by default enabled in Debug builds


#ifdef _DEBUG

namespace debug
{
	void TraceV( const char* pFormat, va_list args )
	{
		if ( CTracing::m_disabled )
			return;

		CStringA text;

		text.FormatV( pFormat, args );
		::OutputDebugStringA( text );
	}

	void TraceV( const wchar_t* pFormat, va_list args )
	{
		if ( CTracing::m_disabled )
			return;

		CStringW text;

		text.FormatV( pFormat, args );
		::OutputDebugStringW( text );
	}


	// TRACE without the overhead: __FILE__, __LINE__, etc:

	void Trace( const char* pFormat, ... )
	{
		utl::CScopedLastError lastError;

		va_list args;
		va_start( args, pFormat );

		debug::TraceV( pFormat, args );
		va_end( args );
	}

	void Trace( const wchar_t* pFormat, ... )
	{
		utl::CScopedLastError lastError;

		va_list args;
		va_start( args, pFormat );

		debug::TraceV( pFormat, args );
		va_end( args );
	}


	namespace impl
	{
		void TraceStringsHeader( const char* pTitlePrefix, size_t itemCount, IN OUT size_t& rMaxCount )
		{
			std::string titleFormat, maxLimit;

			if ( !str::IsEmpty( pTitlePrefix ) )
			{
				titleFormat = pTitlePrefix;
				titleFormat += " - ";
			}

			titleFormat += "item_count=%d%s:\n";

			if ( rMaxCount != utl::npos )
				if ( rMaxCount < itemCount )		// are we reaching a display limit?
					maxLimit = str::Format( "  (max_items=%d)", rMaxCount ).c_str();
				else
					rMaxCount = utl::npos;	// no truncation in effect

			TRACE( titleFormat.c_str(), itemCount, maxLimit.c_str() );
		}
	}


	// CTraceFileLine implementation

	#define FILE_LINE_FORMAT "%s(%d):  %s"

	CTraceFileLine::CTraceFileLine( const char* pSrcFilePath, const int lineNo, size_t relativePathDepth /*= 0*/ )
		: m_pSrcFilePath( pSrcFilePath )
		, m_lineNo( lineNo )
	{
		if ( relativePathDepth != 0 )
			m_pSrcFilePath = FindParentRelativePath( pSrcFilePath, relativePathDepth );
	}

	void CTraceFileLine::operator()( const char* pFormat, ... ) const
	{
		utl::CScopedLastError lastError;
		CStringA fullFormat;

		fullFormat.Format( FILE_LINE_FORMAT, m_pSrcFilePath, m_lineNo, pFormat );

		va_list args;
		va_start( args, pFormat );

		debug::TraceV( fullFormat.GetString(), args );
		va_end( args );
	}

	void CTraceFileLine::operator()( const wchar_t* pFormat, ... ) const
	{
		utl::CScopedLastError lastError;
		CStringW fullFormat;

		fullFormat.Format( _T(FILE_LINE_FORMAT), str::FromUtf8( m_pSrcFilePath ).c_str(), m_lineNo, pFormat );

		va_list args;
		va_start( args, pFormat );

		debug::TraceV( fullFormat.GetString(), args );
		va_end( args );
	}

	const char* CTraceFileLine::FindParentRelativePath( const char* pSrcFilePath, size_t relativePathDepth /*= 2*/ )
	{
		ASSERT_PTR( pSrcFilePath );

		// reverse find past 2 path slashes to find the relative path from parent directory
		//
		typedef std::reverse_iterator<const char*> Tconst_reverse_iterator;

		Tconst_reverse_iterator pStartR = std::make_reverse_iterator( str::end( pSrcFilePath ) );
		Tconst_reverse_iterator pEndR = std::make_reverse_iterator( pSrcFilePath );

		for ( size_t slashCount = relativePathDepth; pStartR != pEndR && slashCount != 0; ++pStartR )
			if ( '\\' ==  *pStartR || '/' ==  *pStartR )
				if ( 0 == --slashCount )
					return &*pStartR + 1;		// after the second slash: parent directory + filename

		return pSrcFilePath;
	}
}

#endif // _DEBUG
