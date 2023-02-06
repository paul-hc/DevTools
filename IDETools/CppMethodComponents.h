// Copyleft 2004 Paul Cocoveanu
//
#ifndef CppMethodComponents_h
#define CppMethodComponents_h
#pragma once

#include "TokenRange.h"
#include "LanguageSearchEngine.h"


namespace code
{
	struct CppMethodComponents
	{
		CppMethodComponents( const TCHAR* pMethodPrototype );

		void splitMethod( const TCHAR validArgListOpenBraces[] = _T("(") );

		std::tstring FormatInfo( void ) const;
		void showMessageBox( void ) const;
	private:
		LanguageSearchEngine m_languageEngine;
		const TCHAR* m_pMethodPrototype;
		int m_methodLength;
	public:
		TokenRange m_templateDecl;
		TokenRange m_inlineModifier;
		TokenRange m_returnType;
		TokenRange m_methodName;
		TokenRange m_typeQualifier;
		TokenRange m_argList;
		TokenRange m_postArgListSuffix; // excluding the terminating line-end
	};
}


#endif // CppMethodComponents_h
