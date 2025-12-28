#ifndef TextEdit_fwd_h
#define TextEdit_fwd_h
#pragma once


class CTextEdit;


namespace ui
{
	struct CTextValidator
	{
		CTextValidator( const CTextEdit* pEdit );

		bool HasLineMismatch( void ) { return m_itemCount != m_lines.size(); }
		bool HasAmendedText( void ) { return !m_amendedText.empty() && m_amendedText != m_inputText; }
		bool StoreAmendedText( void );
	public:
		const CTextEdit* m_pEdit;
		const std::tstring m_inputText;
		std::vector<std::tstring> m_lines;		// effective lines of text (no trailing empty line)
		size_t m_itemCount;						// multi-line edit-box: detect locked item count mismatch

		std::tstring m_amendedText;				// warning only: amended text that need to be output; if empty, RevertContents() is called
	};


	interface ITextInput
	{
		enum Result { Success, Warning, Error };

		virtual ui::ITextInput::Result OnEditInput( IN OUT ui::CTextValidator& rValidator ) = 0;
	};
}


#endif // TextEdit_fwd_h
