#ifndef Control_fwd_h
#define Control_fwd_h
#pragma once

#include "ui_fwd.h"


namespace ui
{
	// implemented by a control to receive commands from a buddy control (e.g. a buddy button)
	//
	interface IBuddyCommandHandler
	{
		virtual bool OnBuddyCommand( UINT cmdId ) = 0;		// returns true if handled
	};


	struct CBuddyLayout
	{
		CBuddyLayout( void ) : m_alignment( NoAlign ), m_spacing( 0, 0 ) {}
		explicit CBuddyLayout( TAlignment alignment, const CSize& spacing ) : m_alignment( alignment ), m_spacing( spacing ) {}
		explicit CBuddyLayout( TAlignment alignment, int spacing ) : m_alignment( alignment ), m_spacing( spacing, spacing ) {}

		CRect& Align( CRect& rCtrlRect, const RECT& buddyAnchor ) const;		// tile align to buddy: layout the rect outside the anchor (by the anchor)
		CRect& ShrinkBuddyRect( CRect& rBuddyRect, const CSize& anchorCtrlSize ) const;

		CRect LayoutCtrl( CWnd* pCtrl, const CWnd* pBuddyCtrl, const CSize* pCustomSize = NULL ) const;		// tile move control outside of buddy anchor
		void ShrinkBuddy( CWnd* pBuddyCtrl, const CWnd* pCtrl ) const;										// shrink buddy control from the outer control
	public:
		TAlignment m_alignment;
		CSize m_spacing;

		enum Metrics { Spacing = 2 };

		static const CBuddyLayout s_tileToRight;
	};
}


#endif // Control_fwd_h
