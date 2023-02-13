#pragma once

#include "AutomationBase.h"
#include "FileBrowser.h"


class MenuFilePicker : public CCmdTarget
	, private CAutomationBase
{
	DECLARE_DYNCREATE( MenuFilePicker )
protected:
	MenuFilePicker( void );
	virtual ~MenuFilePicker();
public:
	CFileBrowser m_browser;

	// generated stuff
public:
	virtual void OnFinalRelease( void );
protected:
	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE( MenuFilePicker )

	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

	// generated OLE dispatch map functions
	long m_trackPosX;
	afx_msg void OnTrackPosXChanged();
	long m_trackPosY;
	afx_msg void OnTrackPosYChanged();
	afx_msg long GetOptionFlags();
	afx_msg void SetOptionFlags( long nNewValue );
	afx_msg long GetFolderLayout();
	afx_msg void SetFolderLayout( long nNewValue );
	afx_msg BSTR GetCurrentFilePath();
	afx_msg void SetCurrentFilePath( LPCTSTR pCurrFilePath );
	afx_msg BSTR SetProfileSection( LPCTSTR pSubSection );
	afx_msg BOOL AddFolder( LPCTSTR pFolderPathFilter, LPCTSTR pFolderAlias );
	afx_msg BOOL AddFolderArray( LPCTSTR pFolderItemFlatArray );
	afx_msg BOOL AddRootFile( LPCTSTR pFilePath, LPCTSTR pLabel );
	afx_msg void AddSortOrder( long pathField, BOOL exclusive );
	afx_msg void ClearSortOrder();
	afx_msg void StoreTrackPos();
	afx_msg BOOL ChooseFile();

	enum
	{
		// properties:
		dispidTrackPosX = 1,
		dispidTrackPosY = 2,
		dispidOptionFlags = 3,
		dispidFolderLayout = 4,
		dispidCurrentFilePath = 5,

		// methods:
		dispidSetProfileSection = 6,
		dispidAddFolder = 7,
		dispidAddFolderArray = 8,
		dispidAddRootFile = 9,
		dispidAddSortOrder = 10,
		dispidClearSortOrder = 11,
		dispidStoreTrackPos = 12,
		dispidChooseFile = 13
	};
};
