
#include "pch.h"
#include "CmdInfoStore.h"
#include "CtrlInterfaces.h"
#include "Dialog_fwd.h"
#include "MfcUtilities.h"		// for struct ui::CTooltipTextMessage
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	// CCmdInfo implementation

	CCmdInfo::CCmdInfo( UINT cmdId /*= 0*/ )
	{
		if ( cmdId != 0 )
			Setup( str::Load( cmdId ) );
	}

	void CCmdInfo::Setup( const std::tstring& info )
	{
		size_t sepPos = info.find( _T('\n') );

		if ( sepPos != std::tstring::npos )
		{
			m_statusText = info.substr( 0, sepPos );
			m_tooltipText = info.substr( sepPos + 1 );
		}
		else if ( std::tstring::npos == info.find( _T('|') ) )		// ignore column layout descriptors (list controls, grids, etc)
			m_statusText = m_tooltipText = info;
		else
		{
			m_statusText.clear();
			m_tooltipText.clear();
		}
	}


	// CCmdInfoStore implementation

	int CCmdInfoStore::s_autoPopDuration = 0;

	CCmdInfoStore& CCmdInfoStore::Instance( void )
	{
		static CCmdInfoStore s_cmdStore;
		return s_cmdStore;
	}

	const CCmdInfo* CCmdInfoStore::FindInfo( UINT cmdId ) const
	{
		std::unordered_map<UINT, CCmdInfo>::const_iterator itFound = m_cmdInfos.find( cmdId );
		return itFound != m_cmdInfos.end() ? &itFound->second : nullptr;
	}

	const CCmdInfo* CCmdInfoStore::RetrieveInfo( UINT cmdId )
	{
		if ( const CCmdInfo* pFoundInfo = FindInfo( cmdId ) )
			return pFoundInfo;

		std::tstring info = str::Load( cmdId );
		if ( !info.empty() )
		{
			CCmdInfo* pCmdInfo = &m_cmdInfos[ cmdId ];
			pCmdInfo->Setup( info );
			return pCmdInfo;
		}

		return nullptr;
	}

	void CCmdInfoStore::RegisterCustom( UINT cmdId, const std::tstring& info )
	{
		CCmdInfo cmdInfo( info );			// allow empty strings to disable tooltips for a command
		m_cmdInfos[ cmdId ] = cmdInfo;
	}

	bool CCmdInfoStore::HandleGetMessageString( std::tstring& rMessage, UINT cmdId )
	{
		if ( const CCmdInfo* pCmdInfo = FindInfo( cmdId ) )
			rMessage = pCmdInfo->m_statusText;

		return !rMessage.empty();
	}

	bool CCmdInfoStore::HandleTooltipNeedText( NMHDR* pNmHdr, LRESULT* pResult, const ui::ICustomCmdInfo* pCustomInfo /*= nullptr*/ )
	{	// code inspired by CFrameWnd::OnToolTipText()
		ASSERT_PTR( pNmHdr );

		ui::CTooltipTextMessage message( pNmHdr );
		if ( !message.IsValidNotification() )
			return false;				// ignore stray notifications

		if ( message.m_pTooltip != nullptr )
			if ( s_autoPopDuration != 0 && s_autoPopDuration != message.m_pTooltip->GetDelayTime( TTDT_AUTOPOP ) )
				message.m_pTooltip->SetDelayTime( TTDT_AUTOPOP, s_autoPopDuration );

		std::tstring text;

		if ( text.empty() && pCustomInfo != nullptr )
			pCustomInfo->QueryTooltipText( text, message.m_cmdId, message.m_pTooltip );

		if ( text.empty() && message.m_hCtrl != nullptr )
			if ( const ui::ICustomCmdInfo* pCtrlCustomInfo = dynamic_cast<const ui::ICustomCmdInfo*>( CWnd::FromHandlePermanent( message.m_hCtrl ) ) )
				pCtrlCustomInfo->QueryTooltipText( text, message.m_cmdId, message.m_pTooltip );		// redirect to the control object

		if ( text.empty() && !message.IgnoreResourceString() )
			if ( const CCmdInfo* pCmdInfo = FindInfo( message.m_cmdId ) )
				text = pCmdInfo->m_tooltipText;
			else
			{
				CCmdInfo cmdInfo( message.m_cmdId );		// use the resource string
				if ( cmdInfo.IsValid() )
					text = cmdInfo.m_tooltipText;
			}

		if ( !message.AssignTooltipText( text ) )
			return false;				// no tooltip info for this command

		utl::AssignPtr( pResult, (LRESULT)0 );		// prevent 64 bit errors
		return true;					// message was handled
	}
}
