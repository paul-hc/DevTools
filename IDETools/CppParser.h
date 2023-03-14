#ifndef CppParser_h
#define CppParser_h
#pragma once

#include "utl/Range.h"
#include "utl/CodeAlgorithms.h"
#include "utl/Language.h"
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

	enum ExprType { ValueExpr, TypeExpr };

	template< typename IteratorT >
	IteratorT FindExpressionEnd( IteratorT itExpr, IteratorT itLast, ExprType exprType, const str::CSequenceSet<char>* pBreakSet = nullptr ) const;

	template< typename StringT >
	StringT ExtractTemplateInstance( const StringT& templateDecl, code::Spacing spacing = code::TrimSpace ) const throws_( code::TSyntaxError );

	template< typename StringT >
	StringT MakeRemoveDefaultParams( const StringT& proto, bool commentOut = true ) const throws_( code::TSyntaxError );
private:
	template< typename StringT >
	void AddArgListParams( StringT* pOutCode, const Range<typename StringT::const_iterator>& itArgs, bool commentOut, const str::CSequenceSet<char>* pBreakSet = nullptr ) const;

	template< typename OutIteratorT, typename IteratorT >
	void MakeTemplateInstance( OutIteratorT itOutTemplInst _out_, IteratorT itFirst, IteratorT itLast ) const;
public:
	const TLanguage& m_lang;

	enum ExpressionOp { ScopeOp, SelectorOp, PtrSelectorOp, AddressOp, DereferenceOp, _None = -1 };		// partial, it has all the operators

	static const str::CSequenceSet<char> s_exprOps;
	static const str::CSequenceSet<char> s_protoBreakWords;
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
