// Copyleft 2004 Paul Cocoveanu
//
#ifndef CppImplementationFormatter_h
#define CppImplementationFormatter_h
#pragma once

#include "Formatter.h"
#include "utl/RuntimeException.h"


namespace code
{
	struct CMethodPrototype;


	class CppImplementationFormatter : public CFormatter
	{
	public:
		CppImplementationFormatter( const CFormatterOptions& _options );
		~CppImplementationFormatter();

		// operations
		CString extractTypeDescriptor( const TCHAR* functionImplLine, const TCHAR* pDocFilename );
		CString implementMethodBlock( const TCHAR* methodPrototypes, const TCHAR* typeDescriptor, bool isInline ) throws_( mfc::CRuntimeException );

		CString autoMakeCode( const TCHAR* codeText );
		CString tokenizeText( const TCHAR* codeText );

		static bool isCppTypeQualifier( std::tstring typeQualifier );
	protected:
		bool loadCodeTemplates( void );
		CString makeCommentDecoration( const TCHAR* decorationCore ) const;

		CString buildTemplateInstanceTypeList( const TokenRange& templateDecl, const TCHAR* methodPrototype ) const;

		void prototypeResolveDefaultParameters( CString& targetString ) const;

		CString implementMethod( const TCHAR* methodPrototype, const TCHAR* templateDecl,
								 const TCHAR* typeQualifier, bool isInline );
		CString inputDocTypeDescriptor( const TCHAR* pDocFilename ) const;
		void splitTypeDescriptor( CString& templateDecl, CString& typeQualifier, const TCHAR* typeDescriptor ) const throws_( mfc::CRuntimeException );

		CString makeIteratorLoop( const TCHAR* codeText, bool isConstIterator ) throws_( mfc::CRuntimeException );
		CString makeIndexLoop( const TCHAR* codeText ) throws_( mfc::CRuntimeException );
	protected:
		// code templates
		CString m_voidFunctionBody;
		CString m_returnFunctionBody;
		CString m_commentDecorationTemplate;
	};
}


#endif // CppImplementationFormatter_h
