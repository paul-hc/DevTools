#if !defined(AFX_TEXTCONTENT_H__E37FE178_CBB7_11D4_B57C_00D0B74ECB52__INCLUDED_)
#define AFX_TEXTCONTENT_H__E37FE178_CBB7_11D4_B57C_00D0B74ECB52__INCLUDED_
#pragma once


#include <vector>
#include <map>


// Automation object for text file IO, replacements, etc
class TextContent : public CCmdTarget
{
	DECLARE_DYNCREATE( TextContent )
public:
	TextContent( void );		// protected constructor used by dynamic creation
	virtual ~TextContent();
private:
	CString m_TextContent;
	std::map< CString, CString > m_fieldReplacements;
private:
	CString tokenizedBuffer, tokenizedSeps;
	CString currToken;
public:
	// generated overrides
	//{{AFX_VIRTUAL(TextContent)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL
protected:
	// generated message map
	//{{AFX_MSG(TextContent)
	//}}AFX_MSG
public:
	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE( TextContent )

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(TextContent)
	BOOL m_showErrors;
	afx_msg void OnShowErrorsChanged();
	afx_msg BSTR GetText();
	afx_msg void SetText( LPCTSTR lpszNewValue );
	afx_msg long GetTextLen();
	afx_msg BOOL LoadFile( LPCTSTR textFilePath );
	afx_msg BOOL LoadFileSection( LPCTSTR compoundFilePath, LPCTSTR sectionName );
	afx_msg long LoadCompoundFileSections( LPCTSTR compoundFilePath, LPCTSTR sectionFilter, BOOL caseSensitive );
	afx_msg long FindText( LPCTSTR match, long startPos, BOOL caseSensitive );
	afx_msg long ReplaceText( LPCTSTR match, LPCTSTR replacement, BOOL caseSensitive );
	afx_msg BOOL AddEmbeddedContent( LPCTSTR matchCoreID, LPCTSTR embeddedContent, BOOL caseSensitive );
	afx_msg BSTR Tokenize( LPCTSTR separatorCharSet );
	afx_msg BSTR GetNextToken();
	afx_msg long MultiLinesToSingleParagraph( LPCTSTR multiLinesText, BOOL doTrimTrailingSpaces );
	afx_msg void AddFieldReplacement( LPCTSTR fieldText, LPCTSTR replaceWith );
	afx_msg void ClearFieldReplacements();
	afx_msg BSTR FormatTimestamp( DATE timestamp, LPCTSTR strftimeFormat );
	//}}AFX_DISPATCH

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEXTCONTENT_H__E37FE178_CBB7_11D4_B57C_00D0B74ECB52__INCLUDED_)
