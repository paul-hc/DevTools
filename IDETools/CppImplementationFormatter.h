// Copyleft 2004 Paul Cocoveanu
//
#ifndef CppImplementationFormatter_h
#define CppImplementationFormatter_h
#pragma once

#include "Formatter.h"
#include "utl/RuntimeException.h"


namespace code
{
	class CppImplementationFormatter : public CFormatter
	{
	public:
		CppImplementationFormatter( const CFormatterOptions& _options );
		~CppImplementationFormatter();

		// operations
		CString extractTypeDescriptor( const TCHAR* pFunctionImplLine, const TCHAR* pDocFilename );
		CString implementMethodBlock( const TCHAR* pMethodPrototypes, const TCHAR* pTypeDescriptor, bool isInline ) throws_( mfc::CRuntimeException );

		CString autoMakeCode( const TCHAR* pCodeText );
		CString tokenizeText( const TCHAR* pCodeText );

		static bool isCppTypeQualifier( std::tstring typeQualifier );
	protected:
		bool loadCodeTemplates( void );
		CString makeCommentDecoration( const TCHAR* pDecorationCore ) const;

		CString buildTemplateInstanceTypeList( const TokenRange& templateDecl, const TCHAR* pMethodPrototype ) const;

		void prototypeResolveDefaultParameters( CString& rTargetString ) const;

		CString implementMethod( const TCHAR* pMethodPrototype, const TCHAR* pTemplateDecl,
								 const TCHAR* pTypeQualifier, bool isInline );
		CString inputDocTypeDescriptor( const TCHAR* pDocFilename ) const;
		void splitTypeDescriptor( CString& rTemplateDecl, CString& rTypeQualifier, const TCHAR* pTypeDescriptor ) const throws_( mfc::CRuntimeException );

		CString makeIteratorLoop( const TCHAR* pCodeText, bool isConstIterator ) throws_( mfc::CRuntimeException );
		CString makeIndexLoop( const TCHAR* pCodeText ) throws_( mfc::CRuntimeException );
	protected:
		// code templates
		CString m_voidFunctionBody;
		CString m_returnFunctionBody;
		CString m_commentDecorationTemplate;
	};
}


#endif // CppImplementationFormatter_h
