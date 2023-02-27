#ifndef TokenizeTextDialog_h
#define TokenizeTextDialog_h
#pragma once

#include "utl/UI/IconButton.h"
#include "utl/UI/LayoutDialog.h"
#include "utl/UI/TandemControls.h"
#include "utl/UI/TextEdit.h"
#include "utl/UI/ThemeStatic.h"
#include "LineSet.h"


class CTokenizeTextDialog : public CLayoutDialog
{
public:
	CTokenizeTextDialog( CWnd* pParent = nullptr );
	virtual ~CTokenizeTextDialog();
private:
	void RegistryLoad( void );
	void RegistrySave( void );

	std::tstring GetSourceText( void ) const;
	std::tstring FormatOutputText( CLineSet& rLineSet ) const;
	std::tstring FormatResultsLabel( void ) const;

	std::tstring GenerateOutputText( void );

	bool IsParseAction( void ) const;
	bool IsLineAction( void ) const;
	bool IsTableAction( void ) const;
public:
	std::tstring m_sourceText;
	std::tstring m_outputText;
private:
	enum Action
	{
		SplitAction, TokenizeAction, MergeAction,								// Parse actions
		ReverseAction, SortAscAction, SortDescAction, FilterUniqueAction,		// Line actions
		TableToTreeAction,														// Table actions
			ActionCount,
			SepActionCount = MergeAction + 1
	};

	struct ActionSeparator
	{
		ActionSeparator( void ) {}
		ActionSeparator( const TCHAR* pInput, const TCHAR* pOutput ) : m_input( pInput ), m_output( pOutput ) {}

		std::tstring m_input;
		std::tstring m_output;
	};
private:
	Action m_action;
	ActionSeparator m_separators[ ActionCount ];
	size_t m_tokenCount;
	static const ActionSeparator m_defaultSeparators[ ActionCount ];

	// enum { IDD = IDD_TOKENIZE_TEXT_DIALOG };
	CHostToolbarCtrl<CTextEdit> m_sourceEdit;
	CHostToolbarCtrl<CTextEdit> m_outputEdit;
	CRegularStatic m_outputStatic;
	CTextEdit m_inputTokensEdit;
	CTextEdit m_outputSepsEdit;
	CIconButton m_defaultInputTokensButton;
	CIconButton m_defaultOutputSepsButton;
	CTextEdit m_trimCharsEdit;
	CIconButton m_defaultTrimCharsButton;
private:
	bool ActionUseSeps( void ) const { return m_action < SepActionCount; }

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	virtual BOOL OnInitDialog( void );
	afx_msg void OnDestroy( void );
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlColor );
	afx_msg void OnActionChanged( UINT radioId );
	afx_msg void OnActionOptionChanged( UINT radioId );
	afx_msg void OnDefaultInputTokens( void );
	afx_msg void OnDefaultOutputSeparator( void );
	afx_msg void OnDefaultTrimChars( void );
	afx_msg void OnCopyOutputToSource( void );
	afx_msg void OnEnChange_SourceText( void );
	afx_msg void OnEnChange_InputTokens( void );
	afx_msg void OnEnChange_OutputTokens( void );
	afx_msg void OnToggle_ShowWhiteSpace( void );
	afx_msg void OnFieldChanged( void );

	DECLARE_MESSAGE_MAP()
};


#endif // TokenizeTextDialog_h
