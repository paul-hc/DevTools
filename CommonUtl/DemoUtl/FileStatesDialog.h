#ifndef FileStatesDialog_h
#define FileStatesDialog_h
#pragma once

#include "utl/LayoutDialog.h"
#include "utl/FileState.h"
#include "utl/ReportListControl.h"


class CLogger;
typedef std::pair< const fs::CFileState, fs::CFileState > TFileStatePair;


class CFileStatesDialog : public CLayoutDialog
						, private CReportListControl::ITextEffectCallback
{
public:
	CFileStatesDialog( CWnd* pParent );
	virtual ~CFileStatesDialog();
private:
	void InitDisplayItems( void );

	// output
	void SetupFileListView( void );

	// CReportListControl::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem ) const;

	static const fs::TFileStatePairMap& GetStatePairs( bool useDiffsMode );
	static const std::tstring& GetNotesAt( size_t pos );


	class CDisplayObject : public utl::ISubject
	{
	public:
		CDisplayObject( const TFileStatePair* pStatePair ) : m_pStatePair( safe_ptr( pStatePair ) ), m_displayPath( pStatePair->first.m_fullPath.GetNameExt() ) {}

		const fs::CPath& GetKeyPath( void ) const { return GetSrcState().m_fullPath; }
		const fs::CFileState& GetSrcState( void ) const { return m_pStatePair->first; }
		const fs::CFileState& GetDestState( void ) const { return m_pStatePair->second; }

		bool IsModified( void ) const { return m_pStatePair->second.IsValid() && m_pStatePair->second != m_pStatePair->first; }

		// utl::ISubject interface
		virtual std::tstring GetCode( void ) const;
		virtual std::tstring GetDisplayCode( void ) const;
	private:
		const TFileStatePair* m_pStatePair;
		const std::tstring m_displayPath;
	};
private:
	bool m_useDiffsMode;
	bool m_useAlternateRows;
	bool m_useTextEffects;
	std::vector< CDisplayObject* > m_displayItems;
private:
	// enum { IDD = IDD_FILE_STATES_DIALOG };
	enum Column { SrcFileName, DestFileName, SrcAttributes, DestAttributes, SrcCreationDate, DestCreationDate, Notes };
	enum CustomColors { ColorErrorBk = color::PastelPink };

	CReportListControl m_fileListCtrl;

	// generated stuff
public:
	virtual BOOL OnCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnToggle_UseDiffsCheck( void );
	afx_msg void OnToggle_UseAlternateRows( void );
	afx_msg void OnToggle_UseTextEffects( void );

	DECLARE_MESSAGE_MAP()
};


#endif // FileStatesDialog_h
