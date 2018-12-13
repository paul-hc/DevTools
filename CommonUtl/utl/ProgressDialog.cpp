
#include "stdafx.h"
#include "ProgressDialog.h"
#include "LayoutEngine.h"
#include "ClockStatic.h"
#include "RuntimeException.h"
#include "StringUtilities.h"
#include "UtilitiesEx.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDC_OPERATION_LABEL, SizeX },
		{ IDC_STAGE_STATIC, SizeX },
		{ IDC_ITEM_STATIC, SizeX },
		{ IDC_CLOCK_STATIC, MoveX },
		{ IDC_PROGRESS_BAR, SizeX },
		{ IDCANCEL, pctMoveX( 50 ) }
	};
}


CProgressDialog::CProgressDialog( const std::tstring& operationLabel, int optionFlags /*= MarqueeProgress*/ )
	: CLayoutDialog()
	, m_operationLabel( operationLabel )
	, m_optionFlags( optionFlags )
	, m_stageCount( 0 )
	, m_itemNo( 0 )
	, m_itemCount( 0 )
	, m_stageLabelStatic( CRegularStatic::Instruction )
	, m_itemLabelStatic( CRegularStatic::Instruction )
	, m_pClockStatic( new CClockStatic )
{
	m_regSection = _T("utl\\ProgressDialog");
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
	GetLayoutEngine().MaxClientSize().cy = 0;		// no vertical resize
}

CProgressDialog::~CProgressDialog()
{
	if ( m_hWnd != NULL )
		DestroyWindow();
}

bool CProgressDialog::Create( const std::tstring& title, CWnd* pParentWnd /*= NULL*/ )
{
	ASSERT( !IsRunning() );
	if ( !__super::Create( IDD_PROGRESS_DIALOG, pParentWnd ) )
		return false;

	ShowWindow( SW_SHOW );
	ui::SetWindowText( m_hWnd, title );

	m_pMsgPump.reset( new CScopedPumpMessage( 1, pParentWnd ) );		// disable the parent window during long operation to simulate a "modal-collaborative" long operation run
	return true;
}

void __declspec( noreturn ) CProgressDialog::Abort( void ) throws_( CUserAbortedException )
{
	throw CUserAbortedException();
}

bool CProgressDialog::CheckRunning( void ) const throws_( CUserAbortedException )
{
	if ( IsRunning() )
		return true;

	Abort();						// this throws
}

void CProgressDialog::SetOperationLabel( const std::tstring& operationLabel )
{
	m_operationLabel = operationLabel;

	if ( IsRunning() )
		m_operationStatic.SetWindowText( m_operationLabel );
}

void CProgressDialog::ShowStage( bool show /*= true*/ )
{
	ui::ShowWindow( m_stageLabelStatic, show );
	ui::ShowWindow( m_stageStatic, show );
}

void CProgressDialog::SetStageLabel( const std::tstring& stageLabel )
{
	ASSERT( IsRunning() );

	m_stageLabel = stageLabel;
	m_stageCount = 0;

	DisplayStageLabel();
	ShowStage();
}

void CProgressDialog::ShowItem( bool show /*= true*/ )
{
	ui::ShowWindow( m_itemLabelStatic, show );
	ui::ShowWindow( m_itemStatic, show );
}

void CProgressDialog::SetItemLabel( const std::tstring& itemLabel )
{
	ASSERT( IsRunning() );

	m_itemLabel = itemLabel;
	m_itemNo = 0;

	DisplayItemLabel();
	ShowItem();
}

void CProgressDialog::SetProgressStep( int step )
{
	ASSERT( IsRunning() );
	m_progressBar.SetStep( step );
}


// ui::IProgressCallback interface implementation

void CProgressDialog::SetProgressRange( int lower, int upper, bool rewindPos /*= false*/ )
{
	upper = std::max( upper, lower );

	SetMarqueeProgress( false );			// switch to bounded progress
	m_progressBar.SetRange32( lower, upper );
	m_itemCount = upper + 1;

	if ( rewindPos )
	{
		m_itemNo = 0;
		m_progressBar.SetPos( lower );
		m_progressBar.UpdateWindow();
	}

	DisplayItemCounts();
}

