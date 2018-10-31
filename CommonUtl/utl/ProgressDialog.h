#ifndef ProgressDialog_h
#define ProgressDialog_h
#pragma once

#include "IProgressBox.h"
#include "LayoutDialog.h"
#include "ThemeStatic.h"


class CScopedPumpMessage;


// Modeless dialog with some information fields and a progress-bar.
// Allows keyboard and mouse user input (pumps pending messages) while performing long operations when advancing methods are called.
// The object lifetime is controlled by the caller, but the dialog window gets destroyed internally when the Close button is pressed.
// Throws a CUserAbortedException if the caller invokes any advancing method while the dialog window has been closed.
//
class CProgressDialog : public CLayoutDialog
					  , public ui::IProgressBox
{
public:
	enum OptionFlag
	{
		HideStage = 1 << 1,
		HideStep = 1 << 2,
		HideProgress = 1 << 3,
		MarqueeProgress = 1 << 4,
		StageLabelCount = 1 << 5,
		StepLabelCount = 1 << 6,
			LabelsCount = StageLabelCount | StepLabelCount,
	};

	CProgressDialog( const std::tstring& operationLabel, int optionFlags = MarqueeProgress );
	virtual ~CProgressDialog();

	bool IsRunning( void ) const { return GetSafeHwnd() != NULL; }
	bool CheckRunning( void ) const throws_( CUserAbortedException );

	bool Create( const std::tstring& title, CWnd* pParentWnd = NULL );
	static void Abort( void ) throws_( CUserAbortedException );

	void SetOperationLabel( const std::tstring& operationLabel );

	void ShowStage( bool show = true );
	void SetStageLabel( const std::tstring& stageLabel );

	void ShowStep( bool show = true );
	void SetStepLabel( const std::tstring& stepLabel );

	CProgressCtrl& GetProgressBar( void ) { return m_progressBar; }
	bool IsMarqueeProgress( void ) const { return HasFlag( m_optionFlags, MarqueeProgress ); }
	void SetProgressStep( int step );			// step divider for less granular progress updates (default is 10 for CProgressCtrl)

	// ui::IProgressBox interface
	virtual void SetProgressRange( int lower, int upper, bool rewindPos = false );
	virtual bool SetMarqueeProgress( bool marquee = true );
	virtual void AdvanceStage( const std::tstring& stageName ) throws_( CUserAbortedException );
	virtual void AdvanceStepItem( const std::tstring& stepItemName ) throws_( CUserAbortedException );
protected:
	static std::tstring FormatLabelCount( const std::tstring& label, int count );
	void DisplayStageLabel( void );
	void DisplayStepLabel( void );

	void PumpMessages( void ) throws_( CUserAbortedException );			// collaborative multitasking: dispatch input messages, and throw if dialog got destroyed
	bool StepIt( void );
	bool ResizeLabelsToContents( void );
private:
	std::tstring m_operationLabel;
	std::tstring m_stageLabel;
	std::tstring m_stepLabel;
	int m_optionFlags;
	std::auto_ptr< CScopedPumpMessage > m_pMsgPump;

	// internal counters, correlated yet independent of m_progressBar.GetPos()
	int m_stageCount;
	int m_stepCount;
private:
	// enum { IDD = IDD_PROGRESS_DIALOG };

	CHeadlineStatic m_operationStatic;
	CNormalStatic m_stageLabelStatic, m_stageStatic;
	CNormalStatic m_stepLabelStatic, m_stepStatic;
	CProgressCtrl m_progressBar;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnDestroy( void );

	DECLARE_MESSAGE_MAP()
};


#endif // ProgressDialog_h
