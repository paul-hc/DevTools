
#include "pch.h"
#include "CppParser.h"
#include "utl/Algorithms.h"
#include "utl/RuntimeException.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/Language.hxx"


// CCppParser implementation

const str::CSequenceSet<char> CCppParser::s_exprOps(
	":: . -> & *"							// scope, member access, address, dereference
	" += -= *= /= %= >>= <<= &= ^= |="		// compound assignment
	" == != >= <= > <"						// comparison
	" ++ -- + - / %"						// in/de-crement, arithmetic
	" ! && ||"								// logical
	" | ^ ~ << >>"							// bitwise operators
	" ? :"									// conditional ternary
	//" sizeof"								// redundant, captured by pred::IsIdenifier()
	, " " );

const str::CSequenceSet<char> CCppParser::s_protoBreakWords( "_in_|_out_|_in_out_" );

CCppParser::CCppParser( void )
	: m_lang( code::GetLangCpp<TCHAR>() )
{
}


// CCppCodeParser implementation

CCppCodeParser::CCppCodeParser( const std::tstring* pCodeText )
	: CCppParser()
	, m_codeText( *safe_ptr( pCodeText ) )
	, m_length( static_cast<TPos>( m_codeText.length() ) )
	, m_itBegin( m_codeText.begin() )
	, m_itEnd( m_codeText.end() )
{
}

CCppCodeParser::TPos CCppCodeParser::FindPosNextSequence( TPos pos, const std::tstring& sequence ) const
{
	ASSERT( IsValidPos( pos ) );
	ASSERT( !sequence.empty() );

	TConstIterator itFound = m_lang.FindNextSequence( m_itBegin + pos, m_itEnd, sequence );

	if ( m_itEnd == itFound )
		return -1;

	TPos foundPos = pvt::Distance( m_itBegin, itFound );
	ENSURE( IsValidPos( foundPos ) );
	return foundPos;
}

bool CCppCodeParser::FindNextSequence( TokenRange* pSeqRange _out_, TPos pos, const std::tstring& sequence ) const
{
	ASSERT_PTR( pSeqRange );
	ASSERT( IsValidPos( pos ) );

	pSeqRange->SetEmptyRange( FindPosNextSequence( pos, sequence ) );
	if ( pSeqRange->m_start == m_length )
		return false;

	pSeqRange->m_end += static_cast<TPos>( sequence.length() );
	return true;
}

CCppCodeParser::TPos CCppCodeParser::FindPosMatchingBracket( TPos bracketPos ) const
{
	ASSERT( IsValidPos( bracketPos ) );
	TConstIterator itCloseBracket = m_lang.FindMatchingBracket( m_itBegin + bracketPos, m_itEnd );

	if ( m_itEnd == itCloseBracket )
		return -1;

	return pvt::Distance( m_itBegin, itCloseBracket );
}

bool CCppCodeParser::SkipPosPastMatchingBracket( TPos* pBracketPos _in_out_ ) const
{
	ASSERT_PTR( pBracketPos );
	ASSERT( IsValidPos( *pBracketPos ) );

	TConstIterator it = m_itBegin + *pBracketPos;

	if ( !m_lang.SkipPastMatchingBracket( &it, m_itEnd ) )
		return false;

	*pBracketPos = pvt::Distance( m_itBegin, it );
	return true;
}

bool CCppCodeParser::FindArgList( TokenRange* pArgList _out_, TPos pos, TCHAR openBracket /*= s_anyBracket*/ ) const
{
	ASSERT_PTR( pArgList );
	ASSERT( IsValidPos( pos ) );

	TConstIterator itOpenBracket = s_anyBracket == openBracket
		? m_lang.FindNextCharThat( m_itBegin + pos, m_itEnd, pred::IsBracket() )
		: m_lang.FindNextCharThat( m_itBegin + pos, m_itEnd, pred::IsChar( openBracket ) );

	if ( itOpenBracket == m_itEnd )
		return false;				// no opening bracket found

	TConstIterator it = itOpenBracket;

	if ( !m_lang.SkipPastMatchingBracket( &it, m_itEnd ) )
		return false;				// no matching closing bracket found

	*pArgList = pvt::MakeTokenRange( Range<TConstIterator>( itOpenBracket, it ), m_itBegin );
	return true;
}

bool CCppCodeParser::SkipWhitespace( TPos* pPos _in_out_ ) const
{
	ASSERT_PTR( pPos );
	ASSERT( IsValidPos( *pPos ) );

	TConstIterator it = m_itBegin + *pPos;
	if ( !m_lang.SkipWhitespace( &it, m_itEnd ) )
		return false;

	*pPos = pvt::Distance( m_itBegin, it );
	return true;
}

