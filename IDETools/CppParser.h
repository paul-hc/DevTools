#ifndef CppParser_h
#define CppParser_h
#pragma once

#include "utl/CodeParsing.h"
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

	CCppParser( void );
public:
	const TLanguage& m_lang;
};


class CCppCodeParser : public CCppParser		// parsing methods on a given code string (by reference)
{
public:
	typedef int TPos;

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
	typedef std::tstring::const_iterator TConstIterator;

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

	static const std::tstring s_template;
	static const std::tstring s_inline;
	static const std::tstring s_operator;
	static const std::tstring s_scopeOp;
	static const std::tstring s_callOp;
};


#endif // CppParser_h
