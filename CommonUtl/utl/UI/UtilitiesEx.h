#ifndef UtilitiesEx_h
#define UtilitiesEx_h
#pragma once

#include "Utilities.h"
#include "WindowHook.h"


namespace ui
{
	void DrawBorder( CDC* pDC, const CRect& rect, COLORREF borderColor, int borderWidth = 1 );
	int MakeBorderRegion( CRgn* pBorderRegion, const CRect& rect, int borderWidth = 1 );
}


interface INonClientRender
{
	virtual void DrawNonClient( CDC* pDC, const CRect& ctrlRect ) = 0;		// ctrlRect in parent's coords
};


class CNonClientDraw : public CWindowHook
{
public:
	CNonClientDraw( CWnd* pWnd, INonClientRender* pCallback = NULL );
	~CNonClientDraw();

	void RedrawNonClient( void )
	{
		m_pWnd->RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_FRAME );
	}

	void ResizeNonClient( void )
	{
		m_pWnd->SetWindowPos( NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER );
	}
protected:
	virtual LRESULT WindowProc( UINT message, WPARAM wParam, LPARAM lParam );
private:
	CWnd* m_pWnd;
	INonClientRender* m_pCallback;
};


class CScopedWindowBorder;


struct CScopedLockRedraw
{
	CScopedLockRedraw( CWnd* pWnd, CScopedWindowBorder* pBorder = NULL, bool doRedraw = true );
	~CScopedLockRedraw();
private:
	CWnd* m_pWnd;
	bool m_doRedraw;
	std::auto_ptr< CScopedWindowBorder > m_pBorder;		// temporary border; used for long operations to signal "busy"
};


struct CScopedClientBorder
{
	CScopedClientBorder( CWnd* pWnd, COLORREF borderColor, int borderWidth = 1 )
		: m_pWnd( pWnd )
	{
		CClientDC dc( m_pWnd );
		CRect clientRect;
		m_pWnd->GetClientRect( &clientRect );
		ui::DrawBorder( &dc, clientRect, borderColor, borderWidth );
	}

	~CScopedClientBorder()
	{
		m_pWnd->Invalidate( FALSE );
	}
private:
	CWnd* m_pWnd;
};


class CScopedWindowBorder : private INonClientRender
{
public:
	CScopedWindowBorder( CWnd* pWnd, COLORREF borderColor, int borderWidth = 1 );
	~CScopedWindowBorder();

	void Release( void );
private:
	// INonClientRender interface
	virtual void DrawNonClient( CDC* pDC, const CRect& ctrlRect );

	void RedrawBorder( void );
private:
	CWnd* m_pWnd;
	COLORREF m_borderColor;
	int m_borderWidth;
	CNonClientDraw m_drawHook;
};


#include "WindowTimer.h"


class CFlashCtrlFrame : public CTimerSequenceHook, private ISequenceTimerCallback
{
public:
	CFlashCtrlFrame( HWND hCtrl, COLORREF frameColor, unsigned int count = 2, int elapse = 300 )
		: CTimerSequenceHook( hCtrl, this, EventId, count * 2, elapse )
		, m_frameBrush( frameColor )
		, m_frameOn( false )
	{
		ASSERT( ui::IsChild( hCtrl ) );
	}
private:
	enum { EventId = 7777 };

	// ISequenceTimerCallback interface
	virtual void OnSequenceStep( void );
private:
	enum Metrics { Spacing = 1, FrameWidth = 2 };
	CBrush m_frameBrush;
	bool m_frameOn;
};


namespace ui
{
	inline void FlashCtrlFrame( HWND hCtrl, COLORREF frameColor = color::Error, unsigned int count = 2, int elapse = 300 )
	{
		new CFlashCtrlFrame( hCtrl, frameColor, count, elapse );
	}

	inline void FlashCtrlFrame( CWnd* pCtrl, COLORREF frameColor = color::Error, unsigned int count = 2, int elapse = 300 )
	{
		FlashCtrlFrame( pCtrl->GetSafeHwnd(), frameColor, count, elapse );
	}
}


class CScopedPumpMessage
{
public:
	CScopedPumpMessage( size_t pumpFreq, CWnd* pDisableWnd = NULL )
		: m_pumpFreq( pumpFreq )
		, m_count( 0 )
		, m_pDisableWnd( pDisableWnd )
		, m_oldEnabled( true )
	{
		if ( m_pDisableWnd != NULL )
		{
			m_oldEnabled = !ui::IsDisabled( m_pDisableWnd->GetSafeHwnd() );
			ui::EnableWindow( m_pDisableWnd->GetSafeHwnd(), false );					// disable input, but allow tastbar activation, paint, etc
		}
	}

	~CScopedPumpMessage()
	{
		if ( m_pDisableWnd != NULL && ::IsWindow( m_pDisableWnd->GetSafeHwnd() ) )
			ui::EnableWindow( m_pDisableWnd->GetSafeHwnd(), m_oldEnabled );				// restore original enabling state
	}

	void CheckPump( void )
	{
		if ( 0 == m_pumpFreq || !( ++m_count % m_pumpFreq ) )
			ui::PumpPendingMessages();
	}
private:
	size_t m_pumpFreq;					// pumps less frequently, 0 for each time
	size_t m_count;
	CWnd* m_pDisableWnd;				// if supplied, gets disabled during the long processing, but allows taskbar activation and paint
	bool m_oldEnabled;
};


