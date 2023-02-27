#ifndef ui_fwd_h
#define ui_fwd_h
#pragma once


enum Alignment
{
	NoAlign = 0,

	H_AlignLeft		= 0x01,
	H_AlignCenter	= 0x02,
	H_AlignRight	= 0x03,

	V_AlignTop		= 0x10,
	V_AlignCenter	= 0x20,
	V_AlignBottom	= 0x30,

		HorizontalMask	= 0x0F,
		VerticalMask	= 0xF0
};

typedef int TAlignment;


typedef int TPercent;
typedef unsigned int TUPercent;
typedef int TValueOrPct;		// if negative is a percentage, otherwise a pozitive value

typedef double TFactor;			// from 0.0 to 1.0


namespace ui
{
	inline bool IsPercentage_0_100( TPercent percentage ) { return percentage >= 0 && percentage <= 100; }
	inline bool IsPercentage_m100_p100( TPercent percentage ) { return percentage >= -100 && percentage <= 100; }

	inline TPercent GetPercentageOf( int value, int maxValue ) { return MulDiv( 100, value, maxValue ); }
	inline int ScaleValue( int value, TPercent percentage ) { return MulDiv( value, percentage, 100 ); }
	inline int AdjustValueBy( int value, TPercent byPercentage ) { return ScaleValue( value, 100 + byPercentage ); }


	// factor utils

	inline bool IsPercentage( TValueOrPct valueOrNegativePct ) { return valueOrNegativePct < 0; }
	inline int EvalValueOrPercentage( TValueOrPct valueOrNegativePct, int extent ) { return IsPercentage( valueOrNegativePct ) ? MulDiv( extent, -valueOrNegativePct, 100 ) : valueOrNegativePct; }


	// factor utils

	inline bool IsFactor_0_1( TFactor factor ) { return factor >= 0.0 && factor <= 1.0; }
	inline bool IsFactor_m1_p1( TFactor factor ) { return factor >= -1.0 && factor <= 1.0; }
	inline TFactor PercentToFactor( TPercent pct ) { ASSERT( IsPercentage_m100_p100( pct ) ); return static_cast<double>( pct ) / 100.0; }
	inline TPercent FactorToPercent( TFactor factor ) { ASSERT( IsFactor_m1_p1( factor ) ); return static_cast<TPercent>( factor * 100.0 ); }

	template< typename ValueT >
	inline ValueT AdjustValueByFactor( ValueT value, TFactor factor ) { return static_cast<ValueT>( value * ( 1.0 + factor ) ); }

/*
	inline bool IsValidPercentage( TPercent percentage ) { return percentage >= 0 && percentage <= 100; }
	inline int ScaleValueBy( int value, TPercent byPercentage ) { return MulDiv( value, byPercentage, 100 ); }
	inline TPercent GetPercentageOf( int value, int maxValue ) { return MulDiv( 100, value, maxValue ); }

	inline bool IsPercentage( TValueOrPct valueOrNegativePct ) { return valueOrNegativePct < 0; }
	inline int EvalValueOrPercentage( TValueOrPct valueOrNegativePct, int extent ) { return IsPercentage( valueOrNegativePct ) ? MulDiv( extent, -valueOrNegativePct, 100 ) : valueOrNegativePct; }
*/

	// Encalpsulates a raw value that can be either a value or a percentage (of en extent).

	union CValuePct
	{
		CValuePct( void ) { m_valuePair.m_value = m_valuePair.m_percentage = SHRT_MAX; }
		explicit CValuePct( int rawValue ) : m_rawValue( rawValue ) {}

		static CValuePct MakeValue( int value ) { CValuePct valPct; valPct.SetValue( value ); return valPct; }
		static CValuePct MakePercentage( TPercent percentage ) { CValuePct valPct; valPct.SetPercentage( percentage ); return valPct; }

		bool IsValid( void ) const { return m_valuePair.IsValid(); }

		int GetRaw( void ) const { return m_rawValue; }
		void SetRaw( int rawValue ) { m_rawValue = rawValue; }

		bool HasValue( void ) const { return CPair::IsValidField( m_valuePair.m_value ); }
		bool HasPercentage( void ) const { return CPair::IsValidField( m_valuePair.m_percentage ); }

		int GetValue( void ) const { ASSERT( HasValue() ); return m_valuePair.m_value; }
		void SetValue( int value ) { m_valuePair.SetValue( value ); }

