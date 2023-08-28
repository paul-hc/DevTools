
#include "pch.h"
#include "ToolbarButtons.h"
#include "ControlBar_fwd.h"
#include "PopupMenus_fwd.h"
#include "ComboBoxEdit.h"
#include "WndUtils.h"
#include "utl/Algorithms.h"
#include "utl/EnumTags.h"
#include "utl/ScopedValue.h"
#include "utl/SerializeStdTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace nosy
{
	struct CToolBarComboBoxButton_ : public CMFCToolBarComboBoxButton
	{
		// public access
		using CMFCToolBarComboBoxButton::m_dwStyle;
	};
}


namespace mfc
{
	std::tstring GetComboSelText( const CMFCToolBarComboBoxButton* pComboBtn, ui::ComboField byField /*= ui::BySel*/ )
	{
		ASSERT_PTR( pComboBtn );

		int selIndex = pComboBtn->GetCurSel();

		if ( ui::ByEdit == byField || CB_ERR == selIndex )
			return pComboBtn->GetText();
		else
			return pComboBtn->GetItem( selIndex );
	}

	std::pair<bool, ui::ComboField> SetComboEditText( OUT CMFCToolBarComboBoxButton* pComboBtn, const std::tstring& newText )
	{
		ASSERT_PTR( pComboBtn );

		int selIndex = pComboBtn->FindItem( newText.c_str() );
		std::pair<bool, ui::ComboField> result( newText != pComboBtn->GetText(), selIndex != CB_ERR ? ui::BySel : ui::ByEdit );

		if ( result.first )		// text will change?
		{
			pComboBtn->SelectItem( selIndex, false );			// select/unselect index preemptively, so that CMFCToolBarComboBoxButton::SetText() does not send notification
			pComboBtn->SetText( newText.c_str() );

			if ( pComboBtn->GetEditCtrl()->GetSafeHwnd() != nullptr )
				pComboBtn->GetEditCtrl()->SetSel( 0, -1 );		// select all edit text
		}

		return result;
	}

	void WriteComboItems( OUT CMFCToolBarComboBoxButton* pComboBtn, const std::vector<std::tstring>& items )
	{
		ASSERT_PTR( pComboBtn );

		pComboBtn->RemoveAllItems();

		for ( std::vector<std::tstring>::const_iterator it = items.begin(); it != items.end(); ++it )
			pComboBtn->AddItem( it->c_str() );
	}


	DWORD ToolBarComboBoxButton_GetStyle( const CMFCToolBarComboBoxButton* pComboButton )
	{
		return mfc::nosy_cast<nosy::CToolBarComboBoxButton_>( pComboButton )->m_dwStyle;
	}


	bool StretchToolbarButton( CMFCToolBarButton* pStretchableButton, int rightSpacing /*= 2*/ )
	{
		// advanced method: called during CMFCToolBar::AdjustLocations(), after buttons have been layed-out and tooltips get updated via UpdateTooltips()
		const CMFCToolBar* pToolBar = dynamic_cast<const CMFCToolBar*>( pStretchableButton->GetParentWnd() );

		if ( nullptr == pToolBar || !HasFlag( pToolBar->GetCurrentAlignment(), CBRS_ORIENT_HORZ ) )
			return false;		// not a horizontal layout toolbar

		CRect barClientRect;
		pToolBar->GetClientRect( &barClientRect );

		// stretch this button, and move the trailing buttons to the right
		const CObList& buttons = pToolBar->GetAllButtons();
		CMFCToolBarButton* pLastButton = (CMFCToolBarButton*)buttons.GetTail();
		int stretchBy = barClientRect.right - pLastButton->Rect().right - rightSpacing;		// horizontal slack delta

		if ( stretchBy <= 0 )
			return false;					// no more space for stretching

		CRect buttonRect = pStretchableButton->Rect();

		buttonRect.right += stretchBy;		// stretch this button
		pStretchableButton->SetRect( buttonRect );

		// shift trailing buttons to the right (if any)
		POSITION pos = buttons.Find( pStretchableButton );
		CMFCToolBarButton* pButton = (CMFCToolBarButton*)buttons.GetNext( pos );

		ENSURE( pStretchableButton == pButton );
		while ( pos != nullptr )
		{
			pButton = (CMFCToolBarButton*)buttons.GetNext( pos );
			if ( pButton == nullptr )
				break;

			buttonRect = pButton->Rect();
			buttonRect.OffsetRect( stretchBy, 0 );		// shift button to the right
			pButton->SetRect( buttonRect );
		}

		return true;		// done stretching
	}


