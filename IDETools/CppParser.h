#ifndef CppParser_h
#define CppParser_h
#pragma once

#include "utl/Language.h"
#include "utl/Range.h"
#include "TokenRange.h"
#include <map>


namespace pvt
{
	template< typename IteratorT >
	inline int Distance( IteratorT itFirst, IteratorT itLast ) { return static_cast<int>( std::distance( itFirst, itLast ) ); }


	// convert from iterator range to TokenRange:

	template< typename IteratorT >
	inline TokenRange MakeTokenRange( const Range<IteratorT>& itRange, IteratorT itBegin )
	{
		return TokenRange( str::MakePosRange( itRange, itBegin ) );
	}

	template< typename ContainerT >
	inline TokenRange MakeFwdTokenRange( const Range<typename ContainerT::const_reverse_iterator>& itRange, const ContainerT& items )
	{
		return TokenRange( str::MakeFwdPosRange( itRange, items ) );
	}
}


class CCppParser
{
public:
	typedef code::CLanguage<TCHAR> TLanguage;
public:
	CCppParser( void );

	template< typename StringT >
	StringT MakeRemoveDefaultParams( const StringT& proto, bool commentOut = true ) const throws_( CRuntimeException )
	{
		typedef typename StringT::const_iterator TIterator;

		StringT outCode;
		pred::IsChar isStartArgs( '(' );

		for ( TIterator it = proto.begin(), itEnd = proto.end(); it != itEnd; )
		{
			Range<TIterator> itArgs( m_lang.FindNextCharThat( it, itEnd, isStartArgs ) );

			outCode.insert( outCode.end(), it, itArgs.m_start );		// insert the code leading to the arg-list (if any, or the remining code)

			if ( itArgs.m_start != itEnd )
			{
				if ( !m_lang.SkipPastMatchingBracket( &itArgs.m_end, itEnd ) )
					throw CRuntimeException( str::Format( _T("Syntax error: function argument list not ended at: '%s'"), str::ToWide( &*itArgs.m_start ).c_str() ) );

				AddArgListParams( &outCode, itArgs, commentOut );
			}

			it = itArgs.m_end;		// skip to next argument-list, or the remaining code
		}

		return outCode;
	}

	template< typename IteratorT >
	IteratorT FindExpressionEnd( IteratorT itExpr, IteratorT itLast, const str::CSequenceSet<char>* pBreakSet = nullptr ) const
	{	// find the end of a C++ expression (e.g. that evaluates )
		const TLanguage::TSeparatorsPair& sepsPair = m_lang.GetParser().GetSeparators();
		str::CSequenceSet<char>::TSeqMatchPos seqMatch = utl::npos;
		pred::IsIdentifier isIdent;

		IteratorT itExprEnd = itExpr;

		while ( itExprEnd != itLast && *itExprEnd != ',' )
		{
			TLanguage::TSepMatchPos sepMatchPos;
			size_t numLength;

			m_lang.SkipWhitespace( &itExprEnd, itLast );		// skip leading whitespace

			if ( sepsPair.MatchesAnyOpenSepAt( &sepMatchPos, itExprEnd, itLast ) )
				sepsPair.SkipMatchingSpec( &itExprEnd, itLast, sepMatchPos );			// skip the C++ spec: cpp::CharQuote, cpp::StringQuote, cpp::Comment, cpp::LineComment
			else if ( code::IsStartBracket( *itExprEnd ) )
			{	// skip cast or function call
				if ( !m_lang.SkipPastMatchingBracket( &itExprEnd, itLast ) )
					 return itLast;		// syntax error: bracked not ended
			}
			else if ( s_exprOps.MatchesAnySequence( &seqMatch, itExprEnd, itLast ) )
				itExprEnd += s_exprOps.GetSequenceLength( seqMatch );
			else if ( pBreakSet != nullptr && pBreakSet->MatchesAnySequence( &seqMatch, itExprEnd, itLast ) )
				break;		// client-requested exclusion
			else if ( isIdent( *itExprEnd ) )
				m_lang.SkipIdentifier( &itExprEnd, itLast );
			else if ( cpp::IsValidNumericLiteral( &*itExprEnd, &numLength ) )
				itExprEnd += numLength;
			else
				break;		// end of expression
		}

		m_lang.SkipBackWhile( &itExprEnd, itExpr, pred::IsBlank() );		// skip backwards the trailing spaces
		return itExprEnd;
	}
private:
	template< typename StringT >
	void AddArgListParams( StringT* pOutCode, const Range<typename StringT::const_iterator>& itArgs, bool commentOut ) const
	{
		ASSERT_PTR( pOutCode );
		REQUIRE( itArgs.IsNonEmpty() );
		typedef typename StringT::const_iterator TIterator;

		pred::IsChar isAssignment( '=' );
		pred::IsIdentifier isIdentifier;

		for ( TIterator it = itArgs.m_start; it != itArgs.m_end; )
		{
			Range<TIterator> itValue( m_lang.FindNextCharThat( it, itArgs.m_end, isAssignment ) );		// parameter default value assignment

			pOutCode->insert( pOutCode->end(), it, itValue.m_start );			// insert the code leading to the assignment (if any, or the remining code)

			if ( itValue.m_start != itArgs.m_end )
			{
				++itValue.m_end;	// skip '='
				itValue.m_end = FindExpressionEnd( itValue.m_end, itArgs.m_end, &s_breakWords );

				if ( commentOut )
				{
					str::AppendSeqPtr( pOutCode, "/*" );
					pOutCode->insert( pOutCode->end(), itValue.m_start, itValue.m_end );	// insert the default value assignment
					str::AppendSeqPtr( pOutCode, "*/" );
				}
				else
				{
					m_lang.SkipBackWhile( &itValue.m_start, it, pred::IsBlank() );	// cut space before the '=' assignment
					m_lang.SkipWhitespace( &itValue.m_end, itArgs.m_end );			// cut space to next param separator

					if ( s_breakWords.MatchesAnySequence( nullptr, itValue.m_end, itArgs.m_end ) )
						--itValue.m_end;											// prevent eating the space separating the one of the break words

					if ( std::distance( itValue.m_end, itArgs.m_end ) > 1 )			// not the last parameter => avoid triming the "last )" trailing space
						str::TrimRight( *pOutCode );
				}
			}

			it = itValue.m_end;		// skip to the next parameter, or the remaining code
		}
	}
public:
	const TLanguage& m_lang;

