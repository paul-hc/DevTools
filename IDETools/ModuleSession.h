#ifndef ModuleSession_h
#define ModuleSession_h
#pragma once

#include "FormatterOptions.h"
#include "DirPathGroup.h"


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

	void LoadFromRegistry( void );
	void SaveToRegistry( void ) const;

	bool EditOptions( void );

	// lazy registry persistence
	code::CFormatterOptions& GetCodeFormatterOptions( void );

	// accessors
	const std::tstring& GetAdditionalAssocFolders( void ) const { return m_additionalAssocFolders; }
	void SetAdditionalAssocFolders( const std::tstring& additionalAssocFolders ) { m_additionalAssocFolders = additionalAssocFolders; }
private:
	static fs::CPath GetDefaultCodeTemplatePath( void );
public:
	// general options
	std::tstring m_developerName;
	fs::CPath m_codeTemplatePath;
	UINT m_splitMaxColumn;
	UINT m_menuVertSplitCount;
	std::tstring m_singleLineCommentToken;

	bool m_autoCodeGeneration;
	bool m_displayErrorMessages;
	bool m_useCommentDecoration;
	bool m_duplicateLineMoveDown;

	// browse files path options
	fs::CPath m_browseInfoPath;

	// prefixes
	std::tstring m_classPrefix;
	std::tstring m_structPrefix;
	std::tstring m_enumPrefix;

	// Visual Studio options (read-only)
	int m_vsTabSizeCpp;
	bool m_vsKeepTabsCpp;
	bool m_vsUseStandardWindowsMenu;

	// additional paths
	inc::CDirPathGroup m_moreAdditionalIncludePath;		// from vbscript, transient
private:
	std::tstring m_additionalAssocFolders;
private:
	// code formatting options (lazy registry)
	std::auto_ptr< code::CFormatterOptions > m_pCodeFormatterOptions;

	// generated stuff
protected:
	DECLARE_MESSAGE_MAP()
};


#endif // ModuleSession_h
