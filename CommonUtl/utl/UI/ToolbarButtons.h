#ifndef ToolbarButtons_h
#define ToolbarButtons_h
#pragma once

#include "utl/StdHashValue.h"
#include "ui_fwd.h"
#include "DataAdapters.h"
#include <afxtoolbarcomboboxbutton.h>
#include <afxtoolbareditboxbutton.h>
#include <unordered_map>


class CEnumTags;


namespace mfc
{
	std::tstring GetComboSelText( const CMFCToolBarComboBoxButton* pComboBtn, ui::ComboField byField = ui::BySel );
	std::pair<bool, ui::ComboField> SetComboEditText( OUT CMFCToolBarComboBoxButton* pComboBtn, const std::tstring& newText );
	void WriteComboItems( OUT CMFCToolBarComboBoxButton* pComboBtn, const std::vector<std::tstring>& items );

	DWORD ToolBarComboBoxButton_GetStyle( const CMFCToolBarComboBoxButton* pComboButton );

	bool StretchToolbarButton( CMFCToolBarButton* pStretchableButton, int rightSpacing = 2 );


	// App singleton that stores all custom toolbar button pointers that must be re-bound when CToolBar::LoadState() - deserializing.
	// The custom button class must override CMyCustomButton::OnChangeParentWnd() and rebind the pointer data-members, e.g. "CEnumTags* m_pEnumTags".
	// IMP: the client code is repsonsible that stored pointers must be valid during the lifetime of custom buttons!

	class CToolbarButtonsRefBinder
	{
		CToolbarButtonsRefBinder( void ) {}
	public:
		static CToolbarButtonsRefBinder* Instance( void );

		void RegisterPointer( UINT btnId, int ptrPos, const void* pRebindableData );

		template< typename Type >
		bool RebindPointer( OUT Type*& rPtr, UINT btnId, int ptrPos ) const
		{
			rPtr = reinterpret_cast<Type*>( RebindPointerData( btnId, ptrPos ) );
			ENSURE( nullptr == rPtr || is_a<Type>( rPtr ) );		// we didn't mess-up the ptrPos sub-key?
			return rPtr != nullptr;
		}
	private:
		void* RebindPointerData( UINT btnId, int ptrPos ) const;
	private:
		typedef std::pair<UINT, int> TBtnId_PtrPosPair;			// <btnId, ptrPos> - e.g. ptrPos is 0 for pointer data-member 1, 1 for pointer data-member 2
		typedef std::unordered_map<TBtnId_PtrPosPair, void*, utl::CPairHasher> TRebindMap;

		TRebindMap m_btnRebindPointers;		// <BtnId_PtrPosPair, pointer> - NULL data pointers are allowed as valid values
	};
}


namespace mfc
{
	enum ButtonOptionFlags
	{
		BO_Transparent	= BIT_FLAG( 0 ),
		BO_BoldFont		= BIT_FLAG( 1 ),

		BO_QueryToolTip	= BIT_FLAG( 8 ),
		BO_StretchWidth	= BIT_FLAG( 9 )
	};


	class CLabelButton : public CMFCToolBarButton
	{
		DECLARE_SERIAL( CLabelButton )
	protected:
		CLabelButton( void );
	public:
		CLabelButton( UINT btnId, int optionFlags = 0, const TCHAR* pText = nullptr );

		bool HasOptionFlag( int flag ) const { return HasFlag( m_optionFlags, flag ); }
		void SetOptionFlag( int flag, bool on = true ) { SetFlag( m_optionFlags, flag, on ); }

		// base overrides
		virtual void Serialize( CArchive& archive );
		virtual BOOL OnUpdateToolTip( CWnd* pWndParent, int buttonIndex, CToolTipCtrl& wndToolTip, CString& rTipText );
		virtual SIZE OnCalculateSize( CDC* pDC, const CSize& sizeDefault, BOOL horz );
		virtual void OnDraw( CDC* pDC, const CRect& rect, CMFCToolBarImages* pImages,
							 BOOL bHorz = TRUE, BOOL bCustomizeMode = FALSE, BOOL bHighlight = FALSE, BOOL bDrawBorder = TRUE, BOOL bGrayDisabledButtons = TRUE ) override;
	protected:
		virtual void CopyFrom( const CMFCToolBarButton& src ) override;

		CFont* GetFont( void ) const;
	private:
		persist int m_optionFlags;
	};
}


