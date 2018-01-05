#ifndef ModuleSession_h
#define ModuleSession_h
#pragma once

#include "FormatterOptions.h"


enum DebugBreakType { NoBreak, Break, PromptBreak };


namespace reg
{
	extern const TCHAR section_settings[];
}


// persistent module settings

class CModuleSession : public CCmdTarget
{
	DECLARE_DYNCREATE( CModuleSession )
public:
	CModuleSession( void );
	virtual ~CModuleSession();

	static DebugBreakType GetDebugBreakType( void );
	static void StoreDebugBreakType( DebugBreakType debugBreakType );
	static bool IsDebugBreakEnabled( void );

	void LoadFromRegistry( void );
	void SaveToRegistry( void ) const;

	bool EditOptions( void );

	// lazy registry persistence
	code::CFormatterOptions& GetCodeFormatterOptions( void );

	// helpers
	static std::tstring GetVStudioCommonDirPath( bool trailSlash = true );
	static std::tstring GetVStudioMacrosDirPath( bool trailSlash = true );
	static std::tstring GetVStudioVC98DirPath( bool trailSlash = true );

	// accessors
	std::tstring GetExpandedAdditionalIncludePath( void ) const;
	const std::tstring& GetAdditionalIncludePath( void ) const { return m_additionalIncludePath; }
	void SetAdditionalIncludePath( const std::tstring& additionalIncludePath ) { m_additionalIncludePath = additionalIncludePath; }

	const std::tstring& GetAdditionalAssocFolders( void ) const { return m_additionalAssocFolders; }
	void SetAdditionalAssocFolders( const std::tstring& additionalAssocFolders ) { m_additionalAssocFolders = additionalAssocFolders; }
public:
	// general options
	std::tstring m_developerName;
	std::tstring m_codeTemplateFile;
	UINT m_splitMaxColumn;
	int m_menuVertSplitCount;
	std::tstring m_singleLineCommentToken;

	bool m_autoCodeGeneration;
	bool m_displayErrorMessages;
	bool m_useCommentDecoration;
	bool m_duplicateLineMoveDown;

	// browse files path options
	std::tstring m_browseInfoPath;

	// prefixes
	std::tstring m_classPrefix;
	std::tstring m_structPrefix;
	std::tstring m_enumPrefix;

	// Visual Studio options (read-only)
	int m_vsTabSizeCpp;
	bool m_vsKeepTabsCpp;
	bool m_vsUseStandardWindowsMenu;
private:
	// additional paths
	std::tstring m_additionalIncludePath;
	std::tstring m_additionalAssocFolders;
private:
	// code formatting options (lazy registry)
	std::auto_ptr< code::CFormatterOptions > m_codeFormatterOptionsPtr;
public:
	// generated overrides
protected:
	// generated message map functions

	DECLARE_MESSAGE_MAP()
};


#endif // ModuleSession_h
