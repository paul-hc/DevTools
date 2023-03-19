#pragma once

#include "utl/AppTools.h"
#include <vector>
#include <map>


// Automation object for text file IO, string replacements, etc

class TextContent : public CCmdTarget
	, private app::CLazyInitAppResources
{
	DECLARE_DYNCREATE( TextContent )
public:
	TextContent( void );		// protected constructor used by dynamic creation
	virtual ~TextContent();
private:
	std::tstring m_textContent;
	std::map<std::tstring, std::tstring> m_fieldReplacements;
private:
	std::vector<TCHAR> m_tokenizedBuffer;
	std::tstring m_tokenizedSeps;
	std::tstring m_currToken;

	// generated stuff
public:
	virtual void OnFinalRelease( void );
protected:
	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE( TextContent )

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
public:
	// generated OLE dispatch map functions
	enum
	{
		// properties:
		// NOTE - ClassWizard will maintain property information here. Use extreme caution when editing this section.
		dispidShowErrors = 1,
		dispidText = 2,
		dispidTextLen = 3,
		// methods:
		dispidLoadFile = 4,
		dispidLoadFileSection = 5,
		dispidFindText = 6,
		dispidReplaceText = 7,
		dispidAddEmbeddedContent = 8,
		dispidTokenize = 9,
		dispidGetNextToken = 10,
		dispidMultiLinesToSingleParagraph = 11,
		dispidAddFieldReplacement = 12,
		dispidClearFieldReplacements = 13,
		dispidFormatTimestamp = 14
	};

	BOOL m_showErrors;
	afx_msg void OnShowErrorsChanged();
	afx_msg BSTR GetText();
	afx_msg void SetText( LPCTSTR lpszNewValue );
	afx_msg long GetTextLen();
	afx_msg BOOL LoadFile( LPCTSTR textFilePath );
	afx_msg BOOL LoadFileSection( LPCTSTR compoundFilePath, LPCTSTR sectionName );
	afx_msg long FindText( LPCTSTR pattern, long startPos, BOOL caseSensitive );
	afx_msg long ReplaceText( LPCTSTR pattern, LPCTSTR replacement, BOOL caseSensitive );
	afx_msg BOOL AddEmbeddedContent( LPCTSTR matchCoreID, LPCTSTR embeddedContent, BOOL caseSensitive );
	afx_msg BSTR Tokenize( LPCTSTR separatorCharSet );
	afx_msg BSTR GetNextToken();
	afx_msg long MultiLinesToSingleParagraph( LPCTSTR multiLinesText, BOOL doTrimTrailingSpaces );
	afx_msg void AddFieldReplacement( LPCTSTR fieldText, LPCTSTR replaceWith );
	afx_msg void ClearFieldReplacements();
	afx_msg BSTR FormatTimestamp( DATE timestamp, LPCTSTR strftimeFormat );
};