bool CProgressDialog::SetMarqueeProgress( bool marquee /*= true*/ )
{
	if ( HasFlag( m_progressBar.GetStyle(), PBS_MARQUEE ) == marquee )
		return false;			// no change required

	SetFlag( m_optionFlags, MarqueeProgress, marquee );
	m_progressBar.ModifyStyle( marquee ? 0 : PBS_MARQUEE, marquee ? PBS_MARQUEE : 0 );

	if ( marquee )
	{
		m_progressBar.SetMarquee( true, 0 );
		m_itemCount = 0;
	}
	return true;
}

void CProgressDialog::SetProgressState( int barState /*= PBST_NORMAL*/ )
{
	ProcessInput();

	m_progressBar.Invalidate();
	m_progressBar.UpdateWindow();
	m_progressBar.SetState( barState );
}

void CProgressDialog::AdvanceStage( const std::tstring& stageName ) throws_( CUserAbortedException )
{
	PumpMessages();

	++m_stageCount;

	if ( HasFlag( m_optionFlags, StageLabelCount ) )
		DisplayStageLabel();

	m_stageStatic.SetWindowText( stageName );
}

void CProgressDialog::AdvanceItem( const std::tstring& itemName ) throws_( CUserAbortedException )
{
	PumpMessages();

	StepIt();				// increment m_itemNo and show some progress

	if ( HasFlag( m_optionFlags, ItemLabelCount ) )
		DisplayItemLabel();

	m_itemStatic.SetWindowText( itemName );
	DisplayItemCounts();
}

void CProgressDialog::AdvanceItemToEnd( void ) throws_( CUserAbortedException )
{
	ProcessInput();

	int lower, upper;
	m_progressBar.GetRange( lower, upper );
	m_progressBar.SetPos( upper );

	m_itemNo = m_itemCount = upper + 1;
	DisplayItemCounts();
}

void CProgressDialog::ProcessInput( void ) const throws_( CUserAbortedException )
{
	if ( m_pMsgPump.get() != NULL )
		m_pMsgPump->CheckPump();		// pump any messages in the queue

	CheckRunning();						// throws if dialog got destroyed in the meantime
}


std::tstring CProgressDialog::FormatLabelCount( const std::tstring& label, int count )
{
	std::tstring labelText = label;
	if ( count != 0 )
		labelText += str::Format( _T(" (%s)"), num::FormatNumber( count, str::GetUserLocale() ).c_str() );

	static const std::tstring s_labelSuffix = _T(":");
	labelText += s_labelSuffix;
	return labelText;
}

void CProgressDialog::DisplayStageLabel( void )
{
	m_stageLabelStatic.SetWindowText( FormatLabelCount( m_stageLabel, HasFlag( m_optionFlags, StageLabelCount ) ? m_stageCount : 0 ) );
}

void CProgressDialog::DisplayItemLabel( void )
{
	m_itemLabelStatic.SetWindowText( FormatLabelCount( m_itemLabel, HasFlag( m_optionFlags, ItemLabelCount ) ? m_itemNo : 0 ) );
}

void CProgressDialog::DisplayItemCounts( void )
{
	std::tstring text;

	if ( m_itemNo != 0 )
	{
		text = _T("Item ");
		text += num::FormatNumber( m_itemNo, str::GetUserLocale() ).c_str();
	}

	if ( !IsMarqueeProgress() )
		if ( m_itemCount != 0 )
		{
			bool wasEmpty = text.empty();
			stream::Tag( text, num::FormatNumber( m_itemCount, str::GetUserLocale() ), _T(" of ") );
			if ( wasEmpty )
				text += _T(" items");
		}

	m_itemCountStatic.SetWindowText( text );
}

void CProgressDialog::PumpMessages( void ) throws_( CUserAbortedException )
{
	ProcessInput();
	ResizeLabelsContentsToFit();
}

bool CProgressDialog::StepIt( void )
{
	bool updated = false;

	++m_itemNo;

	int stepSize = m_progressBar.GetStep();

	if ( stepSize <= 1 || 0 == ( m_itemNo % stepSize ) )
		m_progressBar.StepIt();				// update progress bar once every step divider

	return updated;
}

