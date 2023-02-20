
#include "pch.h"
#include "CppCodeParser.h"
#include "utl/Algorithms.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const std::tstring CCppCodeParser::s_template = _T("template");
const std::tstring CCppCodeParser::s_inline = _T("inline");
const std::tstring CCppCodeParser::s_operator = _T("operator");
const std::tstring CCppCodeParser::s_scopeOp = _T("::");
const std::tstring CCppCodeParser::s_callOp = _T("()");

CCppCodeParser::CCppCodeParser( void )
	: m_lang( code::GetCppLanguage<TCHAR>() )
{
}

bool CCppCodeParser::ParseCode( const std::tstring& codeText )
{
	m_codeSlices.clear();
	m_emptyRange.SetEmptyRange( codeText.length() );

	TConstIterator itCode = codeText.begin(), itEnd = codeText.end(), it = itCode;
	const code::CLanguage<TCHAR>::TSeparatorsPair& sepsPair = m_lang.GetParser().GetSeparators();
	typedef Range<TConstIterator> TIteratorRange;

	for ( code::CLanguage<TCHAR>::TSepMatchPos sepMatchPos = std::tstring::npos; it != itEnd; )
	{
		m_lang.SkipWhitespace( &it, itEnd );

		TConstIterator itSlice = it;		// beginning of a potential slice

		if ( sepsPair.MatchesAnyOpenSepAt( &sepMatchPos, it, itEnd ) )	// matches a quoted string, comment, etc?
			sepsPair.SkipMatchingSpec( &it, itEnd, sepMatchPos );		// skip quoted strings, comments, etc
		else if ( !HasSlice( TemplateDecl ) && str::EqualsSeq( it, itEnd, s_template ) )
		{
			it += s_template.length();
			m_lang.SkipWhitespace( &it, itEnd );

			if ( it == itEnd || *it != '<' )
				return false;					// syntax error: template argument list missing

			m_lang.SkipPastMatchingBrace( &it, itEnd );
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

void CCppCodeParser::ParseQualifiedMethod( const std::tstring& codeText )
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

bool CCppCodeParser::FindSliceEnd( TConstIterator* pItSlice, const TConstIterator& itEnd ) const
{
	ASSERT_PTR( pItSlice );

	TConstIterator it = *pItSlice;

	if ( '(' == *it )			// beginning of an args-list?
		m_lang.SkipPastMatchingBrace( &it, itEnd );		// argument list is a slice in itself
	else
		while ( it != itEnd && !pred::IsSpace()( *it ) && *it != '(' )
		{
			if ( '<' == *it )
				m_lang.SkipPastMatchingBrace( &it, itEnd );
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

const TokenRange& CCppCodeParser::LookupSliceRange( SliceType sliceType ) const
{
	if ( const TokenRange* pFoundRange = FindSliceRange( sliceType ) )
		return *pFoundRange;

	return m_emptyRange;		// code slice not found
}

const TokenRange* CCppCodeParser::FindSliceRange( SliceType sliceType ) const
{
	return utl::FindValuePtr( m_codeSlices, sliceType );
}
