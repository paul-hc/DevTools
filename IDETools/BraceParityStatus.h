#ifndef BraceParityStatus_h
#define BraceParityStatus_h
#pragma once

#include <algorithm>
#include "DocLanguage.h"
#include "TokenRange.h"
#include "CodeParsingBase.h"


namespace code
{
	class BraceParityStatus
	{
	public:
		BraceParityStatus( void ) {}

		// high-level
		int findMatchingBracePos( const TCHAR* pCode, int openBracePos, DocLanguage docLanguage );
		int reverseFindMatchingBracePos( const TCHAR* pCode, int openBracePos, DocLanguage docLanguage );
		TokenRange findArgList( const TCHAR* pCode, int pos, const TCHAR* argListOpenBraces,
								DocLanguage docLanguage, bool allowUnclosedArgList = false );

		bool analyzeBraceParity( const TCHAR* pCode, DocLanguage docLanguage );

		bool filterOutOddBraces( CString& inOutOpenBraces ) const;

		void clear( void );

		// brace status
		bool isEntirelyEven( void ) const;

		bool hasBrace( TCHAR brace ) const { return findBrace( brace ) != NULL; }
		bool isBraceEven( TCHAR brace ) const;

		CString getOddBracesAsString( void ) const;

		CString getErrorMessage( const TCHAR* separator = _T("\r\n") ) const;
	private:
		struct BraceCounter;

		BraceCounter* findBrace( TCHAR brace ) const
		{
			std::vector<BraceCounter>::const_iterator itFound = std::find_if( m_braceCounters.begin(), m_braceCounters.end(), MatchesBrace( brace ) );

			return itFound != m_braceCounters.end() ? const_cast<BraceCounter*>( &*itFound ) : NULL;
		}

		BraceCounter* storeBrace( TCHAR brace );
		bool checkParityErrors( void );
	private:
		struct BraceCounter
		{
			BraceCounter( void );
			BraceCounter( TCHAR brace );

			bool IsEven( void ) const { return 0 == m_parityCounter; }
			void countBrace( TCHAR brace );
		public:
			TCHAR m_openBrace;
			TCHAR m_closeBrace;
			int m_parityCounter;
		};

		struct MatchesOpenBrace : public std::unary_function<const BraceCounter&, bool>
		{
			MatchesOpenBrace( TCHAR openBrace ) : m_openBrace( openBrace ) {}

			bool operator()( const BraceCounter& braceCounter ) const
			{
				return braceCounter.m_openBrace == m_openBrace;
			}
		public:
			TCHAR m_openBrace;
		};

		struct MatchesCloseBrace : public std::unary_function<const BraceCounter&, bool>
		{
			MatchesCloseBrace( TCHAR closeBrace ) : m_closeBrace( closeBrace ) {}

			bool operator()( const BraceCounter& braceCounter ) const
			{
				return braceCounter.m_closeBrace == m_closeBrace;
			}
		public:
			TCHAR m_closeBrace;
		};

		struct MatchesBrace : public std::unary_function<const BraceCounter&, bool>
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
		std::vector<BraceCounter> m_braceCounters;
		std::vector<CString> m_errorMessages;
	};

} // namespace code


namespace code
{
	template< typename PosT >
	void SkipSpace( PosT* pPos, const TCHAR* pCode )
	{
		ASSERT( pPos != nullptr && pCode != nullptr );
		REQUIRE( str::IsValidPos( *pPos, pCode ) );

		while ( pCode[ *pPos ] != 0 && _istspace( pCode[ *pPos ] ) )
			++*pPos;
	}

	template< typename PosT >
	void SkipSpaceBackwards( PosT* pPos, const TCHAR* pCode )
	{
		ASSERT( pPos != nullptr && pCode != nullptr );
		REQUIRE( str::IsValidPos( *pPos, pCode ) );

		while ( *pPos > 0 && _istspace( pCode[ *pPos - 1 ] ) )
			--*pPos;
	}


	// brace lookup helpers

	template< typename PosT >
	bool SkipBrace( PosT* pOutCloseBracePos, const TCHAR* pCode, PosT openBracePos, DocLanguage docLanguage = DocLang_Cpp )
	{
		ASSERT( pOutCloseBracePos != nullptr && pCode != nullptr );
		REQUIRE( str::IsValidPos( openBracePos, pCode ) && IsOpenBrace( pCode[ openBracePos ] ) );

		int closeBracePos = BraceParityStatus().findMatchingBracePos( pCode, static_cast<int>( openBracePos ), docLanguage );
		if ( -1 == closeBracePos )
			return false;		// matching brace not found (bad syntax)

		ENSURE( closeBracePos > openBracePos && str::IsValidPos( closeBracePos, pCode ) && IsCloseBrace( pCode[ closeBracePos ] ) );

		*pOutCloseBracePos = static_cast<PosT>( closeBracePos );
		return true;			// found matching brace
	}

	template< typename PosT >
	bool SkipBraceBackwards( PosT* pOutOpenBracePos, const TCHAR* pCode, PosT closeBracePos, DocLanguage docLanguage = DocLang_Cpp )
	{
		ASSERT( pOutOpenBracePos != nullptr && pCode != nullptr );
		REQUIRE( str::IsValidPos( closeBracePos, pCode ) && IsCloseBrace( pCode[ closeBracePos ] ) );

		int openBracePos = BraceParityStatus().reverseFindMatchingBracePos( pCode, static_cast<int>( closeBracePos ), docLanguage );
		if ( -1 == openBracePos )
			return false;		// matching brace not found (bad syntax)

		ENSURE( openBracePos < closeBracePos && str::IsValidPos( openBracePos, pCode ) && IsOpenBrace( pCode[ openBracePos ] ) );

		*pOutOpenBracePos = static_cast<PosT>( openBracePos );
		return true;			// found matching brace
	}
}


#endif // BraceParityStatus_h
