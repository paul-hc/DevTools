
#include "pch.h"
#include "MethodPrototype.h"
#include "CppCodeParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace code
{
	void CMethodPrototype::Reset( const std::tstring& proto )
	{
		m_proto = proto;

		// set all ranges empty at end
		m_templateDecl.setEmpty( m_proto.length() );
		m_argList = m_inlineModifier = m_returnType = m_qualifiedMethod = m_functionName = m_classQualifier = m_postArgListSuffix =
			m_templateDecl;
	}

	/**
		Finds a method declaration/definition components:
		[m_templateDecl][m_inlineModifier] m_returnType m_qualifiedMethod [m_classQualifier] m_argList [m_postArgListSuffix]

		template< typename PathType, typename ObjectType >
		std::pair<ObjectType*, cache::TStatusFlags> CCacheLoader<PathType, ObjectType>::Acquire( const PathType& pathKey ) const throws(std::exception, std::runtime_error)

		Check-out CMethodPrototypeTests for code cases covered.
	*/
	void CMethodPrototype::SplitMethod( const std::tstring& proto )
	{
		Reset( proto );

		CCppCodeParser parser;

		parser.ParseCode( proto );		// split into code slices

		m_templateDecl = parser.LookupSliceRange( CCppCodeParser::TemplateDecl );
		m_inlineModifier = parser.LookupSliceRange( CCppCodeParser::InlineModifier );
		m_returnType = parser.LookupSliceRange( CCppCodeParser::ReturnType );
		m_qualifiedMethod = parser.LookupSliceRange( CCppCodeParser::QualifiedMethod );
		m_functionName = parser.LookupSliceRange( CCppCodeParser::FunctionName );
		m_classQualifier = parser.LookupSliceRange( CCppCodeParser::ClassQualifier );
		m_argList = parser.LookupSliceRange( CCppCodeParser::ArgList );
		m_postArgListSuffix = parser.LookupSliceRange( CCppCodeParser::PostArgListSuffix );
	}

	std::tstring CMethodPrototype::FormatInfo( void ) const
	{
		static const TCHAR s_quote = _T('\'');
		std::tostringstream oss;

		oss
			<< _T("# CMethodPrototype - Prototype components for:") << std::endl
			<< m_proto << std::endl
			<< _T("templateDecl=") << s_quote << str::ExtractString( m_templateDecl, m_proto ) << s_quote << std::endl
			<< _T("inlineModifier=") << s_quote << str::ExtractString( m_inlineModifier, m_proto ) << s_quote << std::endl
			<< _T("returnType=") << s_quote << str::ExtractString( m_returnType, m_proto ) << s_quote << std::endl
			<< _T("functionName=") << s_quote << str::ExtractString( m_functionName, m_proto ) << s_quote << std::endl
			<< _T("qualifiedMethod=") << s_quote << str::ExtractString( m_qualifiedMethod, m_proto ) << s_quote << std::endl
			<< _T("classQualifier=") << s_quote << str::ExtractString( m_classQualifier, m_proto ) << s_quote << std::endl
			<< _T("argList=") << s_quote << str::ExtractString( m_argList, m_proto ) << s_quote << std::endl
			<< _T("postArgListSuffix=") << s_quote << str::ExtractString( m_postArgListSuffix, m_proto ) << s_quote << std::endl;

		return oss.str();
	}

} // namespace code
