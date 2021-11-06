#ifndef HistoryComboBox_h
#define HistoryComboBox_h
#pragma once

#include "ui_fwd.h"
#include "AccelTable.h"
#include "ItemContent.h"
#include "BaseFrameHostCtrl.h"


#define ON_HCN_VALIDATEITEMS( id, memberFxn )		ON_CONTROL( CHistoryComboBox::HCN_VALIDATEITEMS, id, memberFxn )


class CComboDropList;
class CTextEditor;


class CHistoryComboBox : public CBaseFrameHostCtrl<CComboBox>
					   , public ui::IContentValidator
{
	typedef CBaseFrameHostCtrl<CComboBox> TBaseClass;
public:
	enum NotifCode { HCN_VALIDATEITEMS = CBN_SELENDCANCEL + 10 };		// note: notifications are suppressed during parent's UpdateData()
	enum InternalCmds { Cmd_ResetDropSelIndex = 350 };

	CHistoryComboBox( unsigned int maxCount = ui::HistoryMaxSize, const TCHAR* pItemSep = _T(";"), str::CaseType caseType = str::Case );
	virtual ~CHistoryComboBox();

	void SetMaxCount( unsigned int maxCount ) { m_maxCount = maxCount; }
	void SetItemSep( const TCHAR* pItemSep ) { ASSERT_PTR( pItemSep ); m_pItemSep = pItemSep; }

	str::CaseType GetCaseType( void ) const { return m_caseType; }
	void SetCaseType( str::CaseType caseType = str::Case ) { m_caseType = caseType; }

	void SaveHistory( const TCHAR* pSection, const TCHAR* pEntry );
	void LoadHistory( const TCHAR* pSection, const TCHAR* pEntry, const TCHAR* pDefaultText = NULL );

	std::tstring GetCurrentText( void ) const;
	std::pair< bool, ui::ComboField > SetEditText( const std::tstring& currText );
	void StoreCurrentEditItem( void );

	virtual const ui::CItemContent& GetItemContent( void ) const { return m_itemContent; }
	void SetItemContent( const ui::CItemContent& itemContent ) { m_itemContent = itemContent; }
	ui::CItemContent& RefItemContent( void ) { return m_itemContent; }

	CTextEditor* GetEdit( void ) const { return m_pEdit.get(); }
	void SetEdit( CTextEditor* pEdit );

	// ui::IContentValidator interface
	virtual void ValidateContent( void );
private:
	int GetCmdSelIndex( void ) const;
protected:
	unsigned int m_maxCount;
	const TCHAR* m_pItemSep;
	str::CaseType m_caseType;
private:
	ui::CItemContent m_itemContent;			// self-encapsulated
	CAccelTable m_accel, m_dropDownAccel;
	std::auto_ptr<CTextEditor> m_pEdit;
	std::auto_ptr<CComboDropList> m_pDropList;
	int m_dropSelIndex;						// used when tracking the context menu (highlighted item in LBox)
	LPCTSTR m_pSection, m_pEntry;

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );
	afx_msg void OnUpdateSelectedListItem( CCmdUI* pCmdUI );
	afx_msg void OnDeleteListItem( void );
	afx_msg void OnStoreEditItem( void );
	afx_msg void OnDeleteAllListItems( void );
	afx_msg void OnUpdateDeleteAllListItems( CCmdUI* pCmdUI );
	afx_msg void OnEditListItems( void );
	afx_msg void OnSaveHistory( void );
	afx_msg void OnResetDropSelIndex( void );

	DECLARE_MESSAGE_MAP()
};


#endif // HistoryComboBox_h
