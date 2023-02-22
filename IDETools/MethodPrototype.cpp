
#include "pch.h"
#include "MethodPrototype.h"
#include "CppParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace code
{
	void CMethodPrototype::Reset( const std::tstring& proto )
	{
		m_proto = proto;

		// set all ranges empty at end
		m_indentPrefix.SetEmptyRange( static_cast<int>( m_proto.length() ) );

		m_argList = m_inlineModifier = m_returnType = m_qualifiedMethod = m_functionName = m_classQualifier = m_postArgListSuffix = m_templateDecl =
			m_indentPrefix;
	}

	/**
		Finds a method declaration/definition components:
		[m_templateDecl][m_inlineModifier] m_returnType m_qualifiedMethod [m_classQualifier] m_argList [m_postArgListSuffix]

		template< typename PathType, typename ObjectType >
		std::pair<ObjectType*, cache::TStatusFlags> CCacheLoader<PathType, ObjectType>::Acquire( const PathType& pathKey ) const throws(std::exception, std::runtime_error)

		Check-out CMethodPrototypeTests for code cases covered.
	*/
	void CMethodPrototype::ParseCode( const std::tstring& proto )
	{
		Reset( proto );

		CCppMethodParser parser;

		parser.ParseCode( proto );		// split into code slices

		m_indentPrefix = parser.LookupSliceRange( CCppMethodParser::IndentPrefix );
		m_templateDecl = parser.LookupSliceRange( CCppMethodParser::TemplateDecl );
		m_inlineModifier = parser.LookupSliceRange( CCppMethodParser::InlineModifier );
		m_returnType = parser.LookupSliceRange( CCppMethodParser::ReturnType );
		m_qualifiedMethod = parser.LookupSliceRange( CCppMethodParser::QualifiedMethod );
		m_functionName = parser.LookupSliceRange( CCppMethodParser::FunctionName );
		m_classQualifier = parser.LookupSliceRange( CCppMethodParser::ClassQualifier );
		m_argList = parser.LookupSliceRange( CCppMethodParser::ArgList );
		m_postArgListSuffix = parser.LookupSliceRange( CCppMethodParser::PostArgListSuffix );
	}

	std::tstring CMethodPrototype::FormatInfo( void ) const
	{
		static const TCHAR s_quote = _T('\'');
		std::tostringstream oss;

		oss
			<< _T("# CMethodPrototype - Prototype components for:") << std::endl
			<< m_proto << std::endl
			<< _T("indentPrefix=") << s_quote << m_indentPrefix.MakeToken( m_proto ) << s_quote << std::endl
			<< _T("templateDecl=") << s_quote << m_templateDecl.MakeToken( m_proto ) << s_quote << std::endl
			<< _T("inlineModifier=") << s_quote << m_inlineModifier.MakeToken( m_proto ) << s_quote << std::endl
			<< _T("returnType=") << s_quote << m_returnType.MakeToken( m_proto ) << s_quote << std::endl
			<< _T("functionName=") << s_quote << m_functionName.MakeToken( m_proto ) << s_quote << std::endl
			<< _T("qualifiedMethod=") << s_quote << m_qualifiedMethod.MakeToken( m_proto ) << s_quote << std::endl
			<< _T("classQualifier=") << s_quote << m_classQualifier.MakeToken( m_proto ) << s_quote << std::endl
			<< _T("argList=") << s_quote << m_argList.MakeToken( m_proto ) << s_quote << std::endl
			<< _T("postArgListSuffix=") << s_quote << m_postArgListSuffix.MakeToken( m_proto ) << s_quote << std::endl;

		return oss.str();
	}

} // namespace code
