#ifndef utl_MFC_Vista_TaskDialog_h
#define utl_MFC_Vista_TaskDialog_h
#pragma once

// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#if !defined( _UNICODE )
	#error CTaskDialog requires _UNICODE to be defined.
#endif

#if ( NTDDI_VERSION < NTDDI_VISTA )
	#error CTaskDialog is not supported on Windows versions prior to Vista.
#endif

#ifndef TDF_SIZE_TO_CONTENT
	#define TDF_SIZE_TO_CONTENT 0x01000000
#endif


#include <commctrl.h>
#include "Range.h"


namespace ui
{
	class CTaskDialog : public CObject
		, private utl::noncopyable
	{
		DECLARE_DYNAMIC( CTaskDialog )
	public:
		CTaskDialog( const std::tstring& title, const std::tstring& mainInstructionText, const std::tstring& contentText,
					 int commonButtonFlags = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON,
					 int optionFlags = TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS,
					 const std::tstring& footerText = s_empty );

		CTaskDialog( const std::tstring& title, const std::tstring& mainInstructionText, const std::tstring& contentText,
					 int firstButtonId, int lastButtonId,
					 int commonButtonFlags,
					 int optionFlags = TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS,
					 const std::tstring& footerText = s_empty );
		virtual ~CTaskDialog();

		INT_PTR DoModal( CWnd* pParent );		// CWnd::GetActiveWindow()

		static bool IsSupported( void );

		static INT_PTR ShowDialog( CWnd* pParent, const std::tstring& title, const std::tstring& mainInstructionText, const std::tstring& contentText,
								   int firstButtonId, int lastButtonId,
								   int commonButtonFlags = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON, int optionFlags = TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS, const std::tstring& footerText = s_empty );

		void LoadButtons( int firstButtonId, int lastButtonId );
		void LoadRadioButtons( int firstRadioButtonId, int lastRadioButtonId );

		void SetTitle( const std::tstring& title );
		void SetMainInstructionText( const std::tstring& mainInstructionText );
		void SetContentText( const std::tstring& contentText );
		void SetFooterText( const std::tstring& footerText );
		void SetExpansionArea( const std::tstring& expandedInfoText, const std::tstring& collapsedLabel = s_empty, const std::tstring& expandedLabel = s_empty );

		void SetMainIcon( HICON hMainIcon );
		void SetFooterIcon( HICON hFooterIcon );

		void SetMainIcon( LPCWSTR lpszMainIcon );
		void SetFooterIcon( LPCWSTR lpszFooterIcon );

		void SetDialogWidth( int width = 0 ) { ASSERT_NULL( m_hWnd ); ASSERT( width >= 0 ); m_width = width; }

		void SetOptions( int optionFlags );
		int GetOptions( void ) const { return m_optionFlags; }

		void SetCommonButtons( int buttonMask, int disabledButtonMask = 0, int elevationButtonMask = 0 );
		void SetCommonButtonOptions( int disabledButtonMask, int nElevationButtonMask = 0 );

		void AddButton( int commandControlId, const std::tstring& caption, bool enabled = true, bool requiresElevation = false );
		void SetButtonOptions( int commandControlId, bool enabled, bool requiresElevation = false );
		int GetSelectedButtonID( void ) const { return m_buttonId; }
		bool IsButtonEnabled( int commandControlId ) const;
		void SetDefaultButton( int commandControlId );
		void RemoveAllButtons( void ) { ASSERT_NULL( m_hWnd ); m_buttons.clear(); }

		void AddRadioButton( int radioButtonId, const std::tstring& caption, bool enabled = true );
		void SetRadioButtonOptions( int radioButtonId, bool enabled );
		int GetSelectedRadioButtonID( void ) const { return m_radioId; }
		bool IsRadioButtonEnabled( int radioButtonId ) const;
		void SetDefaultRadioButton( int radioButtonId );
		void RemoveAllRadioButtons( void );

		// verification checkbox
		bool UseVerification( void ) const { return !m_verificationText.empty(); }
		void SetVerificationText( const std::tstring& verificationText );

		bool IsVerificationChecked( void ) const { return m_verificationChecked != FALSE; }
		void SetVerificationChecked( bool verificationChecked = true );

		const Range<int>& GetProgressBarRange( void ) const { return m_progressRange; }
		void SetProgressBarRange( int rangeMin, int rangeMax );
		void SetProgressBarPosition( int progressPos );
		void SetProgressBarState( int state = PBST_NORMAL );
		void SetProgressBarMarquee( bool enabled = true, int marqueeSpeed = 0 );
	protected:
		void ClickRadioButton( int radioButtonId ) const;
		void ClickButton( int commandControlId ) const;
		void NavigateTo( CTaskDialog& rTaskDialog ) const;

		virtual HRESULT OnCreate( void );
		virtual HRESULT OnInit( void );
		virtual HRESULT OnDestroy( void );
		virtual HRESULT OnButtonClick( int commandControlId );
		virtual HRESULT OnRadioButtonClick( int radioButtonId );
		virtual HRESULT OnVerificationCheckboxClick( bool checked );
		virtual HRESULT OnExpandButtonClick( bool expanded );
		virtual HRESULT OnHyperlinkClick( const std::tstring& href );
		virtual HRESULT OnHelp( void );
		virtual HRESULT OnTimer( long lTime );
		virtual HRESULT OnNavigatePage( void );

		virtual int GetCommonButtonId( int flag ) const;
		virtual int GetCommonButtonFlag( int buttonId ) const;
		virtual int GetCommonButtonCount( void ) const;
	private:
		void Notify( UINT uMsg, WPARAM wParam, LPARAM lParam ) const;
		void FillStruct( TASKDIALOGCONFIG& rConfig );
		void FreeStruct( TASKDIALOGCONFIG& rConfig );

		friend HRESULT CALLBACK TaskDialogCallback( HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData );
	private:
		enum ButtonStateFlag { BUTTON_ENABLED = 0x01, BUTTON_ELEVATION = 0x02 };

		struct CButtonInfo
		{
			CButtonInfo( int buttonId = 0, const std::tstring& caption = s_empty, unsigned char state = 0 )
				: m_buttonId( buttonId ), m_caption( caption ), m_state( state ) {}
		public:
			int m_buttonId;
			std::tstring m_caption;
			unsigned char m_state;
		};

		union IconInfo
		{
			HICON hIcon;
			PCWSTR pszIcon;
		};

		TASKDIALOG_BUTTON* MakeButtonData( const std::vector<CButtonInfo>& buttons ) const;
		static INT_PTR FindButtonIndex( const std::vector<CButtonInfo>& buttons, int buttonId );
	private:
		HWND m_hWnd;

		std::tstring m_title;
		std::tstring m_mainInstructionText;
		std::tstring m_contentText;
		std::tstring m_footerText;
		std::tstring m_verificationText;

		// expansion area
		std::tstring m_expandedInfoText;
		std::tstring m_collapsedLabel;
		std::tstring m_expandedLabel;

		int m_commonButtonFlags;		// IDOK, IDCANCEL
		int m_buttonDisabled;
		int m_buttonElevation;

		int m_optionFlags;
		int m_width;
		int m_defaultButton;
		int m_defaultRadioButton;

		// progress bar
		Range<int> m_progressRange;
		int m_progressState;			// ProgressBar or Marquee state
		int m_progressPos;				// ProgressBar pos or Marquee speed

		IconInfo m_mainIcon;
		IconInfo m_footerIcon;

		std::vector<CButtonInfo> m_buttons;
		std::vector<CButtonInfo> m_radioButtons;

		// results
		BOOL m_verificationChecked;		// verification checkbox state
		int m_radioId;
		int m_buttonId;
	public:
		static const std::tstring s_empty;
	};
}


#endif // utl_MFC_Vista_TaskDialog_h
