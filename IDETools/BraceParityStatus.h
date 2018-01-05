#ifndef BraceParityStatus_h
#define BraceParityStatus_h
#pragma once

#include <algorithm>
#include "DocLanguage.h"
#include "TokenRange.h"


namespace code
{
	class BraceParityStatus
	{
	public:
		BraceParityStatus( void );
		~BraceParityStatus();

		// high-level
		int findMatchingBracePos( const TCHAR* pStr, int openBracePos, DocLanguage docLanguage );
		int reverseFindMatchingBracePos( const TCHAR* pStr, int openBracePos, DocLanguage docLanguage );
		TokenRange findArgList( const TCHAR* codeText, int pos, const TCHAR* argListOpenBraces,
								DocLanguage docLanguage, bool allowUnclosedArgList = false );

		bool analyzeBraceParity( const TCHAR* pStr, DocLanguage docLanguage );

		bool filterOutOddBraces( CString& inOutOpenBraces ) const;

		void clear( void );

		// brace status
		bool isEntirelyEven( void ) const;

		bool hasBrace( TCHAR brace ) const;
		bool isBraceEven( TCHAR brace ) const;

		CString getOddBracesAsString( void ) const;

		CString getErrorMessage( const TCHAR* separator = _T("\r\n") ) const;
	private:
		struct BraceCounter;

		BraceCounter* findBrace( TCHAR brace ) const;

		BraceCounter* storeBrace( TCHAR brace );
		bool checkParityErrors( void );
	private:
		struct BraceCounter
		{
			BraceCounter( void );
			BraceCounter( TCHAR brace );

			bool IsEven( void ) const;
			void countBrace( TCHAR brace );
		public:
			TCHAR m_openBrace;
			TCHAR m_closeBrace;
			int m_parityCounter;
		};

		struct MatchesOpenBrace : public std::unary_function< const BraceCounter&, bool >
		{
			MatchesOpenBrace( TCHAR openBrace ) : m_openBrace( openBrace ) {}

			bool operator()( const BraceCounter& braceCounter ) const
			{
				return braceCounter.m_openBrace == m_openBrace;
			}
		public:
			TCHAR m_openBrace;
		};

		struct MatchesCloseBrace : public std::unary_function< const BraceCounter&, bool >
		{
			MatchesCloseBrace( TCHAR closeBrace ) : m_closeBrace( closeBrace ) {}

			bool operator()( const BraceCounter& braceCounter ) const
			{
				return braceCounter.m_closeBrace == m_closeBrace;
			}
		public:
			TCHAR m_closeBrace;
		};

		struct MatchesBrace : public std::unary_function< const BraceCounter&, bool >
		{
			MatchesBrace( TCHAR brace ) : m_brace( brace ) {}

			bool operator()( const BraceCounter& braceCounter ) const
			{
				return braceCounter.m_openBrace == m_brace || braceCounter.m_closeBrace == m_brace;
			}
		public:
			TCHAR m_brace;
		};
	private:
		std::vector< BraceCounter > braceCounters;
		std::vector< CString > errorMessages;
	};

} // namespace code


// inline code
namespace code
{
	inline BraceParityStatus::BraceCounter* BraceParityStatus::findBrace( TCHAR brace ) const
	{
		std::vector< BraceCounter >::const_iterator itFound = std::find_if( braceCounters.begin(), braceCounters.end(),
																			MatchesBrace( brace ) );

		return itFound != braceCounters.end() ? const_cast< BraceCounter* >( &*itFound ) : NULL;
	}

	inline bool BraceParityStatus::hasBrace( TCHAR brace ) const
	{
		return findBrace( brace ) != NULL;
	}

	// BraceParityStatus::BraceCounter inline code

	inline bool BraceParityStatus::BraceCounter::IsEven( void ) const
	{
		return m_parityCounter == 0;
	}

} // namespace code


#endif // BraceParityStatus_h
