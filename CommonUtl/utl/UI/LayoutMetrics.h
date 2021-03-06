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
		: m_useWindowPlacement( useWindowPlacement )
		, m_initialSize( -1, -1 )
		, m_pos( -1, -1 )
		, m_size( -1, -1 )
		, m_showCmd( -1 )
		, m_collapsed( false )
	{
	}
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

			Shift_MoveX		= 0,
			Shift_MoveY		= PercentBitCount,
			Shift_SizeX		= PercentBitCount * 2,
			Shift_SizeY		= PercentBitCount * 3,

			Mask_MoveX		= MaskPercentage << Shift_MoveX,
			Mask_MoveY		= MaskPercentage << Shift_MoveY,
			Mask_SizeX		= MaskPercentage << Shift_SizeX,
			Mask_SizeY		= MaskPercentage << Shift_SizeY,

			DoRepaint		= 1 << ( 4 * PercentBitCount ),
			CollapsedLeft	= DoRepaint << 1,
			CollapsedTop	= CollapsedLeft << 1,
		};
	public:
		struct Fields
		{
			unsigned int m_moveX : PercentBitCount;			// percent horizontal move (0 -> 100)
			unsigned int m_moveY : PercentBitCount;			// percent vertical move (0 -> 100)
			unsigned int m_sizeX : PercentBitCount;			// percent horizontal size (0 -> 100)
			unsigned int m_sizeY : PercentBitCount;			// percent vertical size (0 -> 100)
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

		// move & size 100%
		MoveX		= 100 << Metrics::Shift_MoveX,
		MoveY		= 100 << Metrics::Shift_MoveY,
		SizeX		= 100 << Metrics::Shift_SizeX,
		SizeY		= 100 << Metrics::Shift_SizeY,

		Move		= MoveX | MoveY,
		Size		= SizeX | SizeY,

		DoRepaint	= Metrics::DoRepaint,		// repaint on resize

		CollapsedLeft = Metrics::CollapsedLeft,
		CollapsedTop = Metrics::CollapsedTop,
		CollapsedMask = CollapsedLeft | CollapsedTop,

		UseExpanded = -1						// for CDualLayoutStyle: copy m_expandedStyle for m_collapsedStyle
	};


	// percentage layout functions

	inline int pctMoveX( int byPercentage )
	{
		ASSERT( byPercentage >= 0 && byPercentage <= 100 );
		return byPercentage << Metrics::Shift_MoveX;
	}

	inline int pctMoveY( int byPercentage )
	{
		ASSERT( byPercentage >= 0 && byPercentage <= 100 );
		return byPercentage << Metrics::Shift_MoveY;
	}

	inline int pctSizeX( int byPercentage )
	{
		ASSERT( byPercentage >= 0 && byPercentage <= 100 );
		return byPercentage << Metrics::Shift_SizeX;
	}

	inline int pctSizeY( int byPercentage )
	{
		ASSERT( byPercentage >= 0 && byPercentage <= 100 );
		return byPercentage << Metrics::Shift_SizeY;
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

		// advanced control layout (use with care)
		void AdjustInitialPosition( const CSize& deltaOrigin, const CSize& deltaSize );			// when stretching content to fit: to retain original layout behaviour
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
