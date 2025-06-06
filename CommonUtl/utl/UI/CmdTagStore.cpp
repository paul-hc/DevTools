
#include "pch.h"
#include "CmdTagStore.h"
#include "CtrlInterfaces.h"
#include "Dialog_fwd.h"
#include "MfcUtilities.h"		// for struct ui::CTooltipTextMessage
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	// CCmdTag implementation

	CCmdTag::CCmdTag( UINT cmdId /*= 0*/ )
	{
		if ( cmdId != 0 )
			Setup( str::Load( cmdId ) );
	}

	void CCmdTag::Setup( const std::tstring& cmdSpec )
	{
		size_t sepPos = cmdSpec.find( _T('\n') );

		if ( sepPos != std::tstring::npos )
		{
			m_statusText = cmdSpec.substr( 0, sepPos );
			m_tooltipText = cmdSpec.substr( sepPos + 1 );
		}
		else if ( std::tstring::npos == cmdSpec.find( _T('|') ) )		// ignore column layout descriptors (list controls, grids, etc)
			m_statusText = m_tooltipText = cmdSpec;
		else
		{
			m_statusText.clear();
			m_tooltipText.clear();
		}
	}


	// CCmdTagStore implementation

	int CCmdTagStore::s_autoPopDuration = 0;

	CCmdTagStore& CCmdTagStore::Instance( void )
	{
		static CCmdTagStore s_cmdStore;
		return s_cmdStore;
	}

	const CCmdTag* CCmdTagStore::FindTag( UINT cmdId ) const
	{
		std::unordered_map<UINT, CCmdTag>::const_iterator itFound = m_cmdTags.find( cmdId );
		return itFound != m_cmdTags.end() ? &itFound->second : nullptr;
	}

	const CCmdTag* CCmdTagStore::RetrieveTag( UINT cmdId )
	{
		if ( const CCmdTag* pFoundInfo = FindTag( cmdId ) )
			return pFoundInfo;

		std::tstring cmdSpec = str::Load( cmdId );
		if ( !cmdSpec.empty() )
		{
			CCmdTag* pCmdTag = &m_cmdTags[ cmdId ];
			pCmdTag->Setup( cmdSpec );
			return pCmdTag;
		}

		return nullptr;
	}

	void CCmdTagStore::RegisterCustom( UINT cmdId, const std::tstring& cmdSpec )
	{
		CCmdTag cmdTag( cmdSpec );			// allow empty strings to disable tooltips for a command
		m_cmdTags[ cmdId ] = cmdTag;
	}

	bool CCmdTagStore::HandleGetMessageString( std::tstring& rMessage, UINT cmdId )
	{
		if ( const CCmdTag* pCmdTag = FindTag( cmdId ) )
			rMessage = pCmdTag->m_statusText;

		return !rMessage.empty();
	}

	bool CCmdTagStore::HandleTooltipNeedText( NMHDR* pNmHdr, LRESULT* pResult, const ui::ICustomCmdInfo* pCustomInfo /*= nullptr*/ )
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
			if ( const CCmdTag* pCmdTag = FindTag( message.m_cmdId ) )
				text = pCmdTag->m_tooltipText;
			else
			{
				CCmdTag cmdTag( message.m_cmdId );		// use the resource string
				if ( cmdTag.IsValid() )
					text = cmdTag.m_tooltipText;
			}

		if ( !message.AssignTooltipText( text ) )
			return false;				// no tooltip info for this command

		utl::AssignPtr( pResult, (LRESULT)0 );		// prevent 64 bit errors
		return true;					// message was handled
	}
}
