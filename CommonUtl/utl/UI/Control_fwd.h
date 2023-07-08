#ifndef Control_fwd_h
#define Control_fwd_h
#pragma once

#include "ui_fwd.h"


namespace ui
{
	// implemented by a control to receive commands from a buddy host control (e.g. a buddy button)
	//
	interface IBuddyCommandHandler
	{
		virtual bool OnBuddyCommand( UINT cmdId ) = 0;		// returns true if handled
	};


	enum TandemAlign		// works in combination with enum ::Alignment values
	{
		H_ShrinkHost	= BIT_FLAG( 16 ),
		V_ShrinkHost	= BIT_FLAG( 17 ),

		H_TileMate		= BIT_FLAG( 18 ),
		V_TileMate		= BIT_FLAG( 19 ),

			HostShrinkMask	= H_ShrinkHost | V_ShrinkHost,
			TileMateMask	= H_TileMate | V_TileMate,

			// predefined alignments
			EditShinkHost_MateOnRight = H_AlignRight | V_AlignCenter | ui::H_ShrinkHost,	// tile to right, vertical centre, shrink host horizontally (typical edit/combobox alignment)
			ListHost_TileMateOnTopRight = H_AlignRight | V_AlignTop | ui::V_TileMate		// tile to right, top, no host shrinking (typical list control alignment)
	};
	typedef int TTandemAlign;		// combined values of ::Alignment and ui::TandemAlign


	class CTandemLayout
	{
	public:
		CTandemLayout( void ) : m_tandemAlign( NoAlign ), m_spacing( 0, 0 ) { ENSURE( IsValidAlignment() ); }
		explicit CTandemLayout( TTandemAlign tandemAlign, const CSize& spacing ) : m_tandemAlign( tandemAlign ), m_spacing( spacing ) { ENSURE( IsValidAlignment() ); }
		explicit CTandemLayout( TTandemAlign tandemAlign, int spacing ) : m_tandemAlign( tandemAlign ), m_spacing( spacing, spacing ) { ENSURE( IsValidAlignment() ); }

		bool IsValidAlignment( void ) const;		// TTandemAlign flags leads to host not covering the mate?

		TTandemAlign GetTandemAlign( void ) const { return m_tandemAlign; }
		void SetTandemAlign( TTandemAlign tandemAlign ) { m_tandemAlign = tandemAlign; ENSURE( IsValidAlignment() ); }

		const CSize& GetSpacing( void ) const { return m_spacing; }
		CSize& RefSpacing( void ) { return m_spacing; }

		// operations
		void AlignTandem( CRect& rHostRect, CRect& rMateRect, const CSize* pMateCustomSize = nullptr ) const;	// align rects of both host and mate controls
		void AlignMate( CRect& rMateRect, const RECT& hostRect, bool inTandem ) const;							// align mate rect relative to the host rect

		void ShrinkHostRect( CRect& rHostRect, const CSize& mateSize ) const;

		CRect LayoutMate( CWnd* pMateCtrl, const CWnd* pHostCtrl ) const;										// move mate control relative to host anchor rect (host was repositioned)
		void LayoutTandem( CWnd* pHostCtrl, CWnd* pMateCtrl, const CSize* pMateCustomSize = nullptr ) const;
	private:
		TTandemAlign m_tandemAlign;
		CSize m_spacing;

		enum Metrics { Spacing = 2 };
	public:
		static const CTandemLayout s_mateOnRight;		// typical edit/combobox alignment
	};
}


#endif // Control_fwd_h
