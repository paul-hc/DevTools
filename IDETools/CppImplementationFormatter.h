// Copyleft 2004 Paul Cocoveanu
//
#ifndef CppImplementationFormatter_h
#define CppImplementationFormatter_h
#pragma once

#include "Formatter.h"
#include "utl/RuntimeException.h"


namespace code
{
	struct CTypeDescriptor;


	class CppImplementationFormatter : public CFormatter
	{
	public:
		CppImplementationFormatter( const CFormatterOptions& options );
		~CppImplementationFormatter();

		// operations
		std::tstring ExtractTypeDescriptor( const std::tstring& functionImplLine, const fs::CPath& docPath );
		std::tstring ImplementMethodBlock( const TCHAR* pMethodPrototypes, const TCHAR* pTypeDescriptor, bool isInline ) throws_( CRuntimeException );

		CString autoMakeCode( const TCHAR* pCodeText );
		CString tokenizeText( const TCHAR* pCodeText );

		static bool isCppTypeQualifier( std::tstring typeQualifier );
	protected:
		bool LoadCodeTemplates( void );
		std::tstring MakeCommentDecoration( const std::tstring& decorationCore ) const;

		void ResolveDefaultParameters( std::tstring* pProto ) const;

		std::tstring ImplementMethod( const std::tstring& methodProto, const std::tstring& templateDecl, const std::tstring& typeQualifier, bool isInline );
		std::tstring InputDocTypeDescriptor( const fs::CPath& docPath ) const;

		CString makeIteratorLoop( const TCHAR* pCodeText, bool isConstIterator ) throws_( CRuntimeException );
		CString makeIndexLoop( const TCHAR* pCodeText ) throws_( CRuntimeException );
	protected:
		// code templates
		std::tstring m_voidFunctionBody;
		std::tstring m_returnFunctionBody;
		std::tstring m_commentDecorationTemplate;
	};


	struct CTypeDescriptor
	{
		CTypeDescriptor( const CFormatter* pFmt ) : m_pFmt( pFmt ) { ASSERT_PTR( pFmt ); }

		void Parse( const TCHAR* pTypeDescriptor ) throws_( CRuntimeException );
	private:
		void Split( const TCHAR* pTypeDescriptor ) throws_( CRuntimeException );
		CString buildTemplateInstanceTypeList( const TokenRange& templateDecl, const TCHAR* pMethodPrototype ) const;
	private:
		const CFormatter* m_pFmt;
	public:
		std::tstring m_indentPrefix;
		std::tstring m_templateDecl;
		std::tstring m_typeQualifier;
	};
}


#endif // CppImplementationFormatter_h
