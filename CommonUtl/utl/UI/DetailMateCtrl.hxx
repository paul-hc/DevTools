#ifndef DetailMateCtrl_hxx
#define DetailMateCtrl_hxx

#include "CmdTagStore.h"


// CDetailMateCtrl template code

template< typename BaseCtrl >
CDetailMateCtrl<BaseCtrl>::CDetailMateCtrl( ui::IBuddyCommandHandler* pHostCmdHandler )
	: m_pHostCmdHandler( pHostCmdHandler )
	, m_pHostCtrl( dynamic_cast<CWnd*>( m_pHostCmdHandler ) )
{
	ASSERT_PTR( m_pHostCmdHandler );
}

template< typename BaseCtrl >
void CDetailMateCtrl<BaseCtrl>::QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	pTooltip;
	if ( const ui::CCmdTag* pFoundCmdTag = ui::CCmdTagStore::Instance().RetrieveTag( cmdId ) )
		if ( pFoundCmdTag->IsValid() )
			rText = pFoundCmdTag->m_tooltipText;
}

// message handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CDetailMateCtrl, BaseCtrl, BaseCtrl )
	ON_CONTROL_REFLECT_EX( CN_COMMAND, OnReflect_Command )
END_MESSAGE_MAP()

template< typename BaseCtrl >
BOOL CDetailMateCtrl<BaseCtrl>::OnReflect_Command( void )
{
	return m_pHostCmdHandler->OnBuddyCommand( this->GetDlgCtrlID() );		// skip parent handling if handled
}


#endif // DetailMateCtrl_hxx