struct CScopedDisableDropTarget
{
	CScopedDisableDropTarget( CWnd* pWnd )
		: m_pWnd( HasFlag( ui::GetStyleEx( pWnd->GetSafeHwnd() ), WS_EX_ACCEPTFILES ) ? pWnd : NULL )
	{
		if ( m_pWnd != NULL )
			m_pWnd->DragAcceptFiles( FALSE );
	}
	~CScopedDisableDropTarget()
	{
		if ( m_pWnd != NULL )
			m_pWnd->DragAcceptFiles( TRUE );
	}
private:
	CWnd* m_pWnd;
};


#include "ScopedValue.h"


struct CScopedDisableBeep : private CScopedValue< bool >
{
	CScopedDisableBeep( void ) : CScopedValue< bool >( &ui::RefAsyncApiEnabled(), false ) {}
};


struct CScopedDrawText
{
	CScopedDrawText( CDC* pDC, const CWnd* pCtrl, CFont* pFont, COLORREF textColor = color::Auto, COLORREF bkColor = CLR_NONE )
		: m_pDC( pDC )
		, m_pOldFont( m_pDC->SelectObject( pFont != NULL ? pFont : pCtrl->GetFont() ) )
		, m_oldTextColor( m_pDC->SetTextColor( textColor != color::Auto ? textColor : GetSysColor( pCtrl->IsWindowEnabled() ? COLOR_BTNTEXT : COLOR_GRAYTEXT ) ) )
		, m_oldBkColor( bkColor != CLR_NONE ? m_pDC->SetBkColor( bkColor ) : bkColor )
		, m_oldBkMode( m_pDC->SetBkMode( CLR_NONE == bkColor ? TRANSPARENT : OPAQUE ) )
	{
	}

	~CScopedDrawText()
	{
		m_pDC->SelectObject( m_pOldFont );
		m_pDC->SetTextColor( m_oldTextColor );
		m_pDC->SetBkMode( m_oldBkMode );
		if ( m_oldBkColor != CLR_NONE )
			m_pDC->SetBkColor( m_oldBkColor );
	}
private:
	CDC* m_pDC;
	CFont* m_pOldFont;
	COLORREF m_oldTextColor;
	COLORREF m_oldBkColor;
	int m_oldBkMode;
};


namespace ui
{
	template< typename Value >
	void DDX_HexValue( CDataExchange* pDX, int ctrlId, Value& rValue, const TCHAR* pFormat = _T("0x%08X"), const Value* pNullValue = NULL )
	{
		if ( !pDX->m_bSaveAndValidate )
		{
			if ( NULL == pNullValue || rValue != *pNullValue )
				ddx::SetItemText( pDX, ctrlId, str::Format( pFormat, rValue ) );
			else
				ddx::SetItemText( pDX, ctrlId, std::tstring() );
		}
		else
		{
			std::tstring text = ddx::GetItemText( pDX, ctrlId );

			if ( text.empty() )
				if ( pNullValue != NULL )
					rValue = *pNullValue;
				else
				{
					pDX->m_idLastControl = IDC_HANDLE_EDIT;
					pDX->Fail();
				}
			else if ( !num::ParseHexNumber< Value >( rValue, text ) )
			{
				pDX->m_idLastControl = IDC_HANDLE_EDIT;
				pDX->Fail();
			}
		}
	}

	template< typename Handle >
	inline void DDX_Handle( CDataExchange* pDX, int ctrlId, Handle& rHandle, const Handle* pNullHandle = NULL )
	{
		typedef DWORD_PTR HandleValue;
		DDX_HexValue< HandleValue >( pDX, ctrlId, (HandleValue&)rHandle, _T("%08X"), (const HandleValue*)pNullHandle );
	}
}


namespace app
{
	template< typename Type >
	bool GetProfileVector( std::vector< Type >& rOutVector, const TCHAR* pSection, const TCHAR* pEntry )
	{
		BYTE* pData = NULL;
		UINT byteCount;
		if ( !AfxGetApp()->GetProfileBinary( pSection, pEntry, &pData, &byteCount ) )
			return false;

		ASSERT( 0 == ( byteCount % sizeof( Type ) ) );		// multiple of Type size

		typedef const Type* const_iterator;
		const size_t savedCount = byteCount / sizeof( Type );
		const_iterator pStart = reinterpret_cast< const_iterator >( pData ), pEnd = pStart + savedCount;
		rOutVector.assign( pStart, pEnd );

		delete[] pData;
		return savedCount == rOutVector.size();
	}

	template< typename Type >
	bool WriteProfileVector( const std::vector< Type >& rVector, const TCHAR* pSection, const TCHAR* pEntry )
	{
		BYTE* pData = !rVector.empty() ? (BYTE*)( &rVector.front() ) : NULL;
		UINT byteCount = static_cast< UINT >( rVector.size() * sizeof( Type ) );
		return AfxGetApp()->WriteProfileBinary( pSection, pEntry, pData, byteCount ) != FALSE;
	}
}


#endif // UtilitiesEx_h