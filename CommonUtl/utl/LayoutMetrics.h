#ifndef LayoutMetrics_h
#define LayoutMetrics_h
#pragma once


namespace layout { typedef int Style; }


struct CLayoutStyle
{
	UINT m_ctrlId;
	layout::Style m_layoutStyle;
};


struct CDualLayoutStyle
{
	UINT m_ctrlId;
	layout::Style m_expandedStyle;
	layout::Style m_collapsedStyle;
};


struct CLayoutPlacement
{
	CLayoutPlacement( bool useWindowPlacement )
		: m_useWindowPlacement( useWindowPlacement ), m_initialSize( -1, -1 ), m_pos( -1, -1 ), m_size( -1, -1 ), m_showCmd( -1 ), m_collapsed( false ) {}
public:
	const bool m_useWindowPlacement;	// for top level dialogs with maximize/minimize box; if true m_pos is in workspace coords, otherwise in screen coords
	CSize m_initialSize;				// the original size in dialog template
	CPoint m_pos;
	CSize m_size;
	int m_showCmd;
	bool m_collapsed;
};


class CLayoutEngine;


namespace ui
{
	// implemented by layout dialogs, form views, property pages, etc

	interface ILayoutEngine
	{
		virtual CLayoutEngine& GetLayoutEngine( void ) = 0;
		virtual void RegisterCtrlLayout( const CLayoutStyle layoutStyles[], unsigned int count ) = 0;
		virtual bool HasControlLayout( void ) const = 0;

		virtual void OnCollapseChanged( bool collapsed ) { collapsed; }
	};


	// implemented by controls that act as a layout frame, having custom internal layout rules

	interface ILayoutFrame
	{
		virtual void OnControlResized( UINT ctrlId ) = 0;
	};


	// implemented to customize command information (tooltips, message strings) by dialogs, frames, etc

	interface ICustomCmdInfo
	{
		virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
		{
			rText, cmdId, pTooltip;
		}
	};
}


namespace layout
{
	union Metrics
	{
		explicit Metrics( int layoutStyle = 0 );
		~Metrics( void );

		bool IsValid( void ) const;
		bool HasEffect( void ) const;
	public:
		enum BitDefs
		{
			PercentBitCount	= 7,
			MaskPercentage	= ( 1 << PercentBitCount ) - 1,

			ShiftOffsetX	= 0,
			ShiftOffsetY	= PercentBitCount,
			ShiftStretchX	= PercentBitCount * 2,
			ShiftStretchY	= PercentBitCount * 3,

			MaskOffsetX		= MaskPercentage << ShiftOffsetX,
			MaskOffsetY		= MaskPercentage << ShiftOffsetY,
			MaskStretchX	= MaskPercentage << ShiftStretchX,
			MaskStretchY	= MaskPercentage << ShiftStretchY,

			DoRepaint		= 1 << ( 4 * PercentBitCount ),
			CollapsedLeft	= DoRepaint << 1,
			CollapsedTop	= CollapsedLeft << 1,
		};
	public:
		struct Fields
		{
			unsigned int m_offsetX : PercentBitCount;		// percent horizontal offset (0 -> 100)
			unsigned int m_offsetY : PercentBitCount;		// percent vertical offset (0 -> 100)
			unsigned int m_stretchX : PercentBitCount;		// percent horizontal stretch (0 -> 100)
			unsigned int m_stretchY : PercentBitCount;		// percent vertical stretch (0 -> 100)
			unsigned int m_doRepaint : 1;					// control must be repaint when resized
			unsigned int m_collapsedLeft : 1;				// control is a left edge collapsed marker
			unsigned int m_collapsedTop : 1;				// control is a top edge collapsed marker
		};
	public:
		Style m_layoutStyle;
		Fields m_fields;
	};


	enum PredefinedStyle
	{
		None		= 0,

		// offset & stretch 100%
		OffsetX		= 100 << Metrics::ShiftOffsetX,
		OffsetY		= 100 << Metrics::ShiftOffsetY,
		StretchX	= 100 << Metrics::ShiftStretchX,
		StretchY	= 100 << Metrics::ShiftStretchY,

		Offset		= OffsetX | OffsetY,
		Stretch		= StretchX | StretchY,

		DoRepaint	= Metrics::DoRepaint,			// repaint on resize

		CollapsedLeft = Metrics::CollapsedLeft,
		CollapsedTop = Metrics::CollapsedTop,
		CollapsedMask = CollapsedLeft | CollapsedTop,

		UseExpanded = -1						// for CDualLayoutStyle: copy m_expandedStyle for m_collapsedStyle
	};


	// percentage layout functions

	inline int offsetX( int byPercentage )
	{
		ASSERT( byPercentage >= 0 && byPercentage <= 100 );
		return byPercentage << Metrics::ShiftOffsetX;
	}

	inline int offsetY( int byPercentage )
	{
		ASSERT( byPercentage >= 0 && byPercentage <= 100 );
		return byPercentage << Metrics::ShiftOffsetY;
	}

	inline int stretchX( int byPercentage )
	{
		ASSERT( byPercentage >= 0 && byPercentage <= 100 );
		return byPercentage << Metrics::ShiftStretchX;
	}

	inline int stretchY( int byPercentage )
	{
		ASSERT( byPercentage >= 0 && byPercentage <= 100 );
		return byPercentage << Metrics::ShiftStretchY;
	}


	class CControlState
	{
	public:
		CControlState( const Metrics& metrics = Metrics( 0 ) );
		~CControlState();

		bool HasCollapsedState( void ) const { return m_collapsedMetrics.IsValid(); }
		const Metrics& GetMetrics( bool collapsed ) const { return collapsed && HasCollapsedState() ? m_collapsedMetrics : m_metrics; }

		layout::Style GetLayoutStyle( bool collapsed ) const { return GetMetrics( collapsed ).m_layoutStyle; }
		void SetLayoutStyle( int layoutStyle, bool collapsed );
		void ModifyLayoutStyle( int clearStyle, int setStyle );

		bool IsCtrlInit( void ) const { return m_hControl != NULL; };
		void InitCtrl( HWND hControl );
		void ResetCtrl( void ) { InitCtrl( NULL ); }

		bool ComputeLayout( CRect& rCtrlRect, UINT& rSwpFlags, const CSize& delta, bool collapsed ) const;
		bool RepositionCtrl( const CSize& delta, bool collapsed ) const;
	private:
		Metrics m_metrics, m_collapsedMetrics;		// self-encapsulated
	public:
		HWND m_hControl;
		HWND m_hParent;
	private:
		CPoint m_initialOrigin;
		CSize m_initialSize;
	};


	// bottom-right resize box in dialogs (themed)

	class CResizeGripper
	{
	public:
		CResizeGripper( CWnd* pDialog, const CSize& offset = CSize( 1, 0 ) );

		const CRect& GetRect( void ) const { return m_gripperRect; }

		void Layout( void );
		void Redraw( void );
		void Draw( HDC hDC );
	private:
		CWnd* m_pDialog;
		CSize m_offset;
		CRect m_gripperRect;			// in dialog client coords
	};
}


#endif // LayoutMetrics_h
