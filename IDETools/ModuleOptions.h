#pragma once

#include "utl/AppTools.h"


// This automation object gets created in the VC++ macro initialization via the SetupMacroParameters() function, which is called on script startup.
// Macro file locations example:
//	Visual Studio 6		C:\Program Files (x86)\Microsoft Visual Studio\Common\MSDev98\Macros\C++ Macros.dsm
//	Visual Studio 2008	C:\Users\Paul\Documents\Visual Studio 2008\Projects\VSMacros80\CppMacros\CppMacros.vsmacros

class ModuleOptions : public CCmdTarget
	, private app::CLazyInitAppResources
{
	DECLARE_DYNCREATE( ModuleOptions )
protected:
	ModuleOptions();		   // protected constructor used by dynamic creation
	virtual ~ModuleOptions();

	// generated stuff
public:
	virtual void OnFinalRelease();
protected:
	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(ModuleOptions)

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

	// generated OLE dispatch map functions
	enum
	{
		// properties:
		// NOTE - ClassWizard will maintain property information here. Use extreme caution when editing this section.
		dispid_developerName = 1,
		dispid_codeTemplateFile = 2,
		dispid_splitMaxColumn = 3,
		dispid_menuVertSplitCount = 4,
		dispid_singleLineCommentToken = 5,
		dispid_classPrefix = 6,
		dispid_structPrefix = 7,
		dispid_enumPrefix = 8,
		dispid_autoCodeGeneration = 9,
		dispid_displayErrorMessages = 10,
		dispid_useCommentDecoration = 11,
		dispid_duplicateLineMoveDown = 12,
		dispid_browseInfoPath = 13,
		dispid_additionalIncludePath = 14,
		dispid_additionalAssocFolders = 15,
		dispid_linesBetweenFunctionImpls = 16,
		dispid_returnTypeOnSeparateLine = 17,

		// methods:
		dispidGetVStudioCommonDirPath = 18,
		dispidGetVStudioMacrosDirPath = 19,
		dispidGetVStudioVC98DirPath = 20,
		dispidEditOptions = 21,
		dispidLoadOptions = 22,
		dispidSaveOptions = 23
	};

	afx_msg BSTR GetDeveloperName();
	afx_msg void SetDeveloperName(LPCTSTR lpszNewValue);
	afx_msg BSTR GetCodeTemplateFile();
	afx_msg void SetCodeTemplateFile(LPCTSTR lpszNewValue);
	afx_msg long GetSplitMaxColumn();
	afx_msg void SetSplitMaxColumn(long nNewValue);
	afx_msg long GetMenuVertSplitCount();
	afx_msg void SetMenuVertSplitCount(long nNewValue);
	afx_msg BSTR GetSingleLineCommentToken();
	afx_msg void SetSingleLineCommentToken(LPCTSTR lpszNewValue);
	afx_msg BSTR GetClassPrefix();
	afx_msg void SetClassPrefix(LPCTSTR lpszNewValue);
	afx_msg BSTR GetStructPrefix();
	afx_msg void SetStructPrefix(LPCTSTR lpszNewValue);
	afx_msg BSTR GetEnumPrefix();
	afx_msg void SetEnumPrefix(LPCTSTR lpszNewValue);
	afx_msg BOOL GetAutoCodeGeneration();
	afx_msg void SetAutoCodeGeneration(BOOL bNewValue);
	afx_msg BOOL GetDisplayErrorMessages();
	afx_msg void SetDisplayErrorMessages(BOOL bNewValue);
	afx_msg BOOL GetUseCommentDecoration();
	afx_msg void SetUseCommentDecoration(BOOL bNewValue);
	afx_msg BOOL GetDuplicateLineMoveDown();
	afx_msg void SetDuplicateLineMoveDown(BOOL bNewValue);
	afx_msg BSTR GetBrowseInfoPath();
	afx_msg void SetBrowseInfoPath(LPCTSTR lpszNewValue);
	afx_msg BSTR GetAdditionalIncludePath();
	afx_msg void SetAdditionalIncludePath(LPCTSTR lpszNewValue);
	afx_msg BSTR GetAdditionalAssocFolders();
	afx_msg void SetAdditionalAssocFolders(LPCTSTR lpszNewValue);
	afx_msg long GetLinesBetweenFunctionImpls();
	afx_msg BOOL GetReturnTypeOnSeparateLine();
	afx_msg BSTR GetVStudioCommonDirPath(BOOL addTrailingSlash);
	afx_msg BSTR GetVStudioMacrosDirPath(BOOL addTrailingSlash);
	afx_msg BSTR GetVStudioVC98DirPath(BOOL addTrailingSlash);
	afx_msg BOOL EditOptions();
	afx_msg void LoadOptions();
	afx_msg void SaveOptions();
};