bool CProgressDialog::ResizeLabelsContentsToFit( void )
{
	int idealWidth = m_stageLabelStatic.ComputeIdealTextSize().cx;
	idealWidth = utl::max( m_itemLabelStatic.ComputeIdealTextSize().cx, idealWidth );

	enum { StageLabel, ItemLabel, StageStatic, ItemStatic };

	std::vector< ui::CCtrlPlace > ctrls;
	ctrls.push_back( ui::CCtrlPlace( m_stageLabelStatic ) );
	ctrls.push_back( ui::CCtrlPlace( m_itemLabelStatic ) );

	int currentWidth = utl::max( ctrls[ StageLabel ].m_rect.Width(), ctrls[ ItemLabel ].m_rect.Width() );
	int deltaWidth = idealWidth - currentWidth;

	if ( idealWidth <= currentWidth )
		return false;

	ctrls.push_back( ui::CCtrlPlace( m_stageStatic ) );
	ctrls.push_back( ui::CCtrlPlace( m_itemStatic ) );

	if ( deltaWidth > 0 )
	{
		// stretch labels by deltaWidth
		ctrls[ StageLabel ].m_rect.right += deltaWidth;
		ctrls[ ItemLabel ].m_rect.right += deltaWidth;

		// shrink statics by deltaWidth
		ctrls[ StageStatic ].m_rect.left += deltaWidth;
		ctrls[ ItemStatic ].m_rect.left += deltaWidth;
	}

	if ( !ui::RepositionControls( ctrls ) )
		return false;

	// adjust original layout to reflect the deltaWidth change (resize/move)
	CLayoutEngine& rLayoutEngine = GetLayoutEngine();
	CSize deltaOrigin( deltaWidth, 0 ), deltaSize( -deltaWidth, 0 );

	rLayoutEngine.AdjustControlInitialPosition( ctrls[ StageStatic ].GetCtrlId(), deltaOrigin, deltaSize );
	rLayoutEngine.AdjustControlInitialPosition( ctrls[ ItemStatic ].GetCtrlId(), deltaOrigin, deltaSize );
	return true;
}

void CProgressDialog::DoDataExchange( CDataExchange* pDX )
{
	const bool firstInit = NULL == m_operationStatic.m_hWnd;

	DDX_Control( pDX, IDC_OPERATION_LABEL, m_operationStatic );
	DDX_Control( pDX, IDC_STAGE_LABEL, m_stageLabelStatic );
	DDX_Control( pDX, IDC_STAGE_STATIC, m_stageStatic );
	DDX_Control( pDX, IDC_ITEM_LABEL, m_itemLabelStatic );
	DDX_Control( pDX, IDC_ITEM_STATIC, m_itemStatic );
	DDX_Control( pDX, IDC_ITEM_COUNTS_STATIC, m_itemCountStatic );
	DDX_Control( pDX, IDC_CLOCK_STATIC, *m_pClockStatic );
	DDX_Control( pDX, IDC_PROGRESS_BAR, m_progressBar );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			m_operationStatic.SetWindowText( m_operationLabel );

			if ( HasFlag( m_optionFlags, HideStage ) )
				ShowStage( false );

			if ( HasFlag( m_optionFlags, HideItem ) )
				ShowItem( false );

			if ( HasFlag( m_optionFlags, HideProgress ) )
				ui::ShowWindow( m_progressBar, false );

			SetMarqueeProgress( IsMarqueeProgress() );
		}
	}

	__super::DoDataExchange( pDX );
}

BOOL CProgressDialog::DestroyWindow( void )
{
	m_pMsgPump.reset();			// this will re-enable the parent window post simulated "modal-collaborative" long operation run
	return __super::DestroyWindow();
}


// message handlers

BEGIN_MESSAGE_MAP( CProgressDialog, CLayoutDialog )
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CProgressDialog::OnDestroy( void )
{
	m_pMsgPump.reset();			// just in case, should've aleady been reset on CProgressDialog::DestroyWindow()
	__super::OnDestroy();
}