	// CToolbarButtonsRefBinder implementation

	CToolbarButtonsRefBinder* CToolbarButtonsRefBinder::Instance( void )
	{
		static CToolbarButtonsRefBinder s_refBinder;

		return &s_refBinder;
	}

	void CToolbarButtonsRefBinder::RegisterPointer( UINT btnId, int ptrPos, const void* pRebindableData )
	{
		m_btnRebindPointers[ TBtnId_PtrPosPair( btnId, ptrPos ) ] = const_cast<void*>( pRebindableData );
	}

	void* CToolbarButtonsRefBinder::RebindPointerData( UINT btnId, int ptrPos ) const
	{
		return utl::FindValue( m_btnRebindPointers, TBtnId_PtrPosPair( btnId, ptrPos ) );
	}
}


namespace mfc
{
	// CLabelButton implementation

	IMPLEMENT_SERIAL( CLabelButton, CMFCToolBarButton, 1 )

	CLabelButton::CLabelButton( void )
		: CMFCToolBarButton()
		, m_optionFlags( 0 )
	{
	}

	CLabelButton::CLabelButton( UINT btnId, int optionFlags /*= 0*/, const TCHAR* pText /*= nullptr*/ )
		: CMFCToolBarButton( btnId, mfc::FindButtonImageIndex( btnId ) )
		, m_optionFlags( optionFlags )
	{
		if ( pText != nullptr )
			m_strText = pText;

		m_bText = TRUE;
	}

	void CLabelButton::Serialize( CArchive& archive )
	{
		__super::Serialize( archive );

		archive & m_optionFlags;
	}

	void CLabelButton::CopyFrom( const CMFCToolBarButton& src ) override
	{
		__super::CopyFrom( src );

		const CLabelButton& srcButton = (const CLabelButton&)src;

		m_optionFlags = srcButton.m_optionFlags;
	}

	BOOL CLabelButton::OnUpdateToolTip( CWnd* pWndParent, int buttonIndex, CToolTipCtrl& wndToolTip, CString& rTipText )
	{
		enum { RightSpacing = 2 };

		if ( HasOptionFlag( BO_StretchWidth ) )
			mfc::StretchToolbarButton( this, RightSpacing );

		if ( HasOptionFlag( BO_QueryToolTip ) )
			return false;			// dynamic query for tooltip text, evenually via frame's ui::ICustomCmdInfo interface

		return __super::OnUpdateToolTip( pWndParent, buttonIndex, wndToolTip, rTipText );
	}

	void CLabelButton::OnDraw( CDC* pDC, const CRect& rect, CMFCToolBarImages* pImages,
							   BOOL bHorz /*= TRUE*/, BOOL /*bCustomizeMode = FALSE*/, BOOL /*bHighlight = FALSE*/,
							   BOOL /*bDrawBorder = TRUE*/,	BOOL /*bGrayDisabledButtons = TRUE*/ ) override
	{
		CScopedGdi<CFont> scFont( pDC, &( HasOptionFlag( BO_BoldFont ) ? GetGlobalData()->fontBold : GetGlobalData()->fontRegular ) );
		CScopedValue<UINT> scStyle( &m_nStyle );		// display as enabled while drawing

		scStyle.SetFlag( TBBS_DISABLED, false );

		CMFCToolBarButton::OnDraw( pDC, rect, pImages, bHorz, FALSE, FALSE, FALSE, FALSE );
	}
}


