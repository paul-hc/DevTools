#include "stdafx.h"
#include "DocLanguage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


str::CaseType getLanguageCase( DocLanguage docLanguage )
{
	switch ( docLanguage )
	{
		default:
		case DocLang_Cpp:
		case DocLang_IDL:
		case DocLang_HtmlXml:
			return str::Case;
		case DocLang_Basic:
		case DocLang_SQL:
		case DocLang_None:
			return str::IgnoreCase;
	}
}