bool CCppCodeParser::SkipMatchingToken( TPos* pPos _in_out_, const std::tstring& token )
{
	ASSERT_PTR( pPos );
	ASSERT( IsValidPos( *pPos ) );

	if ( !str::EqualsSeq( m_itBegin + *pPos, m_itEnd, token ) )
		return false;

	*pPos += token.length();
	SkipWhitespace( pPos );
	return true;
}

bool CCppCodeParser::SkipAnyOf( TPos* pPos _in_out_, const TCHAR charSet[] )
{
	ASSERT_PTR( pPos );
	ASSERT( IsValidPos( *pPos ) );

	TPos oldPos = *pPos;
	while ( *pPos != m_length && str::IsAnyOf( m_codeText[ *pPos ], charSet ) )
		++*pPos;

	return *pPos != oldPos;
}

bool CCppCodeParser::SkipAnyNotOf( TPos* pPos _in_out_, const TCHAR charSet[] )
{
	ASSERT_PTR( pPos );
	ASSERT( IsValidPos( *pPos ) );

	TPos oldPos = *pPos;
	while ( *pPos != m_length && !str::IsAnyOf( m_codeText[ *pPos ], charSet ) )
		++*pPos;

	return *pPos != oldPos;
}


// CCppMethodParser implementation

const std::string CCppMethodParser::s_template = "template";
const std::string CCppMethodParser::s_inline = "inline";
const std::string CCppMethodParser::s_operator = "operator";
const std::string CCppMethodParser::s_callOp = "()";
const std::tstring CCppMethodParser::s_scopeOp = _T("::");

CCppMethodParser::CCppMethodParser( void )
	: CCppParser()
{
}

bool CCppMethodParser::ParseCode( const std::tstring& codeText )
{
	m_codeSlices.clear();
	m_emptyRange.SetEmptyRange( codeText.length() );

	TConstIterator itCode = codeText.begin(), itEnd = codeText.end(), it = itCode;
	const TLanguage::TSeparatorsPair& sepsPair = m_lang.GetParser().GetSeparators();
	typedef Range<TConstIterator> TIteratorRange;

	for ( ; it != itEnd; )
	{
		m_lang.SkipWhitespace( &it, itEnd );
		if ( it == itEnd )
			break;							// ending in whitespace

		if ( !HasSlice( IndentPrefix ) && it != itCode )	// found leading whitespace?
			m_codeSlices[ IndentPrefix ] = pvt::MakeTokenRange( TIteratorRange( itCode, it ), itCode );

		TConstIterator itSlice = it;		// beginning of a potential slice
		TLanguage::TSepMatchPos sepMatchPos;

		if ( sepsPair.MatchesAnyOpenSepAt( &sepMatchPos, it, itEnd ) )	// matches a quoted string, comment, etc?
			sepsPair.SkipMatchingSpec( &it, itEnd, sepMatchPos );		// skip quoted strings, comments, etc
		else if ( !HasSlice( TemplateDecl ) && str::EqualsSeq( it, itEnd, s_template ) )
		{
			it += s_template.length();
			m_lang.SkipWhitespace( &it, itEnd );

			if ( it == itEnd || *it != '<' )
				return false;					// syntax error: template argument list missing

			m_lang.SkipPastMatchingBracket( &it, itEnd );
			m_codeSlices[ TemplateDecl ] = pvt::MakeTokenRange( TIteratorRange( itSlice, it ), itCode );
		}
		else if ( !HasSlice( InlineModifier ) && str::EqualsSeq( it, itEnd, s_inline ) )
		{
			it += s_inline.length();
			m_codeSlices[ InlineModifier ] = pvt::MakeTokenRange( TIteratorRange( itSlice, it ), itCode );
		}
		else if ( FindSliceEnd( &it, itEnd ) )
		{
			if ( '(' == *itSlice )				// is slice an args-list?
			{
				if ( !HasSlice( ArgList ) )
				{
					m_codeSlices[ ArgList ] = pvt::MakeTokenRange( TIteratorRange( itSlice, it ), itCode );
					m_codeSlices[ PostArgListSuffix ] = pvt::MakeTokenRange( TIteratorRange( it, itEnd ), itCode );

					if ( !HasSlice( ReturnType ) )
						m_codeSlices[ ReturnType ] = m_emptyRange;		// too late for a return type, supress it to prevent inadvertent insertion
				}
			}
			else if ( it != itEnd && '(' == *it )		// is slice ending-on an args-list?
			{
				if ( !HasSlice( QualifiedMethod ) )
					m_codeSlices[ QualifiedMethod ] = pvt::MakeTokenRange( TIteratorRange( itSlice, it ), itCode );
			}
			else if ( !HasSlice( QualifiedMethod ) &&
					  utl::Distance<size_t>( itSlice, it ) > s_scopeOp.length() &&
					  str::EqualsSeq( it - s_scopeOp.length(), it, s_scopeOp ) )		// is slice ending-on "::" (truncated type-qualifier)?
				m_codeSlices[ QualifiedMethod ] = pvt::MakeTokenRange( TIteratorRange( itSlice, it ), itCode );
			else if ( !HasSlice( ReturnType ) )
				m_codeSlices[ ReturnType ] = pvt::MakeTokenRange( TIteratorRange( itSlice, it ), itCode );
		}
		else
			++it;
	}

	ParseQualifiedMethod( codeText );
	return true;
}