namespace mfc
{
	class CEditBoxButton : public CMFCToolBarEditBoxButton
	{
		DECLARE_SERIAL( CEditBoxButton )
	protected:
		CEditBoxButton( void );
	public:
		CEditBoxButton( UINT btnId, int width, DWORD dwStyle = ES_AUTOHSCROLL, int optionFlags = 0 );
		virtual ~CEditBoxButton();

		bool HasOptionFlag( int flag ) const { return HasFlag( m_optionFlags, flag ); }
		void SetOptionFlag( int flag, bool on = true ) { SetFlag( m_optionFlags, flag, on ); OnOptionFlagsChanged(); }

		// base overrides
		virtual void Serialize( CArchive& archive );
		virtual void OnChangeParentWnd( CWnd* pParentWnd );
		virtual CEdit* CreateEdit( CWnd* pWndParent, const CRect& rect );
		virtual void OnGlobalFontsChanged( void );
		virtual BOOL OnUpdateToolTip( CWnd* pWndParent, int buttonIndex, CToolTipCtrl& wndToolTip, CString& rTipText );
	protected:
		virtual void CopyFrom( const CMFCToolBarButton& src ) override;

		void OnOptionFlagsChanged( void );
	private:
		persist int m_optionFlags;
	};


	class CToolBarEditCtrl : public CMFCToolBarEditCtrl
	{
	public:
		CToolBarEditCtrl( CMFCToolBarEditBoxButton& editButton, bool transparent );
	private:
		bool m_transparent;

		// generated stuff
	protected:
		afx_msg BOOL OnEraseBkgnd( CDC* pDC );
		HBRUSH CtlColor( CDC* pDC, UINT ctlColor );
		afx_msg BOOL OnEnUpdate_Reflect( void );

		DECLARE_MESSAGE_MAP()
	};
}


namespace mfc
{
	class CEnumComboBoxButton : public CMFCToolBarComboBoxButton
	{
		DECLARE_SERIAL( CEnumComboBoxButton )
	protected:
		CEnumComboBoxButton( void );
	public:
		CEnumComboBoxButton( UINT btnId, const CEnumTags* pEnumTags, int width, DWORD dwStyle = CBS_DROPDOWNLIST );
		virtual ~CEnumComboBoxButton();

		bool HasValidValue( void ) const { return GetCurSel() != -1; }
		int GetValue( void ) const;
		bool SetValue( int value );

		template< typename EnumType >
		EnumType GetEnum( void ) const { return (EnumType)GetValue(); }

		template<>
		bool GetEnum<bool>( void ) const { return GetValue() != 0; }

		const CEnumTags* GetTags( void ) const { return m_pEnumTags; }
		void SetTags( const CEnumTags* pEnumTags );

		// base overrides
		virtual void OnChangeParentWnd( CWnd* pParentWnd );
		virtual BOOL CanBeStretched( void ) const { return true; }
	protected:
		virtual void CopyFrom( const CMFCToolBarButton& src ) overrides(CMFCToolBarComboBoxButton);
	private:
		rebound const CEnumTags* m_pEnumTags;
	};
}


namespace mfc
{
	class CStockValuesComboBoxButton : public CMFCToolBarComboBoxButton
	{
		DECLARE_SERIAL( CStockValuesComboBoxButton )
	protected:
		CStockValuesComboBoxButton( void );
	public:
		CStockValuesComboBoxButton( UINT btnId, const ui::IStockTags* pStockTags, int width, DWORD dwStyle = CBS_DROPDOWN | CBS_DISABLENOSCROLL );
		virtual ~CStockValuesComboBoxButton();

		const ui::IStockTags* GetTags( void ) const { return m_pStockTags; }

		// input/output
		template< typename ValueT >
		bool OutputValue( ValueT value );

		template< typename ValueT >
		bool InputValue( OUT ValueT* pOutValue, ui::ComboField byField, bool showErrors = true ) const;		// ui::BySel for CBN_SELCHANGE, ui::ByEdit
	private:
		template< typename ValueT >
		typename const ui::CStockTags<ValueT>* GetTagsAs( void ) const { return checked_static_cast< const ui::CStockTags<ValueT>* >( m_pStockTags ); }

		void SetTags( const ui::IStockTags* pStockTags );

		bool OutputTag( const std::tstring& tag );
		void OnInputError( void ) const;

