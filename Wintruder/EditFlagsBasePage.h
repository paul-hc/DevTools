#ifndef EditFlagsBasePage_h
#define EditFlagsBasePage_h
#pragma once

#include "utl/UI/CmdTagStore.h"
#include "utl/UI/LayoutChildPropertySheet.h"
#include "DetailBasePage.h"
#include "FlagsEdit.h"


abstract class CEditFlagsBasePage : public CDetailBasePage
								  , public ui::IEmbeddedPageCallback
{
protected:
	CEditFlagsBasePage( const std::tstring& section, UINT templateId = 0 );
public:
	virtual ~CEditFlagsBasePage();

	// IWndObserver interface
	virtual bool IsDirty( void ) const;
	virtual void OnTargetWndChanged( const CWndSpot& targetWnd );

	virtual void ApplyPageChanges( void ) throws_( CRuntimeException );

	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;
protected:
	// ui::IEmbeddedPageCallback interface
	virtual void OnChildPageNotify( CLayoutPropertyPage* pEmbeddedPage, CWnd* pCtrl, int notifCode );

	enum FlagsPage { GeneralPage, SpecificPage, PageCount };

	bool UseSpecificFlags( void ) const { return m_pSpecificFlags.get() != nullptr; }
	void SetSpecificFlags( DWORD specificFlags );

	virtual void StoreFlagStores( HWND hTargetWnd ) = 0;

	void LoadPageInfo( UINT id );
	void QueryAllFlagStores( std::vector<const CFlagStore*>& rFlagStores ) const;
	DWORD EvalUnknownFlags( DWORD flags ) const;
	void InputFlags( DWORD* pFlags, DWORD* pSpecificFlags = nullptr ) const;
protected:
	std::tstring m_wndClass;
	const CFlagStore* m_pGeneralStore;
	const CFlagStore* m_pSpecificStore;

	DWORD m_flags;
	std::auto_ptr<DWORD> m_pSpecificFlags;		// for extended styles class specific is separate from m_flags

	HWND m_hWndLastTarget;
	ui::CCmdTag m_pageInfo;						// caption, tooltip
	std::tstring m_childPageTipText[ PageCount ];
	std::tstring m_unknownTipLabel;
protected:
	// enum { IDD = IDD_EDIT_FLAGS_PAGE };

	CFlagsEdit m_totalEdit;
	CFlagsEdit m_unknownEdit;

	CLayoutChildPropertySheet m_listSheet;
	CBaseFlagsCtrl* m_pGeneralList;
	CBaseFlagsCtrl* m_pSpecificList;
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
	virtual BOOL OnTtnNeedText( UINT cmdId, NMHDR* pNmHdr, LRESULT* pResult );
protected:
	afx_msg void OnFnFlagsChanged_TotalEdit( void );
	afx_msg void OnFnFlagsChanged_UnknownEdit( void );
	afx_msg void OnFnFlagsChanged_GeneralList( void );
	afx_msg void OnFnFlagsChanged_SpecificList( void );

	DECLARE_MESSAGE_MAP()
};


#endif // EditFlagsBasePage_h