namespace mfc
{
	// CEditBoxButton implementation

	IMPLEMENT_SERIAL( CEditBoxButton, CMFCToolBarEditBoxButton, 1 );

	CEditBoxButton::CEditBoxButton( void )
		: CMFCToolBarEditBoxButton()
		, m_optionFlags( 0 )
	{
	}

	CEditBoxButton::CEditBoxButton( UINT btnId, int width, DWORD dwStyle /*= ES_AUTOHSCROLL*/, int optionFlags /*= 0*/ )
		: CMFCToolBarEditBoxButton( btnId, mfc::FindButtonImageIndex( btnId ), dwStyle, width )
		, m_optionFlags( optionFlags )
	{
		OnOptionFlagsChanged();
	}

	CEditBoxButton::~CEditBoxButton()
	{
	}

	void CEditBoxButton::OnOptionFlagsChanged( void )
	{
		ASSERT_NULL( GetHwnd() );

		SetFlag( m_dwStyle, ES_MULTILINE, HasOptionFlag( BO_Transparent ) );		// transparency requires ES_MULTILINE edit style!  (otherwise the background turns black)
	}

	void CEditBoxButton::Serialize( CArchive& archive )
	{
		__super::Serialize( archive );

		archive & m_optionFlags;
	}

	void CEditBoxButton::CopyFrom( const CMFCToolBarButton& src ) override
	{
		__super::CopyFrom( src );

		const CEditBoxButton& srcButton = (const CEditBoxButton&)src;

		m_optionFlags = srcButton.m_optionFlags;
	}

	void CEditBoxButton::OnChangeParentWnd( CWnd* pParentWnd )
	{
		__super::OnChangeParentWnd( pParentWnd );

		if ( HasOptionFlag( BO_BoldFont ) )
			m_pWndEdit->SetFont( &GetGlobalData()->fontBold );
	}

	CEdit* CEditBoxButton::CreateEdit( CWnd* pWndParent, const CRect& rect )
	{
		CToolBarEditCtrl* pWndEdit = new CToolBarEditCtrl( *this, HasOptionFlag( BO_Transparent ) );

		if ( !pWndEdit->Create( m_dwStyle, rect, pWndParent, m_nID ) )
		{
			delete pWndEdit;
			return nullptr;
		}

		if ( HasOptionFlag( BO_Transparent ) )
			pWndEdit->ModifyStyleEx( 0, WS_EX_TRANSPARENT );

		return pWndEdit;
	}

	void CEditBoxButton::OnGlobalFontsChanged( void )
	{
		__super::OnGlobalFontsChanged();

		if ( HasOptionFlag( BO_BoldFont ) )
			if ( m_pWndEdit->GetSafeHwnd() != nullptr )
				m_pWndEdit->SetFont( &GetGlobalData()->fontBold );
	}

	BOOL CEditBoxButton::OnUpdateToolTip( CWnd* pWndParent, int buttonIndex, CToolTipCtrl& wndToolTip, CString& rTipText )
	{
		enum { RightSpacing = 2 };

		if ( HasOptionFlag( BO_StretchWidth ) )
			mfc::StretchToolbarButton( this, RightSpacing );

		if ( HasOptionFlag( BO_QueryToolTip ) )
			return false;			// dynamic query for tooltip text, evenually via frame's ui::ICustomCmdInfo interface

		return __super::OnUpdateToolTip( pWndParent, buttonIndex, wndToolTip, rTipText );
	}


	// CToolBarEditCtrl implementation

	BEGIN_MESSAGE_MAP( CToolBarEditCtrl, CMFCToolBarEditCtrl )
		ON_WM_ERASEBKGND()
		ON_WM_CTLCOLOR_REFLECT()
		ON_CONTROL_REFLECT_EX( EN_UPDATE, OnEnUpdate_Reflect )
	END_MESSAGE_MAP()

