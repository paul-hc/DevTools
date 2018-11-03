
#include "stdafx.h"
#include "ContentFitBase.h"
#include "LayoutEngine.h"
#include "ScopedGdi.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	CContentFitBase::CContentFitBase( CWnd* pCtrl, int contentFitFlags /*= 0*/ )
		: m_pCtrl( pCtrl )
		, m_contentFitFlags( contentFitFlags )
	{
		ASSERT_PTR( m_pCtrl );
	}

	void CContentFitBase::SetContentFitFlags( int contentFitFlags )
	{
		m_contentFitFlags = contentFitFlags;

		if ( m_pCtrl->m_hWnd != NULL && m_contentFitFlags != 0 )
			ResizeToFit( m_contentFitFlags );
	}

	bool CContentFitBase::ResizeToFit( int fitFlags /*= FitHorizontally*/ )
	{
		CSize idealSize = ComputeIdealSize();
		CRect ctrlRect = ui::GetControlRect( m_pCtrl->m_hWnd );

		CSize deltaSize = idealSize - ctrlRect.Size();

		if ( !HasFlag( fitFlags, FitHorizontally ) || ( deltaSize.cx < 0 && !HasFlag( fitFlags, FitAllowShrinking ) ) )
			deltaSize.cx = 0;

		if ( !HasFlag( fitFlags, FitVertically ) || ( deltaSize.cy < 0 && !HasFlag( fitFlags, FitAllowShrinking ) ) )
			deltaSize.cy = 0;

		if ( 0 == deltaSize.cx && 0 == deltaSize.cy )
			return false;

		CSize layDeltaOrigin( 0, 0 ), layDeltaSize( 0, 0 );		// for layout adjustment

		DWORD style = m_pCtrl->GetStyle();

		if ( HasFlag( fitFlags, FitHorizontally ) && deltaSize.cx != 0 )
			switch ( style & SS_TYPEMASK )
			{
				default:
				case SS_LEFT:
					ctrlRect.right += deltaSize.cx;
					layDeltaSize.cx -= deltaSize.cx;
					break;
				case SS_CENTER:
				{
					int halfDeltaWidth = deltaSize.cx / 2 + ( deltaSize.cx % 2 );		// +1 for odd fitting width

					ctrlRect.left -= halfDeltaWidth;
					ctrlRect.right += halfDeltaWidth;
					layDeltaOrigin.cx -= halfDeltaWidth;
					layDeltaSize.cx += halfDeltaWidth * 2;
					break;
				}
				case SS_RIGHT:
					ctrlRect.left += deltaSize.cx;
					layDeltaOrigin.cx += deltaSize.cx;
					layDeltaSize.cx -= deltaSize.cx;
					break;
			}

		if ( HasFlag( fitFlags, FitVertically ) && deltaSize.cy != 0 )
			if ( HasFlag( style, SS_CENTERIMAGE ) )
			{
				int halfDeltaHeight = deltaSize.cy / 2 + ( deltaSize.cy % 2 );		// +1 for odd fitting height

				ctrlRect.top -= halfDeltaHeight;
				ctrlRect.bottom += halfDeltaHeight;
				layDeltaOrigin.cy -= halfDeltaHeight;
				layDeltaSize.cy += halfDeltaHeight * 2;
			}
			else
			{
				ctrlRect.bottom += deltaSize.cy;
				layDeltaSize.cy += deltaSize.cy;
			}

		m_pCtrl->MoveWindow( &ctrlRect );

		if ( ui::ILayoutEngine* pParentLayout = dynamic_cast< ui::ILayoutEngine* >( m_pCtrl->GetParent() ) )
		{	// adjust layout for this control so that it will reposition as expected on parent dialog resize
			CLayoutEngine& rLayoutEngine = pParentLayout->GetLayoutEngine();
			UINT ctrlId = m_pCtrl->GetDlgCtrlID();

			if ( rLayoutEngine.HasControlState( ctrlId ) )
				rLayoutEngine.AdjustControlInitialPosition( ctrlId, layDeltaOrigin, layDeltaSize );
		}
		return true;
	}

	CSize CContentFitBase::ComputeIdealTextSize( void )
	{
		CRect textBounds;
		m_pCtrl->GetClientRect( &textBounds );

		std::tstring text = ui::GetWindowText( m_pCtrl );

		if ( !text.empty() )
		{
			CClientDC clientDC( m_pCtrl );
			CScopedGdi< CFont > scopedFont( &clientDC, m_pCtrl->GetFont() );

			ui::SetRectSize( textBounds, ui::GetTextSize( &clientDC, text.c_str(), GetDrawTextFlags() ) );
		}

		CSize idealSize = textBounds.Size() + ui::GetNonClientSize( m_pCtrl->m_hWnd );
		return idealSize;
	}

	UINT CContentFitBase::GetDrawTextFlags( void ) const
	{
		return 0;
	}

	void CContentFitBase::OnContentChanged( void )
	{
		if ( m_contentFitFlags != 0 )
			ResizeToFit( m_contentFitFlags );
	}
}
