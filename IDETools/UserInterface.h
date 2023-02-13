#pragma once

#include "utl/AppTools.h"


// UserInterface command target

class UserInterface : public CCmdTarget
	, private app::CLazyInitAppResources
{
	DECLARE_DYNCREATE(UserInterface)
protected:
	UserInterface();
	virtual ~UserInterface();

	// generated stuff
public:
	virtual void OnFinalRelease( void );
protected:
	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(UserInterface)

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

	// generated OLE dispatch map functions
	enum 
	{
		dispidIDEToolsRegistryKey = 1,
		dispidRunUnitTests = 2,
		dispidInputBox = 3,
		dispidGetClipboardText = 4,
		dispidSetClipboardText = 5,
		dispidIsClipFormatAvailable = 6,
		dispidIsClipFormatNameAvailable = 7,
		dispidIsKeyPath = 8,
		dispidCreateKeyPath = 9,
		dispidRegReadString = 10,
		dispidRegReadNumber = 11,
		dispidRegWriteString = 12,
		dispidRegWriteNumber = 13,
		dispidEnsureStringValue = 14,
		dispidEnsureNumberValue = 15,
		dispidGetEnvironmentVariable = 16,
		dispidSetEnvironmentVariable = 17,
		dispidExpandEnvironmentVariables = 18,
		dispidLocateFile = 19
	};

	BSTR GetIDEToolsRegistryKey();
	void SetIDEToolsRegistryKey(LPCTSTR lpszNewValue);
	void RunUnitTests(void);
	BSTR InputBox(LPCTSTR title, LPCTSTR prompt, LPCTSTR initialValue);
	BSTR GetClipboardText();
	void SetClipboardText(LPCTSTR text);
	BOOL IsClipFormatAvailable(long clipFormat);
	BOOL IsClipFormatNameAvailable(LPCTSTR formatName);
	BOOL IsKeyPath(LPCTSTR pKeyFullPath);
	BOOL CreateKeyPath(LPCTSTR pKeyFullPath);
	BSTR RegReadString(LPCTSTR pKeyFullPath, LPCTSTR pValueName, LPCTSTR pDefaultString);
	long RegReadNumber(LPCTSTR pKeyFullPath, LPCTSTR pValueName, long defaultNumber);
	BOOL RegWriteString(LPCTSTR pKeyFullPath, LPCTSTR pValueName, LPCTSTR pStrValue);
	BOOL RegWriteNumber(LPCTSTR pKeyFullPath, LPCTSTR pValueName, long numValue);
	BOOL EnsureStringValue(LPCTSTR pKeyFullPath, LPCTSTR pValueName, LPCTSTR pDefaultString);
	BOOL EnsureNumberValue(LPCTSTR pKeyFullPath, LPCTSTR pValueName, long defaultNumber);
	BSTR GetEnvironmentVariable(LPCTSTR varName);
	BOOL SetEnvironmentVariable(LPCTSTR varName, LPCTSTR varValue);
	BSTR ExpandEnvironmentVariables(LPCTSTR sourceString);
	BSTR LocateFile(LPCTSTR localDirPath);
};