	CToolBarEditCtrl::CToolBarEditCtrl( CMFCToolBarEditBoxButton& editButton, bool transparent )
		: CMFCToolBarEditCtrl( editButton )
		, m_transparent( transparent )
	{
		REQUIRE( m_transparent );		// Warning: transparency support does not work for edit box; the code is just experimental!
	}

	BOOL CToolBarEditCtrl::OnEraseBkgnd( CDC* pDC )
	{
		if ( m_transparent )
		{
			//CMFCVisualManager::GetInstance()->OnFillButtonInterior( pDC, &m_buttonEdit, clientRect, CMFCVisualManager::ButtonsIsRegular );
			return TRUE;
		}
		return __super::OnEraseBkgnd( pDC );
	}

	HBRUSH CToolBarEditCtrl::CtlColor( CDC* pDC, UINT ctlColor )
	{
		ctlColor;
		if ( m_transparent )
		{
			pDC->SetBkMode( TRANSPARENT );
			return (HBRUSH)::GetStockObject( HOLLOW_BRUSH );
		}

		return nullptr; //::GetSysColorBrush( COLOR_BTNFACE );
	}

	BOOL CToolBarEditCtrl::OnEnUpdate_Reflect( void )
	{
		CWnd* pToolBar = m_buttonEdit.GetParentWnd();
		CRect ctrlRect;

		GetWindowRect( &ctrlRect );
		pToolBar->ScreenToClient( &ctrlRect );
		pToolBar->InvalidateRect( &ctrlRect );
		pToolBar->UpdateWindow();

		return FALSE;					// continue routing
	}
}


namespace mfc
{
	IMPLEMENT_SERIAL( CEnumComboBoxButton, CMFCToolBarComboBoxButton, 1 )

	CEnumComboBoxButton::CEnumComboBoxButton( void )
		: CMFCToolBarComboBoxButton()
		, m_pEnumTags( nullptr )
	{
	}

	CEnumComboBoxButton::CEnumComboBoxButton( UINT btnId, const CEnumTags* pEnumTags, int width, DWORD dwStyle /*= CBS_DROPDOWNLIST*/ )
		: CMFCToolBarComboBoxButton( btnId, mfc::FindButtonImageIndex( btnId ), dwStyle, width )
		, m_pEnumTags( nullptr )
	{
		SetTags( pEnumTags );
	}

	CEnumComboBoxButton::~CEnumComboBoxButton()
	{
	}

	int CEnumComboBoxButton::GetValue( void ) const
	{
		ASSERT_PTR( m_pEnumTags );

		int selIndex = GetCurSel();
		if ( -1 == selIndex )
		{
			ASSERT( HasValidValue() );
			return -1;
		}

		return m_pEnumTags->GetSelValue<int>( selIndex );
	}

	bool CEnumComboBoxButton::SetValue( int value )
	{
		ASSERT_PTR( m_pEnumTags );

		if ( HasValidValue() && value == GetValue() )
			return false;

		VERIFY( SelectItem( m_pEnumTags->GetTagIndex( value ), false ) );
		return true;
	}

	void CEnumComboBoxButton::SetTags( const CEnumTags* pEnumTags )
	{
		m_pEnumTags = pEnumTags;
		mfc::CToolbarButtonsRefBinder::Instance()->RegisterPointer( m_nID, 0, m_pEnumTags );

		if ( m_pEnumTags != nullptr )
			mfc::WriteComboItems( this, m_pEnumTags->GetUiTags() );
		else
			RemoveAllItems();
	}

	void CEnumComboBoxButton::OnChangeParentWnd( CWnd* pParentWnd )
	{
		__super::OnChangeParentWnd( pParentWnd );

		if ( nullptr == m_pEnumTags )		// parent toolbar is loading state (de-serializing)?
			mfc::CToolbarButtonsRefBinder::Instance()->RebindPointer( m_pEnumTags, m_nID, 0 );
	}

