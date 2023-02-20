#ifndef CodeParser_h
#define CodeParser_h
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


class CCppCodeParser
{
public:
	enum SliceType
	{
		TemplateDecl,			// "template< typename PathT, typename ObjectT >"
		InlineModifier,			// "inline"
		ReturnType,				// "std::pair<ObjectT*, cache::TStatusFlags>"
		QualifiedMethod,		// "CCacheLoader<PathT, ObjectT>::Acquire"
		FunctionName,			// "Acquire"
		ClassQualifier,			// "CCacheLoader<PathT, ObjectT>::"
		ArgList,				// "( const PathT& pathKey )"
		PostArgListSuffix,		// " const throws(std::exception, std::runtime_error)"
			_Unknown
	};

	CCppCodeParser( void );

	// split code into slices
	bool ParseCode( const std::tstring& codeText );						// split into m_codeSlices; returns true if no syntax error

	const TokenRange& LookupSliceRange( SliceType sliceType ) const;	// lookup slices in the split codeText
	bool HasSlice( SliceType sliceType ) const { return FindSliceRange( sliceType ) != nullptr; }
private:
	const TokenRange* FindSliceRange( SliceType sliceType ) const;

	typedef std::tstring::const_iterator TConstIterator;

	bool FindSliceEnd( TConstIterator* pItSlice /*in-out*/, const TConstIterator& itEnd ) const;
	void ParseQualifiedMethod( const std::tstring& codeText );
private:
	const code::CLanguage<TCHAR>& m_lang;
	std::map<SliceType, TokenRange> m_codeSlices;
	TokenRange m_emptyRange;					// at the end of codeText

	static const std::tstring s_template;
	static const std::tstring s_inline;
	static const std::tstring s_operator;
	static const std::tstring s_scopeOp;
	static const std::tstring s_callOp;
};


#endif // CodeParser_h
