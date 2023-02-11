// Copyleft 2023 Paul Cocoveanu
//
#ifndef MethodPrototype_h
#define MethodPrototype_h
#pragma once

#include "TokenRange.h"
	#include "LanguageSearchEngine.h"


namespace code
{
	struct CMethodPrototype
	{
		CMethodPrototype( void ) : m_languageEngine( DocLang_Cpp ) {}

		void SplitMethod( const std::tstring& methodPrototype );

		std::tstring FormatInfo( void ) const;
		void ShowMessageBox( void ) const;
	private:
		void Reset( const std::tstring& methodPrototype );
	private:
		std::tstring m_methodPrototype;
		LanguageSearchEngine m_languageEngine;
	public:
		TokenRange m_argList;				// e.g. "( const PathType& pathKey )"
		TokenRange m_postArgListSuffix;		// e.g. " const" - excluding the terminating line-end
		TokenRange m_functionName;			// e.g. "Acquire" or "operator!="
		TokenRange m_methodQualifiedName;	// e.g. "CCacheLoader<PathType, ObjectType>::Acquire"
		TokenRange m_typeQualifier;			// e.g. "CCacheLoader<PathType, ObjectType>::"
		TokenRange m_returnType;			// e.g. "std::pair<ObjectType*, cache::TStatusFlags>"

		TokenRange m_inlineModifier;		// e.g. "inline"
		TokenRange m_templateDecl;			// e.g. "template< typename PathType, typename ObjectType >"
	};
}


#endif // MethodPrototype_h