	void CEnumComboBoxButton::CopyFrom( const CMFCToolBarButton& src ) overrides(CMFCToolBarComboBoxButton)
	{
		__super::CopyFrom( src );

		const CEnumComboBoxButton& srcButton = (const CEnumComboBoxButton&)src;

		m_pEnumTags = srcButton.m_pEnumTags;
		ASSERT_PTR( m_pEnumTags );
	}
}


namespace mfc
{
	// CStockValuesComboBoxButton implementation

	IMPLEMENT_SERIAL( CStockValuesComboBoxButton, CMFCToolBarComboBoxButton, 1 )

	CStockValuesComboBoxButton::CStockValuesComboBoxButton( void )
		: CMFCToolBarComboBoxButton()
		, m_pStockTags( nullptr )
	{
	}

	CStockValuesComboBoxButton::CStockValuesComboBoxButton( UINT btnId, const ui::IStockTags* pStockTags, int width, DWORD dwStyle /*= CBS_DROPDOWN | CBS_DISABLENOSCROLL*/ )
		: CMFCToolBarComboBoxButton( btnId, mfc::FindButtonImageIndex( btnId ), dwStyle, width )
		, m_pStockTags( nullptr )
	{
		SetTags( pStockTags );
	}

	CStockValuesComboBoxButton::~CStockValuesComboBoxButton()
	{
		// Note: after this button get deleted, the editor host interface pointer stored in m_pStockTags will be dangling.
		//	That's ok as long as the destination button updates the editor host interface pointer to itself - via SetTags().
	}

	void CStockValuesComboBoxButton::SetTags( const ui::IStockTags* pStockTags )
	{
		m_pStockTags = pStockTags;
		mfc::CToolbarButtonsRefBinder::Instance()->RegisterPointer( m_nID, 0, m_pStockTags );

		if ( m_pStockTags != nullptr )
		{
			std::vector<std::tstring> tags;
			m_pStockTags->QueryStockTags( tags );
			mfc::WriteComboItems( this, tags );
		}
		else
			RemoveAllItems();
	}

	bool CStockValuesComboBoxButton::OutputTag( const std::tstring& tag )
	{
		std::pair<bool, ui::ComboField> result = mfc::SetComboEditText( this, tag );

		if ( HasFocus() && GetHwnd() != nullptr )
			m_pWndCombo->SetEditSel( 0, -1 );

		return result.first;		// true if changed
	}

	void CStockValuesComboBoxButton::OnInputError( void ) const
	{
		// give parent a chance to restore previous valid value
		if ( m_pWndParent->GetSafeHwnd() != nullptr )
			ui::SendCommand( m_pWndParent->GetOwner()->GetSafeHwnd(), m_nID, ui::CN_INPUTERROR, GetComboBox()->GetSafeHwnd() );
	}

	void CStockValuesComboBoxButton::OnChangeParentWnd( CWnd* pParentWnd )
	{
		__super::OnChangeParentWnd( pParentWnd );

		if ( nullptr == m_pStockTags )		// parent toolbar is loading state (de-serializing)?
			mfc::CToolbarButtonsRefBinder::Instance()->RebindPointer( m_pStockTags, m_nID, 0 );

		if ( m_pWndCombo->GetSafeHwnd() != nullptr )
			CComboDropList::MakeSubclass( m_pWndCombo, true, true );		// use LB_FINDSTRINGEXACT to avoid partial matches (auto-delete)
	}

	void CStockValuesComboBoxButton::CopyFrom( const CMFCToolBarButton& src )
	{
		__super::CopyFrom( src );

		const CStockValuesComboBoxButton& srcButton = (const CStockValuesComboBoxButton&)src;

		SetTags( srcButton.m_pStockTags );		// will store the editor host pointer to this
		ASSERT_PTR( m_pStockTags );
	}
}


