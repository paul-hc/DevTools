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


	struct CTandemLayout
	{
		CTandemLayout( void ) : m_alignment( NoAlign ), m_spacing( 0, 0 ) {}
		explicit CTandemLayout( TAlignment alignment, const CSize& spacing ) : m_alignment( alignment ), m_spacing( spacing ) {}
		explicit CTandemLayout( TAlignment alignment, int spacing ) : m_alignment( alignment ), m_spacing( spacing, spacing ) {}

		void AlignTandem( CRect& rHostRect, CRect& rMateRect, const CSize* pMateCustomSize = NULL ) const;	// align the mate control and shrink the host control
		void AlignOutside( CRect& rMateRect, const RECT& hostRect ) const;									// tile align to host: layout the rect by the host

		void ShrinkHostRect( CRect& rHostRect, const CSize& mateSize ) const;

		CRect LayoutMate( CWnd* pMateCtrl, const CWnd* pHostCtrl, const CSize* pCustomSize = NULL ) const;	// tile move control outside of host anchor
		void LayoutTandem( CWnd* pHostCtrl, CWnd* pMateCtrl, const CSize* pMateCustomSize = NULL ) const;
	public:
		TAlignment m_alignment;
		CSize m_spacing;

		enum Metrics { Spacing = 2 };

		static const CTandemLayout s_mateOnRight;
	};
}


#endif // Control_fwd_h
