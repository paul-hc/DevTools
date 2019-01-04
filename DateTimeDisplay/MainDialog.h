#ifndef MainDialog_h
#define MainDialog_h
#pragma once

#include "utl/Range.h"
#include "utl/UI/BaseMainDialog.h"
#include "utl/UI/InternalChange.h"
#include "utl/UI/SyncScrolling.h"
#include "utl/UI/TextEditor.h"
#include "DateTimeInfo.h"


class CMainDialog : public CBaseMainDialog
{
public:
	CMainDialog( CWnd* pParent = NULL );

	// base overrides
	virtual void SaveToRegistry( void );
private:
	void ParseInput( void );
	void ReadInputSelLine( void );
	void SetResultsText( const std::tstring& outputText );

	enum PickerFormat { FmtDateTime, FmtDuration, FmtEmpty };
	bool SetFormatCurrLinePicker( PickerFormat pickerFormat, const CTimeSpan* pDuration = NULL );
	void EnableApplyButton( void );

	static const std::tstring GetDefaultInputText( const CTime& dateTime = CTime::GetCurrentTime() );		// CTime( 1446294619 ): "Oct 31, 2015 - 12:30:19"
private:
	std::tstring m_inputText;
	Range< int > m_inputSel;
	std::vector< CDateTimeInfo > m_infos;

	std::tstring m_inputCaretLineText;
	std::tstring m_currFormat;
	static const TCHAR* m_format[];
	static const CTime m_midnight;

	// enum { IDD = IDD_MAIN_DIALOG };
	CTextEditor m_inputEdit;
	CTextEdit m_resultsEdit;
	CDateTimeCtrl m_currPicker;
	CComboBox m_currTypeCombo;
	CInternalChange m_lineChange;
	CSyncScrolling m_syncScrolling;

	// generated overrides
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	// generated message map
	afx_msg LRESULT OnKickIdle( WPARAM, LPARAM );
	afx_msg void OnEnChangeSourceTextEdit( void );
	afx_msg void OnEnVScroll( UINT editId );
	afx_msg void OnDateTimeChange_CurrLinePicker( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnCbnSelChange_CurrTypeCombo( void );
	afx_msg void OnSelectAll( void );
	afx_msg void OnCurrLineApply( void );
	afx_msg void OnClearAll( void );

	DECLARE_MESSAGE_MAP()
};


#endif // MainDialog_h