		TPercent GetPercentage( void ) const { ASSERT( HasPercentage() ); return m_valuePair.m_percentage; }
		void SetPercentage( TPercent percentage ) { m_valuePair.SetPercentage( percentage ); }

		int EvalValue( int extent ) const;
		double EvalValue( double extent ) const;
	private:
		struct CPair
		{
			bool IsValid( void ) const { return IsValidField( m_value ) || IsValidField( m_percentage ); }
			static bool IsValidField( short field ) { return field != SHRT_MAX; }

			void SetValue( int value ) { m_value = static_cast<short>( value ); m_percentage = SHRT_MAX; }
			void SetPercentage( TPercent percentage ) { m_percentage = static_cast<short>( percentage ); m_value = SHRT_MAX; }
		public:
			short m_value;
			short m_percentage;
		};
	private:
		int m_rawValue;
		CPair m_valuePair;
	};
}


namespace ui
{
	enum StretchMode { OriginalSize, StretchFit, ShrinkFit };
}


#define ON_CN_INPUTERROR( id, memberFxn )		ON_CONTROL( ui::CN_INPUTERROR, id, memberFxn )


namespace ui
{
	enum ComboField { BySel, ByEdit };

	enum PopupAlign { DropRight, DropDown, DropLeft, DropUp };
	enum { HistoryMaxSize = 20 };
	enum StdPopup { AppMainPopup, AppSysTray, AppDebugPopup, TextEditorPopup, HistoryComboPopup, ListView, DateTimePopup };		// IDR_STD_CONTEXT_MENU

	enum FontEffect { Regular = 0, Bold = 1 << 0, Italic = 1 << 1, Underline = 1 << 2 };
	typedef int TFontEffect;


	enum { MinCmdId = 0, MaxCmdId = 0xFFFF, MinAppCmdId = 1, MaxAppCmdId = 0x7FFF, AtEnd = -1 };

	inline int ToCmdId( UINT uCmdId ) { return (short)(unsigned short)( uCmdId ); }


	CPoint GetCursorPos( HWND hWnd = nullptr );			// return screen coordinates if NULL
	const CPoint& GetNullPos( void );

	// works for VK_CONTROL, VK_SHIFT, VK_MENU (alt), VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, etc
	inline bool IsKeyPressed( int virtKey ) { return ::GetKeyState( virtKey ) < 0; }


	// conversion to/from state (0-based index); raw state (UINT) uses is 1-based indexes - works for list control, tree control
	typedef UINT TRawCheckState;

	inline TRawCheckState CheckStateToRaw( int checkState ) { return INDEXTOSTATEIMAGEMASK( checkState + 1 ); }
	inline int CheckStateFromRaw( TRawCheckState rawCheckState )	{ return ( rawCheckState >> 12 ) - 1; }


	// standard notifications
	enum NotifCode { CN_INPUTERROR = CBN_SELENDCANCEL + 20 };		// note: notifications are suppressed during parent's UpdateData()


	struct CNmHdr : public tagNMHDR
	{
		CNmHdr( const CWnd* pCtrl, int notifyCode )
		{
			hwndFrom = pCtrl->GetSafeHwnd();
			idFrom = pCtrl->GetDlgCtrlID();
			code = notifyCode;
		}

		LRESULT NotifyParent( void );						// returns 0 if handler did not reject the action (0 means unhandled or success; 1 means reject)
	};


	COLORREF AlterColorSlightly( COLORREF bkColor );		// slightly modified background colour used as transparent colour for bitmap masks, so that themes that render with alpha blending don't show weird colours (such as radio button)
}


class CBalloonHostWnd;


namespace ui
{
	// baloon tips (modeless)
	void RequestCloseAllBalloons( void );		// close the balloon quickly

	CBalloonHostWnd* ShowBalloonTip( const TCHAR* pTitle, const std::tstring& message, HICON hToolIcon = TTI_NONE, const CPoint& screenPos = GetNullPos() );
	CBalloonHostWnd* ShowBalloonTip( const CWnd* pCtrl, const TCHAR* pTitle, const std::tstring& message, HICON hToolIcon = TTI_NONE );

	// show modeless balloon if available, otherwise modal ui::MessageBox
	CBalloonHostWnd* SafeShowBalloonTip( UINT mbStyle, const TCHAR* pTitle, const std::tstring& message, CWnd* pCtrl );
}


#endif // ui_fwd_h