namespace ui
{
	// CSliderCtrl utils

	bool IsValidPos( const CSliderCtrl* pSliderCtrl, int newPos )
	{
		ASSERT_PTR( pSliderCtrl );
		Range<int> limits;

		pSliderCtrl->GetRange( limits.m_start, limits.m_end );
		return limits.Contains( newPos );
	}

	bool ValidatePos( const CSliderCtrl* pSliderCtrl, OUT int* pNewPos )
	{
		ASSERT_PTR( pSliderCtrl );
		Range<int> limits;

		pSliderCtrl->GetRange( limits.m_start, limits.m_end );
		limits.Constrain( *pNewPos );

		return limits.Contains( *pNewPos ) && *pNewPos != pSliderCtrl->GetPos();
	}
}


namespace mfc
{
	class CCustomSliderCtrl : public CSliderCtrl
	{
	public:
		CCustomSliderCtrl( CSliderButton* pButton );
		virtual ~CCustomSliderCtrl();

		bool IsVisible( void ) const { return ui::IsVisible( m_hWnd ); }
		bool UpdatePos( int pos );
	private:
		CSliderButton* m_pButton;
	protected:
		afx_msg void OnMouseMove( UINT flags, CPoint point );
		afx_msg void OnLButtonUp( UINT flags, CPoint point );
		afx_msg void OnKeyDown( UINT chr, UINT repCnt, UINT flags );

		DECLARE_MESSAGE_MAP()
	};


	IMPLEMENT_SERIAL( CSliderButton, CMFCToolBarButton, 1 )

	CSliderButton::CSliderButton( void )
		: CMFCToolBarButton()
		, m_width( DefaultWidth )
		, m_dwStyle( WS_CHILD | WS_VISIBLE | DefaultStyle )
		, m_limits( 0, 0 )
		, m_pos( 0 )
	{
		m_pSliderCtrl.reset( new CCustomSliderCtrl( this ) );
	}

	CSliderButton::CSliderButton( UINT btnId, int width, DWORD dwStyle /*= DefaultStyle*/ )
		: CMFCToolBarButton( btnId, mfc::FindButtonImageIndex( btnId ) )
		, m_width( 0 == width ? DefaultWidth : width )
		, m_dwStyle( dwStyle | WS_CHILD | WS_VISIBLE | DefaultStyle )
		, m_limits( 0, 0 )
		, m_pos( 0 )
	{
		m_pSliderCtrl.reset( new CCustomSliderCtrl( this ) );
	}

	CSliderButton::~CSliderButton()
	{
		if ( m_pSliderCtrl->GetSafeHwnd() != nullptr )
			m_pSliderCtrl->DestroyWindow();
	}

	CSliderCtrl* CSliderButton::GetSliderCtrl( void ) const
	{
		return m_pSliderCtrl.get();
	}

	void CSliderButton::SetRange( int minValue, int maxValue )
	{
		SetLimits( Range<int>( minValue, maxValue ) );

		mfc::ForEach_MatchingButton<mfc::CSliderButton>( m_nID, func::MakeSetter<const Range<int>&>( &mfc::CSliderButton::SetLimits, m_limits ), this );
	}

	bool CSliderButton::SetCountRange( size_t count, size_t tickFreqThresholdCount /*= TickFreqThresholdCount*/ )
	{
		Range<int> limits( 0, utl::max( 2, count - 1 ) );

		if ( m_limits == limits )
			return false;		// no change

		SetRange( limits.m_start, limits.m_end );

		if ( GetHwnd() != nullptr )
			m_pSliderCtrl->SetTicFreq( static_cast<int>( 1 + count / tickFreqThresholdCount ) );

		return true;
	}

	void CSliderButton::SetLimits( const Range<int>& limits )
	{
		m_limits = limits;

		if ( m_pSliderCtrl->GetSafeHwnd() != nullptr )
			m_pSliderCtrl->SetRange( m_limits.m_start, m_limits.m_end, true );
	}

