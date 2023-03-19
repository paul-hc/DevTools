#ifndef CppParser_hxx
#define CppParser_hxx
#pragma once


// CCppParser template methods

template< typename IteratorT >
IteratorT CCppParser::FindExpressionEnd( IteratorT itExpr, IteratorT itLast, ExprType exprType, const str::CSequenceSet<char>* pBreakSet /*= nullptr*/ ) const
{	// find the end of a C++ expression (that evaluates to a value), or a qualified type;  uses pBreakSet for specialized exclusions per client usage.
	const TLanguage::TSeparatorsPair& sepsPair = m_lang.GetParser().GetSeparators();
	str::CSequenceSet<char>::TSeqMatchPos seqMatch = utl::npos;
	pred::IsIdentifier isIdent;

	IteratorT itExprEnd = itExpr;
	while ( itExprEnd != itLast )
	{
		TLanguage::TSepMatchPos sepMatchPos;
		size_t numLength;

		if ( exprType != TypeExpr )
		{
			m_lang.SkipWhitespace( &itExprEnd, itLast );		// skip leading whitespace (only for value expressions)

			if ( itExprEnd == itLast )
				break;					// reached the end of source after whitespaces
		}

		if ( sepsPair.MatchesAnyOpenSepAt( &sepMatchPos, itExprEnd, itLast ) )
			sepsPair.SkipMatchingSpec( &itExprEnd, itLast, sepMatchPos );			// skip the C++ spec: CharQuote, StringQuote, Comment, LineComment
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

template<typename StringT>
StringT CCppParser::ExtractTemplateInstance( const StringT& templateDecl, code::Spacing spacing /*= code::TrimSpace*/ ) const throws_( code::TSyntaxError )
{
	// converts "template< typename Type, class MyClass, struct MyStruct, enum MyEnum, int level >" to "<Type, MyClass, MyStruct, MyEnum, level>"
	typedef typename StringT::const_iterator TIterator;

	StringT outTemplInst;
	TIterator it = templateDecl.begin(), itEnd = templateDecl.end();
	Range<TIterator> itArgs( m_lang.FindNextChar( it, itEnd, '<' ) );

	if ( itArgs.m_start != itEnd )
	{
		if ( !m_lang.SkipPastMatchingBracket( &itArgs.m_end, itEnd ) )
			throw code::TSyntaxError( str::Format( "Syntax error: template argument list not ended at: '%s'", str::ValueToString<std::string>( &*itArgs.m_start ).c_str() ), UTL_FILE_LINE );

		// shrink to trim-out the '<' and '>' to template list contents
		++itArgs.m_start;
		--itArgs.m_end;

		// remove default types e.g. "template< typename CharT, typename TraitsT = std::char_traits<CharT> >" => "template< typename CharT, typename TraitsT >"
		StringT typeList;
		AddArgListParams( &typeList, itArgs, false );

		MakeTemplateInstance( std::back_inserter( outTemplInst ), typeList.begin(), typeList.end());
	}

	if ( !outTemplInst.empty() )
		code::EncloseCode( &outTemplInst, "<", ">", spacing );

	return outTemplInst;
}

template< typename OutIteratorT, typename IteratorT >
void CCppParser::MakeTemplateInstance( OUT OutIteratorT itOutTemplInst, IteratorT itFirst, IteratorT itLast ) const
{
	// for each template argument, strip the typename: "typename T" -> "T", or the non-type "int depth" -> "depth"
	TLanguage::CScopedSkipArgLists skipArgLists( &m_lang );		// to skip template templates with nested arg-lists

	for ( IteratorT it = itFirst; it != itLast; )
	{
		Range<IteratorT> itTypename( it );
		m_lang.SkipWhitespace( &itTypename.m_start, itLast );

		if ( itTypename.m_start != itLast )
		{	// typename|class|struct|union|enum, or any non-type params such as "int" or "std::basic_string<wchar_t>"
			itTypename.m_end = FindExpressionEnd( itTypename.m_start, itLast, TypeExpr );
			m_lang.SkipWhitespace( &itTypename.m_end, itLast );
		}

		std::copy( it, itTypename.m_start, itOutTemplInst );		// add the code leading to "typename T", but strip "typename "

		Range<IteratorT> itName( itTypename.m_end );
		m_lang.SkipIdentifier( &itName.m_end, itLast );

		std::copy( itName.m_start, itName.m_end, itOutTemplInst );	// add the "T", or "depth"

		if ( itName.m_end != itLast )
		{
			it = m_lang.FindNextChar( itName.m_end, itLast, ',' );
			if ( it != itLast )
				std::copy( itName.m_end, ++it, itOutTemplInst );	// add the ","
		}
		else
			it = itName.m_end;		// the end, really
	}
}

template< typename StringT >
StringT CCppParser::MakeRemoveDefaultParams( const StringT& proto, bool commentOut /*= true*/ ) const throws_( code::TSyntaxError )
{
	typedef typename StringT::const_iterator TIterator;

	StringT outCode;

	for ( TIterator it = proto.begin(), itEnd = proto.end(); it != itEnd; )
	{
		Range<TIterator> itArgs( m_lang.FindNextChar( it, itEnd, '(' ) );

		outCode.insert( outCode.end(), it, itArgs.m_start );		// insert the code leading to the arg-list (if any, or the remining code)

		if ( itArgs.m_start != itEnd )
		{
			if ( !m_lang.SkipPastMatchingBracket( &itArgs.m_end, itEnd ) )
				throw code::TSyntaxError( str::Format( "Syntax error: function argument list not ended at: '%s'", str::ValueToString<std::string>( &*itArgs.m_start ).c_str() ), UTL_FILE_LINE );

			AddArgListParams( &outCode, itArgs, commentOut, &s_protoBreakWords );
		}

		it = itArgs.m_end;		// skip to next argument-list, or the remaining code
	}

	return outCode;
}

template< typename StringT >
void CCppParser::AddArgListParams( StringT* pOutCode, const Range<typename StringT::const_iterator>& itArgs, bool commentOut,
								   const str::CSequenceSet<char>* pBreakSet /*= nullptr*/ ) const
{
	ASSERT_PTR( pOutCode );
	REQUIRE( itArgs.IsNonEmpty() );
	typedef typename StringT::const_iterator TIterator;

	for ( TIterator it = itArgs.m_start; it != itArgs.m_end; )
	{
		Range<TIterator> itValue( m_lang.FindNextChar( it, itArgs.m_end, '=' ) );		// parameter default value assignment

		pOutCode->insert( pOutCode->end(), it, itValue.m_start );				// insert the code leading to the assignment (if any, or the remining code)

		if ( itValue.m_start != itArgs.m_end )
		{
			++itValue.m_end;	// skip '='
			itValue.m_end = FindExpressionEnd( itValue.m_end, itArgs.m_end, ValueExpr, pBreakSet );

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

				if ( itValue.m_end != itArgs.m_end )
					if ( s_protoBreakWords.MatchesAnySequence( nullptr, itValue.m_end, itArgs.m_end ) )
						--itValue.m_end;										// prevent eating the space separating the one of the break words

				if ( std::distance( itValue.m_end, itArgs.m_end ) > 1 )			// not the last parameter => avoid triming the "last )" trailing space
					str::TrimRight( *pOutCode );
			}
		}

		it = itValue.m_end;		// skip to the next parameter, or the remaining code
	}
}


#endif // CppParser_hxx
