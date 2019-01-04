#ifndef TokenizeTextDialog_h
#define TokenizeTextDialog_h
#pragma once

#include "utl/UI/IconButton.h"
#include "utl/UI/LayoutDialog.h"
#include "utl/UI/TextEdit.h"
#include "LineSet.h"


class CTokenizeTextDialog : public CLayoutDialog
{
public:
	CTokenizeTextDialog( CWnd* pParent = NULL );
	virtual ~CTokenizeTextDialog();
private:
	void RegistryLoad( void );
	void RegistrySave( void );

	std::tstring GetSourceText( void ) const;
	std::tstring FormatOutputText( CLineSet& rLineSet ) const;

	std::tstring GenerateOutputText( void );
public:
	std::tstring m_sourceText;
	std::tstring m_outputText;
private:
	enum Action
	{
		SplitAction, TokenizeAction, MergeAction, ReverseAction, SortAscAction, SortDescAction, FilterUniqueAction,
		ActionCount, SepActionCount = MergeAction + 1
	};

	struct ActionSeparator
	{
		ActionSeparator( void ) {}
		ActionSeparator( const TCHAR* pInput, const TCHAR* pOutput ) : m_input( pInput ), m_output( pOutput ) {}

		std::tstring m_input;
		std::tstring m_output;
	};

	Action m_action;
	ActionSeparator m_separators[ ActionCount ];
	size_t m_tokenCount;
	static const ActionSeparator m_defaultSeparators[ ActionCount ];

	// enum { IDD = IDD_TOKENIZE_TEXT_DIALOG };
	CTextEdit m_sourceEdit;
	CTextEdit m_outputEdit;
	CTextEdit m_inputTokensEdit;
	CTextEdit m_outputSepsEdit;
	CIconButton m_defaultInputTokensButton;
	CIconButton m_defaultOutputSepsButton;
	CTextEdit m_trimCharsEdit;
	CIconButton m_defaultTrimCharsButton;
	CIconButton m_copyOutputToSourceButton;
private:
	bool ActionUseSeps( void ) const { return m_action < SepActionCount; }

	// generated function overrides
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	// generated message map functions
	afx_msg void OnDestroy( void );
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT ctlColor );
	afx_msg void OnActionChanged( UINT radioId );
	afx_msg void OnActionOptionChanged( UINT radioId );
	afx_msg void OnDefaultInputTokens( void );
	afx_msg void OnDefaultOutputSeparator( void );
	afx_msg void OnDefaultTrimChars( void );
	afx_msg void OnCopyOutputToSource( void );
	virtual BOOL OnInitDialog( void );
	afx_msg void OnEnChange_SourceText( void );
	afx_msg void OnEnChange_InputTokens( void );
	afx_msg void OnEnChange_OutputTokens( void );
	afx_msg void OnToggle_ShowWhiteSpace( void );
	afx_msg void OnFieldChanged( void );

	DECLARE_MESSAGE_MAP()
};


#endif // TokenizeTextDialog_h
