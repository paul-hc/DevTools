// Copyleft 2004 Paul Cocoveanu
//
#ifndef CppImplFormatter_h
#define CppImplFormatter_h
#pragma once

#include "Formatter.h"
#include "utl/RuntimeException.h"


class CCodeSnippetsParser;
class CMethodPrototypeTests;


namespace code
{
	struct CTypeDescriptor;


	class CCppImplFormatter : public CFormatter
	{
		friend class CMethodPrototypeTests;
	public:
		CCppImplFormatter( const CFormatterOptions& options );
		~CCppImplFormatter();

		// operations
		std::tstring ExtractTypeDescriptor( const std::tstring& functionImplLine, const fs::CPath& docPath );
		std::tstring ImplementMethodBlock( const TCHAR* pMethodPrototypes, const TCHAR* pTypeDescriptor, bool isInline ) throws_( CRuntimeException );

		std::tstring AutoMakeCode( const TCHAR* pCodeText );
		std::tstring TokenizeText( const TCHAR* pCodeText );

		static bool IsCppTypeQualifier( std::tstring typeQualifier );
	private:
		//std::auto_ptr<CCodeSnippetsParser> m_pCodeSnippets;		// self-encapsulates, lazy loaded
		CCodeSnippetsParser* EnsureSnippetsLoaded( void );
		bool StoreCodeSnippets( void );
		std::tstring MakeCommentDecoration( const std::tstring& decorationCore ) const;

		std::tstring ImplementMethod( const std::tstring& methodProto, const CTypeDescriptor& tdInfo );
		std::tstring InputDocTypeDescriptor( const fs::CPath& docPath ) const;

		std::tstring MakeIteratorLoop( const TCHAR* pCodeText, bool isConstIterator );
		std::tstring MakeIndexLoop( const TCHAR* pCodeText );
	private:
		std::auto_ptr<CCodeSnippetsParser> m_pCodeSnippets;		// self-encapsulates, lazy loaded

		// code snippets
		std::tstring m_voidFunctionBody;
		std::tstring m_returnFunctionBody;
		std::tstring m_commentDecorationTemplate;
	};


	struct CTypeDescriptor
	{
		CTypeDescriptor( const CFormatter* pFmt, bool isInline );

		void Parse( const TCHAR* pTypeDescriptor ) throws_( CRuntimeException );

		void IndentCode( std::tstring* pCodeText ) const;
	private:
		void Split( const TCHAR* pTypeDescriptor ) throws_( CRuntimeException );
	private:
		const CFormatter* m_pFmt;
	public:
		std::tstring m_templateDecl;
		std::tstring m_typeQualifier;
		std::tstring m_indentPrefix;
		std::tstring m_inlinePrefix;
	};
}


#endif // CppImplFormatter_h