		// base overrides
	public:
		virtual void OnChangeParentWnd( CWnd* pParentWnd );
		virtual BOOL CanBeStretched( void ) const { return true; }
	protected:
		virtual void CopyFrom( const CMFCToolBarButton& src ) overrides(CMFCToolBarComboBoxButton);
	private:
		rebound const ui::IStockTags* m_pStockTags;
	};
}


class CSliderCtrl;


namespace ui
{
	// CSliderCtrl utils
	bool IsValidPos( const CSliderCtrl* pSliderCtrl, int newPos );
	bool ValidatePos( const CSliderCtrl* pSliderCtrl, OUT int* pNewPos );		// true if changed, after clamping
}


namespace mfc
{
	class CCustomSliderCtrl;


	class CSliderButton : public CMFCToolBarButton
	{
		DECLARE_SERIAL( CSliderButton )

		CSliderButton( void );
	public:
		CSliderButton( UINT btnId, int width = 0, DWORD dwStyle = DefaultStyle );
		virtual ~CSliderButton();

		CSliderCtrl* GetSliderCtrl( void ) const;

		const Range<int>& GetRange( void ) const { return m_limits; }
		void SetRange( int minValue, int maxValue );	// will sync all buttons
		bool SetCountRange( size_t count, size_t tickFreqThresholdCount = TickFreqThresholdCount );		// set [0, count-1] range, auto tick frequency; will sync all buttons

		int GetPos( void ) const;
		bool SetPos( int pos, bool notify, bool syncAll = true );
	private:
		void SetLimits( const Range<int>& limits );

		// base overrides
	public:
		virtual void Serialize( CArchive& archive );
		virtual SIZE OnCalculateSize( CDC* pDC, const CSize& sizeDefault, BOOL horz );
		virtual void OnShow( BOOL show );
		virtual void OnChangeParentWnd( CWnd* pParentWnd );
		virtual void OnMove( void );
		virtual void OnSize( int width );
		virtual HWND GetHwnd( void ) { return GetSliderCtrl()->GetSafeHwnd(); }
		virtual BOOL CanBeStretched( void ) const { return TRUE; }
		virtual BOOL HaveHotBorder( void ) const { return FALSE; }

		virtual void OnDraw( CDC* pDC, const CRect& rect, CMFCToolBarImages* pImages,
							 BOOL bHorz = TRUE, BOOL bCustomizeMode = FALSE,
							 BOOL bHighlight = FALSE,
							 BOOL bDrawBorder = TRUE,
							 BOOL bGrayDisabledButtons = TRUE );		// no-op
	protected:
		virtual void CopyFrom( const CMFCToolBarButton& src );
	private:
		persist int m_width;
		persist DWORD m_dwStyle;
		persist Range<int> m_limits;
		persist int m_pos;

		std::auto_ptr<CCustomSliderCtrl> m_pSliderCtrl;

		enum { DefaultStyle = TBS_HORZ | TBS_AUTOTICKS | TBS_TRANSPARENTBKGND | TBS_NOTIFYBEFOREMOVE | TBS_TOOLTIPS };
		enum { DefaultWidth = 150, DefaultHeight = 22 /*25*/, TickFreqThresholdCount = 30 };
	};
}


namespace mfc
{
	// CStockValuesComboBoxButton template code

	template< typename ValueT >
	bool CStockValuesComboBoxButton::OutputValue( ValueT value )
	{
		if ( const ui::CStockTags<ValueT>* pStockTags = GetTagsAs<ValueT>() )
			return OutputTag( pStockTags->FormatValue( value ) );

		ASSERT( false );
		return false;
	}

	template< typename ValueT >
	bool CStockValuesComboBoxButton::InputValue( OUT ValueT* pOutValue, ui::ComboField byField, bool showErrors /*= true*/ ) const
	{
		if ( const ui::CStockTags<ValueT>* pStockTags = GetTagsAs<ValueT>() )
		{
			std::tstring currentTag = mfc::GetComboSelText( this, byField );

			if ( currentTag.empty() )
				return false;			// transient empty text, e.g. when the combo list is still dropped down, etc

			if ( pStockTags->ParseValue( pOutValue, currentTag ) )
				if ( pStockTags->IsValidValue( *pOutValue ) )
					return true;

			OnInputError();		// give owner a chance to restore previous valid value

			if ( showErrors )
				return ui::ShowInputError( GetComboBox(), pStockTags->FormatValidationError(), MB_ICONERROR );		// invalid input
		}
		else
			ASSERT( false );

		return false;
	}
}


#endif // ToolbarButtons_h
