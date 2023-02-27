#ifndef ProgressDialog_h
#define ProgressDialog_h
#pragma once

#include "utl/IProgressService.h"
#include "LayoutDialog.h"
#include "ThemeStatic.h"


class CClockStatic;
class CScopedPumpMessage;


// Modeless dialog with some information fields and a progress-bar.
// Allows keyboard and mouse user input (pumps pending messages) while performing long operations when advancing methods are called.
// The object lifetime is controlled by the caller, but the dialog window may get destroyed internally when the Close button is pressed.
// Throws a CUserAbortedException if the caller invokes any advancing method while the dialog window has been closed.
// Note: uses Null Pattern for convenience, so that the client can continue using the IProgressService interface even after the user closed the dialog.
//
class CProgressDialog : public CLayoutDialog
					  , private utl::IProgressHeader
					  , private utl::IProgressService
{
public:
	enum OptionFlag
	{
		HideStage		= 1 << 1,
		HideItem		= 1 << 2,
		HideProgress	= 1 << 3,

		MarqueeProgress	= 1 << 4,
		StageLabelCount	= 1 << 5,
		ItemLabelCount	= 1 << 6,

			LabelsCount = StageLabelCount | ItemLabelCount,
	};

	CProgressDialog( const std::tstring& operationLabel, int optionFlags = MarqueeProgress );
	virtual ~CProgressDialog();

	bool IsRunning( void ) const { return m_hWnd != nullptr; }
	bool CheckRunning( void ) const throws_( CUserAbortedException );

	bool Create( const std::tstring& title, CWnd* pParentWnd = nullptr );
	static void __declspec(noreturn) Abort( void ) throws_( CUserAbortedException );

	CProgressCtrl& GetProgressBar( void ) { return m_progressBar; }
	bool IsMarqueeProgress( void ) const { return HasFlag( m_optionFlags, MarqueeProgress ); }

	utl::IProgressService* GetService( void ) { return this; }
protected:
	enum ProgressType { Marquee, Bounded };

	bool SetProgressType( ProgressType progressType );
	void SetProgressRange( int lower, int upper, bool rewindPos );

	// utl::IProgressHeader interface
	virtual void SetDialogTitle( const std::tstring& title );
	virtual void SetOperationLabel( const std::tstring& operationLabel );

	virtual void ShowStage( bool show = true );
	virtual void SetStageLabel( const std::tstring& stageLabel );

	virtual void ShowItem( bool show = true );
	virtual void SetItemLabel( const std::tstring& itemLabel );
private:
	// utl::IProgressService interface
	virtual utl::IProgressHeader* GetHeader( void );
	virtual bool SetMarqueeProgress( void );
	virtual void SetBoundedProgressCount( size_t itemCount, bool rewindPos = true );
	virtual void SetProgressStep( int step );			// step divider for less granular progress updates (default is 10 for CProgressCtrl)
	virtual void SetProgressState( int barState = PBST_NORMAL );

	virtual void AdvanceStage( const std::tstring& stageName ) throws_( CUserAbortedException );
	virtual void AdvanceItem( const std::tstring& itemName ) throws_( CUserAbortedException );
	virtual void AdvanceItemToEnd( void ) throws_( CUserAbortedException );
	virtual void ProcessInput( void ) const throws_( CUserAbortedException );
protected:
	static std::tstring FormatLabelCount( const std::tstring& label, int count );
	void DisplayStageLabel( void );
	void DisplayItemLabel( void );
	void DisplayItemCounts( void );

	void PumpMessages( void ) throws_( CUserAbortedException );			// collaborative multitasking: dispatch input messages, and throw if dialog got destroyed
	bool StepIt( void );
	bool ResizeLabelsContentsToFit( void );
private:
	std::tstring m_operationLabel;
	std::tstring m_stageLabel;
	std::tstring m_itemLabel;
	int m_optionFlags;
	std::auto_ptr<CScopedPumpMessage> m_pMsgPump;
	mutable bool m_isAborted;

	// internal counters, correlated yet independent of m_progressBar.GetPos()
	int m_stageCount;
	int m_itemNo, m_itemCount;
private:
	// enum { IDD = IDD_PROGRESS_DIALOG };

	CHeadlineStatic m_operationStatic;
	CRegularStatic m_stageLabelStatic, m_stageStatic;
	CRegularStatic m_itemLabelStatic, m_itemStatic;
	CRegularStatic m_itemCountStatic;
	std::auto_ptr<CClockStatic> m_pClockStatic;

	CProgressCtrl m_progressBar;

	// generated stuff
public:
	virtual BOOL DestroyWindow( void );
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnDestroy( void );
	afx_msg LRESULT OnDisableModal( WPARAM, LPARAM );

	DECLARE_MESSAGE_MAP()
};


#endif // ProgressDialog_h
