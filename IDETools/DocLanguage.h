#ifndef DocLanguage_h
#define DocLanguage_h
#pragma once


enum DocLanguage
{
	DocLang_None,
	DocLang_Cpp,
	DocLang_Basic,
	DocLang_SQL,
	DocLang_HtmlXml,
	DocLang_IDL
};


str::CaseType getLanguageCase( DocLanguage docLanguage );


#endif // DocLanguage_h