	int CSliderButton::GetPos( void ) const
	{
		if ( m_pSliderCtrl->GetSafeHwnd() != nullptr )
			ASSERT( m_pSliderCtrl->GetPos() == m_pos );		// current pos consistent with the slider ctrl?

		return m_pos;
	}

	bool CSliderButton::SetPos( int pos, bool notify, bool syncAll /*= true*/ )
	{
		bool changed = pos != m_pos;

		m_pos = pos;

		if ( GetHwnd() != nullptr )
			if ( m_pSliderCtrl->UpdatePos( m_pos ) )
				changed = true;

		if ( syncAll )
			// sync other buttons on all toolbars with the same btnId
			mfc::ForEach_MatchingButton<mfc::CSliderButton>( m_nID, func::MakeSetter( &mfc::CSliderButton::SetPos, m_pos, false, false ), this );

		if ( notify && m_pSliderCtrl->GetSafeHwnd() != nullptr )
			ui::SendCommand( m_pSliderCtrl->GetOwner()->GetSafeHwnd(), m_nID, BN_CLICKED, m_pSliderCtrl->GetSafeHwnd() );

		return changed;
	}

	void CSliderButton::CopyFrom( const CMFCToolBarButton& s )
	{
		CMFCToolBarButton::CopyFrom( s );

		const CSliderButton& src = (const CSliderButton&)s;

		m_width = src.m_width;
		m_dwStyle = src.m_dwStyle;
		m_limits = src.m_limits;
		m_pos = src.m_pos;
	}

	void CSliderButton::Serialize( CArchive& archive )
	{
		__super::Serialize( archive );

		if ( archive.IsLoading() )
		{
			archive >> m_width;
			archive >> m_dwStyle;

			Range<int> limits;
			int pos;

			archive >> limits;
			archive >> pos;

			SetLimits( limits );
			SetPos( pos, false );
		}
		else
		{
			archive << m_width;
			archive << m_dwStyle;
			archive << m_limits;
			archive << m_pos;
		}
	}

	void CSliderButton::OnChangeParentWnd( CWnd* pParentWnd )
	{
		__super::OnChangeParentWnd( pParentWnd );

		if ( m_pSliderCtrl->GetSafeHwnd() != nullptr )
		{
			CWnd* pOldParentWnd = m_pSliderCtrl->GetParent();
			ASSERT_PTR( pOldParentWnd );

			if ( pParentWnd != nullptr && pOldParentWnd->GetSafeHwnd() == pParentWnd->GetSafeHwnd() )
				return;

			m_pSliderCtrl->DestroyWindow();
		}

		if ( pParentWnd->GetSafeHwnd() != nullptr )
			if ( m_pSliderCtrl->Create( m_dwStyle, m_rect, pParentWnd, m_nID ) )
			{
				m_pSliderCtrl->SetRange( m_limits.m_start, m_limits.m_end );
				m_pSliderCtrl->SetPos( m_pos );
				m_pSliderCtrl->SetOwner( pParentWnd->GetTopLevelFrame() );

				//m_pSliderCtrl->SetLineSize( 1 );
				//m_pSliderCtrl->SetPageSize( 10 );
			}
			else
				ASSERT( false );
	}

	SIZE CSliderButton::OnCalculateSize( CDC* pDC, const CSize& sizeDefault, BOOL horz )
	{
		pDC, sizeDefault;

		if ( !IsVisible() )
		{
			if ( m_pSliderCtrl->GetSafeHwnd() != nullptr )
				m_pSliderCtrl->ShowWindow( SW_HIDE );

			return CSize( 0, 0 );
		}

		if ( horz )
		{
			if ( m_pSliderCtrl->GetSafeHwnd() != nullptr && !IsHidden() )
			{
				m_pSliderCtrl->ShowWindow( SW_SHOWNOACTIVATE );
				m_pSliderCtrl->ModifyStyle( TBS_VERT, TBS_HORZ );
			}

			return CSize( m_width, DefaultHeight );
		}
		else
		{
			if ( m_pSliderCtrl->GetSafeHwnd() != nullptr && !IsHidden() )
			{
				m_pSliderCtrl->ShowWindow( SW_SHOWNOACTIVATE );
				m_pSliderCtrl->ModifyStyle( TBS_HORZ, TBS_VERT );
			}

			return CSize( DefaultHeight, m_width );
		}
	}