	enum ExpressionOp { ScopeOp, SelectorOp, PtrSelectorOp, AddressOp, DereferenceOp, _None = -1 };

	static const str::CSequenceSet<char> s_exprOps;
	static const str::CSequenceSet<char> s_breakWords;
};


class CCppCodeParser : public CCppParser		// parsing methods on a given code string (by reference)
{
public:
	typedef int TPos;
	typedef std::tstring::const_iterator TConstIterator;

	CCppCodeParser( const std::tstring* pCodeText );	// use pointer to force by-reference semantics

	bool IsValidPos( TPos pos ) const { return static_cast<size_t>( pos ) < m_codeText.length(); }
	bool AtEnd( TPos pos ) const { return pos == m_length; }

	TPos FindPosNextSequence( TPos pos, const std::tstring& sequence ) const;		// -1 if not found
	bool FindNextSequence( TokenRange* pSeqRange _in_out_, TPos pos, const std::tstring& sequence ) const;

	TPos FindPosMatchingBracket( TPos bracketPos ) const;							// -1 if not found
	bool SkipPosPastMatchingBracket( TPos* pBracketPos _in_out_ ) const;
	bool FindArgList( TokenRange* pArgList _out_, TPos pos, TCHAR openBracket = s_anyBracket ) const;

	bool SkipWhitespace( TPos* pPos _in_out_ ) const;
	bool SkipAnyOf( TPos* pPos _in_out_, const TCHAR charSet[] );
	bool SkipAnyNotOf( TPos* pPos _in_out_, const TCHAR charSet[] );

	bool SkipMatchingToken( TPos* pPos _in_out_, const std::tstring& token );
private:
	const std::tstring& m_codeText;
public:
	const TPos m_length;
	TConstIterator m_itBegin;
	TConstIterator m_itEnd;

	static const TCHAR s_anyBracket = '\0';
};


class CCppMethodParser : public CCppParser
{
public:
	enum SliceType
	{
		IndentPrefix,			// whitespace before TemplateDecl
		TemplateDecl,			// "template< typename PathT, typename ObjectT >"
		InlineModifier,			// "inline"
		ReturnType,				// "std::pair<ObjectT*, cache::TStatusFlags>"
		QualifiedMethod,		// "CCacheLoader<PathT, ObjectT>::Acquire"
		FunctionName,			// "Acquire"
		ClassQualifier,			// "CCacheLoader<PathT, ObjectT>::"
		ArgList,				// "( const PathT& pathKey )"
		PostArgListSuffix		// " const throws(std::exception, std::runtime_error)"
	};

	CCppMethodParser( void );

	// split code into slices
	bool ParseCode( const std::tstring& codeText );						// split into m_codeSlices; returns true if no syntax error

	const TokenRange& LookupSliceRange( SliceType sliceType ) const;	// lookup slices in the split codeText
	bool HasSlice( SliceType sliceType ) const { return FindSliceRange( sliceType ) != nullptr; }
private:
	const TokenRange* FindSliceRange( SliceType sliceType ) const;

	typedef std::tstring::const_iterator TConstIterator;

	bool FindSliceEnd( TConstIterator* pItSlice _in_out_, const TConstIterator& itEnd ) const;
	void ParseQualifiedMethod( const std::tstring& codeText );
private:
	std::map<SliceType, TokenRange> m_codeSlices;
	TokenRange m_emptyRange;					// at the end of codeText

	static const std::string s_template;
	static const std::string s_inline;
	static const std::string s_operator;
	static const std::string s_callOp;
	static const std::tstring s_scopeOp;
};


#endif // CppParser_h