void CCppMethodParser::ParseQualifiedMethod( const std::tstring& codeText )
{	// extract FunctionName, ClassQualifier, adjust ReturnType from QualifiedMethod
	REQUIRE( !HasSlice( FunctionName ) && !HasSlice( ClassQualifier ) );

	if ( const TokenRange* pQualifiedMethod = FindSliceRange( QualifiedMethod ) )
	{
		ASSERT( !pQualifiedMethod->IsEmpty() );
		size_t posScopeOp = codeText.rfind( s_scopeOp, pQualifiedMethod->m_end );

		if ( pQualifiedMethod->Contains( posScopeOp ) )
		{
			posScopeOp += s_scopeOp.length();
			m_codeSlices[ ClassQualifier ] = TokenRange( pQualifiedMethod->m_start, posScopeOp );

			if ( posScopeOp < codeText.length() )			// is there a function name?  (i.e. not the case "int CFile::")
				m_codeSlices[ FunctionName ] = TokenRange( posScopeOp, pQualifiedMethod->m_end );
		}
		else
			m_codeSlices[ FunctionName ] = *pQualifiedMethod;

		if ( TokenRange* pReturnType = utl::FindValuePtr( m_codeSlices, ReturnType ) )		// we have a return type, which may be partial?
			if ( !pReturnType->IsEmpty() )
			{	// cover the whole return type for cases such as "const TCHAR*" - originally stored as "const"
				int posRetEnd = pQualifiedMethod->m_start;

				while ( posRetEnd >= pReturnType->m_end && pred::IsSpace()( codeText[ posRetEnd - 1 ] ) )
					--posRetEnd;

				pReturnType->m_end = posRetEnd;
			}
	}
}

bool CCppMethodParser::FindSliceEnd( TConstIterator* pItSlice _in_out_, const TConstIterator& itEnd ) const
{
	ASSERT_PTR( pItSlice );

	TConstIterator it = *pItSlice;

	if ( '(' == *it )			// beginning of an args-list?
		m_lang.SkipPastMatchingBracket( &it, itEnd );		// argument list is a slice in itself
	else
		while ( it != itEnd && !pred::IsSpace()( *it ) && *it != '(' )
		{
			if ( '<' == *it )
				m_lang.SkipPastMatchingBracket( &it, itEnd );
			else if ( str::EqualsSeq( it, itEnd, s_scopeOp ) )
				it += s_scopeOp.length();
			else if ( str::EqualsSeq( it, itEnd, s_operator ) )
			{
				it += s_operator.length();
				m_lang.SkipWhitespace( &it, itEnd );

				if ( str::EqualsSeq( it, itEnd, s_callOp ) )
					it += s_callOp.length();							// skip "operator()"

				m_lang.SkipUntil( &it, itEnd, pred::IsChar( '(' ) );	// skip to operator end (arg-list)
			}
			else if ( pred::IsIdentifier()( *it ) )
				m_lang.SkipIdentifier( &it, itEnd );
			else
				++it;			// skip *, &
		}

	if ( it == *pItSlice )
		return false;			// no advance

	*pItSlice = it;
	return true;
}

const TokenRange& CCppMethodParser::LookupSliceRange( SliceType sliceType ) const
{
	if ( const TokenRange* pFoundRange = FindSliceRange( sliceType ) )
		return *pFoundRange;

	return m_emptyRange;		// code slice not found
}

const TokenRange* CCppMethodParser::FindSliceRange( SliceType sliceType ) const
{
	return utl::FindValuePtr( m_codeSlices, sliceType );
}