	void CSliderButton::OnMove( void )
	{
		if ( m_pSliderCtrl->GetSafeHwnd() != nullptr && m_pSliderCtrl->IsVisible() )
			m_pSliderCtrl->SetWindowPos( nullptr, m_rect.left + 1, m_rect.top + 1, m_rect.Width() - 2, m_rect.Height() - 2, SWP_NOZORDER | SWP_NOACTIVATE );
	}

	void CSliderButton::OnSize( int width )
	{
		m_width = width;
		m_rect.right = m_rect.left + m_width;

		if ( m_pSliderCtrl->GetSafeHwnd() != nullptr && m_pSliderCtrl->IsVisible() )
			m_pSliderCtrl->SetWindowPos( nullptr, m_rect.left + 1, m_rect.top + 1, m_rect.Width() - 2, m_rect.Height() - 2, SWP_NOZORDER | SWP_NOACTIVATE );
	}

	void CSliderButton::OnShow( BOOL show )
	{
		if ( m_pSliderCtrl->GetSafeHwnd() != nullptr )
			if ( show )
			{
				m_pSliderCtrl->ShowWindow( SW_SHOWNOACTIVATE );
				OnMove();
			}
			else
				m_pSliderCtrl->ShowWindow( SW_HIDE );
	}

	void CSliderButton::OnDraw( CDC* /*pDC*/, const CRect& /*rect*/, CMFCToolBarImages* /*pImages*/, BOOL /*horz = TRUE*/,
								BOOL /*bCustomizeMode = FALSE*/, BOOL /*bHighlight = FALSE*/, BOOL /*bDrawBorder = TRUE*/,
								BOOL /*bGrayDisabledButtons = TRUE*/ )
	{
	}


	// CCustomSliderCtrl implementation

	CCustomSliderCtrl::CCustomSliderCtrl( CSliderButton* pButton )
		: CSliderCtrl()
		, m_pButton( pButton )
	{
		ASSERT_PTR( m_pButton );
	}

	CCustomSliderCtrl::~CCustomSliderCtrl()
	{
	}

	bool CCustomSliderCtrl::UpdatePos( int pos )
	{
		int oldPos = GetPos();
		if ( oldPos == pos )
			return false;

		SetPos( pos );
		return true;
	}


	// message handlers

	BEGIN_MESSAGE_MAP( CCustomSliderCtrl, CSliderCtrl )
		ON_WM_MOUSEMOVE()
		ON_WM_KEYDOWN()
		ON_WM_LBUTTONUP()
	END_MESSAGE_MAP()

	void CCustomSliderCtrl::OnMouseMove( UINT flags, CPoint point )
	{
		int pos = GetPos();

		__super::OnMouseMove( flags, point );

		if ( HasFlag( flags, MK_LBUTTON ) && pos != GetPos() )
			m_pButton->SetPos( GetPos(), true );
	}

	void CCustomSliderCtrl::OnLButtonUp( UINT flags, CPoint point )
	{
		__super::OnLButtonUp( flags, point );

		if ( IsWindowVisible() )
			m_pButton->SetPos( GetPos(), true );
	}

	void CCustomSliderCtrl::OnKeyDown( UINT chr, UINT repCnt, UINT flags )
	{
		int pos = GetPos();

		__super::OnKeyDown( chr, repCnt, flags );

		if ( GetPos() != pos )
			m_pButton->SetPos( GetPos(), true );
	}
}
