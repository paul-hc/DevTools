#ifndef DetailMateCtrl_hxx
#define DetailMateCtrl_hxx

#include "CmdInfoStore.h"


// CDetailMateCtrl template code

template< typename BaseCtrl >
CDetailMateCtrl<BaseCtrl>::CDetailMateCtrl( ui::IBuddyCommandHandler* pHostCmdHandler )
	: m_pHostCmdHandler( pHostCmdHandler )
	, m_pHostCtrl( dynamic_cast<CWnd*>( m_pHostCmdHandler ) )
{
	ASSERT_PTR( m_pHostCmdHandler );
}

template< typename BaseCtrl >
void CDetailMateCtrl<BaseCtrl>::QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const
{
	pTooltip;
	if ( const ui::CCmdInfo* pFoundInfo = ui::CCmdInfoStore::Instance().RetrieveInfo( cmdId ) )
		if ( pFoundInfo->IsValid() )
			rText = pFoundInfo->m_tooltipText;
}

// message handlers

BEGIN_TEMPLATE_MESSAGE_MAP( CDetailMateCtrl, BaseCtrl, BaseCtrl )
	ON_CONTROL_REFLECT_EX( CN_COMMAND, OnReflect_Command )
END_MESSAGE_MAP()

template< typename BaseCtrl >
BOOL CDetailMateCtrl<BaseCtrl>::OnReflect_Command( void )
{
	return m_pHostCmdHandler->OnBuddyCommand( GetDlgCtrlID() );		// skip parent handling if handled
}


#endif // DetailMateCtrl_hxx
