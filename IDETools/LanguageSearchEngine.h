#ifndef LanguageSearchEngine_h
#define LanguageSearchEngine_h
#pragma once

#include "DocLanguage.h"
#include "StringUtilitiesEx.h"
#include "CodeUtilities.h"
#include "TokenRange.h"


namespace code
{
	enum CommentState { NoComment = -1, SingleLineComment, MultiLineComment };


	struct CommentTokens
	{
		const TCHAR* getOpenToken( CommentState commentState ) const;

		bool hasSingleLineComment( void ) const;
		bool hasMultiLineComment( void ) const;

		static const CommentTokens& getLanguageSpecific( DocLanguage m_docLanguage );
	public:
		const TCHAR* m_singleLineComment;
		const TCHAR* m_openComment;
		const TCHAR* m_closeComment;
	};


	template< typename IntegralType >
	struct FormattedNumber
	{
		FormattedNumber( IntegralType number, const CString& format )
			: m_number( number )
			, m_format( format )
		{
		}

		FormattedNumber( const FormattedNumber& source )
			: m_number( source.m_number )
			, m_format( source.m_format )
		{
		}

		CString formatString( void ) const
		{
			CString numberString;

			numberString.Format( m_format, m_number );
			return numberString;
		}
	public:
		IntegralType m_number;
		CString m_format;
	};


	/**
		Searches by skiping neutral language syntax elements such as quotes, remarks, etc.
		NOTE: All search methods return the end of the string position as null (NOT FOUND).
	*/
	class LanguageSearchEngine
	{
	public:
		LanguageSearchEngine( DocLanguage _docLanguage );
		~LanguageSearchEngine();

		// Attributes
		void setDocLanguage( DocLanguage _docLanguage );

		// Searching
		TokenRange findString( const TCHAR* pString, const TCHAR* subString, int startPos = 0,
							   str::CaseType caseType = str::Case ) const;
		int findOneOf( const TCHAR* pString, const TCHAR* charSet, int startPos,
					   str::CaseType caseType = str::Case ) const;
		int findNextWhitespace( const TCHAR* pString, int startPos = 0 ) const;
		int findNextNonWhitespace( const TCHAR* pString, int startPos = 0 ) const;
		TokenRange findQuotedString( const TCHAR* pString, int startPos = 0, str::CaseType caseType = str::Case,
									 const TCHAR* quoteSet = code::quoteChars ) const;
		TokenRange findComment( const TCHAR* pString, int startPos = 0, str::CaseType caseType = str::Case ) const;

		TokenRange findNextNumber( const TCHAR* text, int startPos = 0 );
		FormattedNumber< unsigned int > extractNumber( const TCHAR* text, int startPos = 0 ) throws_( CRuntimeException );

		// Token matching
		bool isTokenMatch( const TCHAR* pString, int pos, const TCHAR* token, bool skipFwdWhiteSpace = true ) const;
		bool isTokenMatchBefore( const TCHAR* pString, int pos, const TCHAR* token, bool skipBkwdWhiteSpace = true ) const;

		bool isCommentStatement( int& outStatementEndPos, const TCHAR* pString, int pos ) const;
		bool isSingleLineCommentStatement( const TCHAR* pString, int pos ) const;

		bool isCCastStatement( int& outStatementEndPos, const TCHAR* pString, int pos ) const;
		bool isUnicodePortableStringConstant( int& outStatementEndPos, const TCHAR* pString, int pos ) const;
		bool isProtectedLineTermination( int& outStatementEndPos, const TCHAR* pString, int pos ) const;
	public:
		// Implemented by concrete predicates that customize language code search
		interface IsMatchAtCursor
		{
			virtual bool operator()( const TCHAR* pCursor ) = 0;
		};

		bool findNextMatch( const TCHAR* pString, int startPos, IsMatchAtCursor& isMatchAtCursor ) const;
	public:
		// generic predicate search; it works with stuff like _istspace(), _istdigit(), etc
		//
		template< typename UnaryPredType >
		struct IsPredMatch : public LanguageSearchEngine::IsMatchAtCursor
		{
			IsPredMatch( const TCHAR* pString, UnaryPredType predicate, bool negatePred = false )
				: m_pString( pString ), m_predicate( predicate ), m_negatePred( negatePred ), m_outFoundPos( -1 ) {}

			virtual bool operator()( const TCHAR* pCursor )
			{
				if ( !!m_predicate( *pCursor ) != m_negatePred )
				{
					// found a match -> return the position
					m_outFoundPos = int( pCursor - m_pString );
					return true;
				}
				return false;
			}
		private:
			const TCHAR* m_pString;
			UnaryPredType m_predicate;
			bool m_negatePred;
		public:
			int m_outFoundPos;
		};

		template< typename UnaryPredType >
		int findIf( const TCHAR* pString, int startPos, UnaryPredType predicate ) const
		{
			ASSERT( pString != nullptr );

			IsPredMatch< UnaryPredType > predIsPredMatch( pString, predicate );

			if ( !findNextMatch( pString, startPos, predIsPredMatch ) )
				return str::Length( pString );

			return predIsPredMatch.m_outFoundPos;
		}

		template< typename UnaryPredType >
		int findIfNot( const TCHAR* pString, int startPos, UnaryPredType predicate ) const
		{
			ASSERT( pString != nullptr );

			IsPredMatch< UnaryPredType > predIsPredMatch( pString, predicate, true /*negate the predicate*/ );

			if ( !findNextMatch( pString, startPos, predIsPredMatch ) )
				return str::Length( pString );

			return predIsPredMatch.m_outFoundPos;
		}
	public:
		DocLanguage m_docLanguage;
	};


	// LanguageSearchEngine inline code

	inline int LanguageSearchEngine::findNextWhitespace( const TCHAR* pString, int startPos /*= 0*/ ) const
	{
		return findIf( pString, startPos, _istspace );
	}

	inline int LanguageSearchEngine::findNextNonWhitespace( const TCHAR* pString, int startPos /*= 0*/ ) const
	{
		return findIfNot( pString, startPos, _istspace );
	}

} // namespace code

#endif // LanguageSearchEngine_h
