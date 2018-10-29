#ifndef CustomTaskDialogs_h
#define CustomTaskDialogs_h
#pragma once

#include "utl/TaskDialog.h"


namespace my
{
	class CProgressBarTaskDialog : public CTaskDialog
	{
	public:
		CProgressBarTaskDialog( void )
			: CTaskDialog(
				_T("Progress Bar"),
				_T("Important!\nPlease read!"),
				_T("This is an important message to the user."),
				TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
				TDF_ENABLE_HYPERLINKS | TDF_CALLBACK_TIMER | TDF_SHOW_PROGRESS_BAR )
		{
			SetProgressBarRange( 0, 300 );
		}
	protected:
		// event overrides
		virtual HRESULT OnTimer( long lTick )
		{
			lTick;
			static int iCounter = 0;
			if ( 80 == iCounter )
				SetProgressBarState( PBST_ERROR );
			else if ( 190 == iCounter )
				SetProgressBarState( PBST_PAUSED );
			else if ( 260 == iCounter )
				SetProgressBarState( PBST_NORMAL );

			SetProgressBarPosition( iCounter );

			if ( 300 == iCounter )
				iCounter = 0;		// reset
			else
				iCounter += 5;

			return S_OK;
		}
	};

	class CSecondNavigationDialog : public CTaskDialog
	{
	public:
		CSecondNavigationDialog( void )
			: CTaskDialog(
				_T("Navigation Usage"),
				_T("Step 2"),
				_T("This is the second navigation dialog."),
				TDCBF_CANCEL_BUTTON,
				TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS )
		{
			AddButton( 101, _T("Go back!\nGo to the first navigation dialog.") );
			AddButton( 102, _T("Choice 1") );
			AddButton( 103, _T("Choice 2") );
		}

		void SetPreviousDialog( CTaskDialog* pPrevDlg ) { m_pPrevDlg = pPrevDlg; }
	protected:
		// event overrides
		virtual HRESULT OnButtonClick( int buttonId )
		{
			if ( 101 == buttonId )
			{
				NavigateTo( *m_pPrevDlg );
				return S_FALSE;
			}
			return S_OK;
		}
	private:
		CTaskDialog* m_pPrevDlg;
	};

	class CFirstNavigationDialog : public CTaskDialog
	{
	public:
		CFirstNavigationDialog( void )
			: CTaskDialog(
				_T("Navigation Usage"),
				_T("Step 1"),
				_T("This is the first navigation dialog."),
				TDCBF_CANCEL_BUTTON,
				TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS )
		{
			AddButton( 101 , _T("Go next!\nGo to the second navigation dialog.") );
		}
	protected:
		// event overrides
		virtual HRESULT OnButtonClick( int buttonId )
		{
			if ( 101 == buttonId )
			{
				m_nextDlg.SetPreviousDialog( this );
				NavigateTo( m_nextDlg );
				return S_FALSE;
			}
			return S_OK;
		}
	private:
		CSecondNavigationDialog m_nextDlg;
	};
}


#endif // CustomTaskDialogs_h
