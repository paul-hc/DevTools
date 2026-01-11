#ifndef DebugTrace_h
#define DebugTrace_h
#pragma once

#include "StringBase.h"


struct CTracing
{
	static bool m_disabled;				// by default enabled
	static bool m_hResultDisabled;
};


#ifdef _DEBUG

namespace debug
{
	void Trace( const char* pFormat, ... );
	void Trace( const wchar_t* pFormat, ... );


	namespace impl
	{
		void TraceStringsHeader( const char* pTitlePrefix, size_t itemCount, IN OUT size_t& rMaxCount );
	}

	template< typename VectLikeT >
	void TraceStrings( const VectLikeT& textItems, const char* pTitlePrefix = nullptr, size_t maxCount = utl::npos )
	{
		typedef typename VectLikeT::value_type TStringy;
		typedef typename TStringy::value_type TChar;

		size_t itemCount = textItems.size();
		impl::TraceStringsHeader( pTitlePrefix, itemCount, maxCount );

		std::basic_string<TChar> itemFormat;
		str::AppendAnsi( itemFormat, "\t[%d]\t%s\n" );

		size_t i = 0, lastPos = itemCount - 1;
		for ( typename VectLikeT::const_iterator itItem = textItems.begin(); itItem != textItems.end(); )
		{
			Trace( itemFormat.c_str(), i, str::traits::GetCharPtr( *itItem ) );
			++itItem;

			if ( ++i == maxCount - 1 )		// reached the truncation limit?
			{	// trace "..." text and jump to tracing the last item:
				Trace( "\t...\n" );
				itItem = textItems.end() - 1;
				i = lastPos;
			}
		}
	}


	class CTraceFileLine : private utl::noncopyable
	{
	public:
		CTraceFileLine( const char* pSrcFilePath, const int lineNo, size_t relativePathDepth = 0 );
		~CTraceFileLine() {}

		void operator()( const char* pFormat, ... ) const;
		void operator()( const wchar_t* pFormat, ... ) const;
	private:
		static const char* FindParentRelativePath( const char* pSrcFilePath, size_t relativePathDepth = 2 );
	private:
		const char* m_pSrcFilePath;
		const int m_lineNo;
	};
}

#endif // _DEBUG


#endif // DebugTrace_h
