#ifndef FlagsListCtrl_h
#define FlagsListCtrl_h
#pragma once

#include "utl/UI/ReportListControl.h"
#include "BaseFlagsCtrl.h"


class CFlagsListCtrl : public CReportListControl
					 , public CBaseFlagsCtrl
{
public:
	CFlagsListCtrl( void );
	virtual ~CFlagsListCtrl();

	// base overrides
	virtual CMenu* GetPopupMenu( ListPopup popupType );
protected:
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const;

	virtual void OutputFlags( void );
	virtual void InitControl( void );

	DWORD InputFlags( void ) const;
private:
	void AddFlagItem( int itemIndex, const CFlagInfo* pFlag );

	const CFlagInfo* GetFlagInfoAt( int pos ) const { return GetPtrAt<CFlagInfo>( pos ); }
	void SetFlagInfoAt( int pos, const CFlagInfo* pFlagInfo ) { VERIFY( SetPtrAt( pos, const_cast<CFlagInfo*>( pFlagInfo ) ) ); }

	CFlagGroup* GetFlagGroup( int groupId ) const;
	int FindGroupIdWithFlag( const CFlagInfo* pFlag ) const;

	void QueryCheckedFlagWorkingSet( std::vector<const CFlagInfo*>& rFlagSet, int index ) const;
	bool AnyFlagCheckConflict( const std::vector<const CFlagInfo*>& flagSet, bool checked ) const;
	bool SetFlagsChecked( const std::vector<const CFlagInfo*>& flagSet, bool checked );
private:
	CMenu m_contextMenu;
public:
	// generated overrides
	virtual void PreSubclassWindow( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	afx_msg BOOL OnLvnToggleCheckState_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg BOOL OnLvnCheckStatesChanged_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg BOOL OnLvnLinkClick_Reflect( NMHDR* pNmHdr, LRESULT* pResult );			// group task click
	afx_msg void OnCopy( void );
	afx_msg void OnUpdateCopy( CCmdUI* pCmdUI );
	afx_msg void OnCopySelected( void );
	afx_msg void OnUpdateCopySelected( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // FlagsListCtrl_h
