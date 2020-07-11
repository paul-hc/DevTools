#ifndef FileListDialog_h
#define FileListDialog_h
#pragma once

#include "utl/FileState.h"
#include "utl/UI/LayoutDialog.h"
#include "utl/UI/ReportListControl.h"


class CDisplayObject;
class CLogger;

namespace fs { typedef std::map< fs::CFileState, fs::CFileState > TFileStatePairMap; }
typedef std::pair< const fs::CFileState, fs::CFileState > TFileStatePair;


class CFileListDialog : public CLayoutDialog
					  , private ui::ITextEffectCallback
{
public:
	CFileListDialog( CWnd* pParent );
	virtual ~CFileListDialog();
private:
	void InitDisplayItems( void );

	// output
	void SetupFileListView( void );

	// ui::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const;
	virtual void ModifyDiffTextEffectAt( std::vector< ui::CTextEffect >& rMatchEffects, LPARAM rowKey, int subItem, CReportListControl* pCtrl ) const;
private:
	bool m_useDiffsMode;
	bool m_useAlternateRows;
	bool m_useTextEffects;
	bool m_useDoubleBuffer;
	std::vector< CDisplayObject* > m_displayItems;
private:
	// enum { IDD = IDD_FILE_LIST_DIALOG };
	enum Column { SrcFileName, DestFileName, SrcAttributes, DestAttributes, SrcCreationDate, DestCreationDate, Notes };
	enum CustomColors { ColorErrorBk = color::PastelPink };

	CReportListControl m_fileListCtrl;
	CImageList m_imageList;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnDestroy( void );
	afx_msg void OnBnClicked_OpenDialog( void );
	afx_msg void OnToggle_UseDiffsCheck( void );
	afx_msg void OnToggle_UseAlternateRows( void );
	afx_msg void OnToggle_UseTextEffects( void );
	afx_msg void OnToggle_UseDoubleBuffer( void );
	afx_msg void OnToggle_UseExplorerTheme( void );
	afx_msg void OnToggle_UseDefaultDraw( void );
	afx_msg void OnToggle_UseDbgGuides( void );

	DECLARE_MESSAGE_MAP()
};


#include "utl/Subject.h"


class CDisplayObject : public CSubject
{
public:
	CDisplayObject( const TFileStatePair* pStatePair ) : m_pStatePair( safe_ptr( pStatePair ) ), m_displayPath( pStatePair->first.m_fullPath.GetFilename() ) {}

	const fs::CPath& GetKeyPath( void ) const { return GetSrcState().m_fullPath; }
	const fs::CFileState& GetSrcState( void ) const { return m_pStatePair->first; }
	const fs::CFileState& GetDestState( void ) const { return m_pStatePair->second; }

	bool IsModified( void ) const { return m_pStatePair->second.IsValid() && m_pStatePair->second != m_pStatePair->first; }

	// utl::ISubject interface
	virtual const std::tstring& GetCode( void ) const;
	virtual std::tstring GetDisplayCode( void ) const;
private:
	const TFileStatePair* m_pStatePair;
	const std::tstring m_displayPath;
};


#endif // FileListDialog_h
