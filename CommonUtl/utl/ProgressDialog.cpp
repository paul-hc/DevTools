
#include "stdafx.h"
#include "ProgressDialog.h"
#include "LayoutEngine.h"
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
		{ IDC_STEP_STATIC, SizeX },
		{ IDC_PROGRESS_BAR, SizeX },
		{ IDCANCEL, pctMoveX( 50 ) }
	};
}


CProgressDialog::CProgressDialog( const std::tstring& operationLabel, int optionFlags /*= MarqueeProgress*/ )
	: CLayoutDialog()
	, m_operationLabel( operationLabel )
	, m_optionFlags( optionFlags )
	, m_stageCount( 0 )
	, m_stepCount( 0 )
	, m_stageLabelStatic( CNormalStatic::Instruction )
	, m_stepLabelStatic( CNormalStatic::Instruction )
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

void CProgressDialog::Abort( void ) throws_( CUserAbortedException )
{
	throw CUserAbortedException();
}

bool CProgressDialog::CheckRunning( void ) const throws_( CUserAbortedException )
{
	if ( IsRunning() )
		return true;

	Abort();
	return false;
}

void CProgressDialog::SetOperationLabel( const std::tstring& operationLabel )
{
	m_operationLabel = operationLabel;

	if ( IsRunning() )
		ui::SetWindowText( m_operationStatic, m_operationLabel );
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

void CProgressDialog::ShowStep( bool show /*= true*/ )
{
	ui::ShowWindow( m_stepLabelStatic, show );
	ui::ShowWindow( m_stepStatic, show );
}

void CProgressDialog::SetStepLabel( const std::tstring& stepLabel )
{
	ASSERT( IsRunning() );

	m_stepLabel = stepLabel;
	m_stepCount = 0;

	DisplayStepLabel();
	ShowStep();
}

void CProgressDialog::SetProgressStep( int step )
{
	ASSERT( IsRunning() );
	m_progressBar.SetStep( step );
}


// ui::IProgressBox interface implementation

void CProgressDialog::SetProgressRange( int lower, int upper, bool rewindPos /*= false*/ )
{
	SetMarqueeProgress( false );			// switch to bounded progress
	m_progressBar.SetRange32( lower, std::max( upper, lower ) );

	if ( rewindPos )
	{
		m_stepCount = 0;
		m_progressBar.SetPos( lower );
	}
}

bool CProgressDialog::SetMarqueeProgress( bool marquee /*= true*/ )
{
	if ( HasFlag( m_progressBar.GetStyle(), PBS_MARQUEE ) == marquee )
		return false;			// no change required

	SetFlag( m_optionFlags, MarqueeProgress, marquee );
	m_progressBar.ModifyStyle( marquee ? 0 : PBS_MARQUEE, marquee ? PBS_MARQUEE : 0 );

	if ( marquee )
		m_progressBar.SetMarquee( true, 0 );
	return true;
}

void CProgressDialog::AdvanceStage( const std::tstring& stageName ) throws_( CUserAbortedException )
{
	PumpMessages();

	++m_stageCount;

	if ( HasFlag( m_optionFlags, StageLabelCount ) )
		DisplayStageLabel();

	ui::SetWindowText( m_stageStatic, stageName );
}

void CProgressDialog::AdvanceStepItem( const std::tstring& stepItemName ) throws_( CUserAbortedException )
{
	PumpMessages();

	StepIt();				// increment step count and show some progress

	if ( HasFlag( m_optionFlags, StepLabelCount ) )
		DisplayStepLabel();

	ui::SetWindowText( m_stepStatic, stepItemName );
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
	ui::SetWindowText( m_stageLabelStatic, FormatLabelCount( m_stageLabel, HasFlag( m_optionFlags, StageLabelCount ) ? m_stageCount : 0 ) );
}

void CProgressDialog::DisplayStepLabel( void )
{
	ui::SetWindowText( m_stepLabelStatic, FormatLabelCount( m_stepLabel, HasFlag( m_optionFlags, StepLabelCount ) ? m_stepCount : 0 ) );
}

void CProgressDialog::PumpMessages( void ) throws_( CUserAbortedException )
{
	ResizeLabelsToContents();
	if ( m_pMsgPump.get() != NULL )
		m_pMsgPump->CheckPump();		// pump any messages in the queue

	CheckRunning();						// throws if dialog got destroyed in the meantime
}

bool CProgressDialog::StepIt( void )
{
	bool updatedProgress = false;

	++m_stepCount;

	int stepSize = m_progressBar.GetStep();

	if ( stepSize <= 1 || 0 == ( m_stepCount % stepSize ) )
		m_progressBar.StepIt();				// call StepIt() on progress bar once every step divider

	return updatedProgress;
}

bool CProgressDialog::ResizeLabelsToContents( void )
{
	return false;
}

void CProgressDialog::DoDataExchange( CDataExchange* pDX )
{
	const bool firstInit = NULL == m_operationStatic.m_hWnd;

	DDX_Control( pDX, IDC_OPERATION_LABEL, m_operationStatic );
	DDX_Control( pDX, IDC_STAGE_LABEL, m_stageLabelStatic );
	DDX_Control( pDX, IDC_STAGE_STATIC, m_stageStatic );
	DDX_Control( pDX, IDC_STEP_LABEL, m_stepLabelStatic );
	DDX_Control( pDX, IDC_STEP_STATIC, m_stepStatic );
	DDX_Control( pDX, IDC_PROGRESS_BAR, m_progressBar );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
	{
		if ( firstInit )
		{
			ui::SetWindowText( m_operationStatic, m_operationLabel );

			if ( HasFlag( m_optionFlags, HideStage ) )
				ShowStage( false );

			if ( HasFlag( m_optionFlags, HideStep ) )
				ShowStep( false );

			if ( HasFlag( m_optionFlags, HideProgress ) )
				ui::ShowWindow( m_progressBar, false );

			SetMarqueeProgress( IsMarqueeProgress() );
		}
	}

	__super::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CProgressDialog, CLayoutDialog )
	ON_WM_DESTROY()
END_MESSAGE_MAP()

void CProgressDialog::OnDestroy( void )
{
	CWnd* pOwner = GetOwner();

	m_pMsgPump.reset();			// this will re-enable the parent window post simulated "modal-collaborative" long operation run
	__super::OnDestroy();

	if ( pOwner->GetSafeHwnd() != NULL )
		if ( pOwner->IsWindowEnabled() )
			pOwner->SetForegroundWindow();
}
