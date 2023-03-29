
#include "stdafx.h"
#include "CmdInfoStore.h"
#include "CtrlInterfaces.h"
#include "Dialog_fwd.h"
#include "ReportListControl.h"
#include "WndUtils.h"
#include "ThemeStatic.h"

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

	const std::tstring CCmdInfoStore::m_nilText = _T("<nil>");
	int CCmdInfoStore::m_autoPopDuration = 0;

	CCmdInfoStore& CCmdInfoStore::Instance( void )
	{
		static CCmdInfoStore s_cmdStore;
		return s_cmdStore;
	}

	const CCmdInfo* CCmdInfoStore::FindInfo( UINT cmdId ) const
	{
		std::unordered_map< UINT, CCmdInfo >::const_iterator itFound = m_cmdInfos.find( cmdId );
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
			if ( m_autoPopDuration != 0 && m_autoPopDuration != message.m_pTooltip->GetDelayTime( TTDT_AUTOPOP ) )
				message.m_pTooltip->SetDelayTime( TTDT_AUTOPOP, m_autoPopDuration );

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


	// CTooltipTextMessage implementation

	CTooltipTextMessage::CTooltipTextMessage( NMHDR* pNmHdr )
		: m_pTooltip( static_cast<CToolTipCtrl*>( CWnd::FromHandlePermanent( pNmHdr->hwndFrom ) ) )
		, m_pTttA( TTN_NEEDTEXTA == pNmHdr->code ? reinterpret_cast<TOOLTIPTEXTA*>( pNmHdr ) : nullptr )
		, m_pTttW( TTN_NEEDTEXTW == pNmHdr->code ? reinterpret_cast<TOOLTIPTEXTW*>( pNmHdr ) : nullptr )
		, m_cmdId( static_cast<UINT>( pNmHdr->idFrom ) )
		, m_hCtrl( nullptr )
	{
		ASSERT( m_pTttA != nullptr || m_pTttW != nullptr );

		if ( ( m_pTttA != nullptr && HasFlag( m_pTttA->uFlags, TTF_IDISHWND ) ) ||
			 ( m_pTttW != nullptr && HasFlag( m_pTttW->uFlags, TTF_IDISHWND ) ) )
		{
			m_hCtrl = (HWND)pNmHdr->idFrom;						// idFrom is actually the HWND of the tool
			m_cmdId = ::GetDlgCtrlID( m_hCtrl );
		}
	}

	bool CTooltipTextMessage::IsValidNotification( void ) const
	{
		return m_cmdId != 0;
	}

	bool CTooltipTextMessage::AssignTooltipText( const std::tstring& text )
	{
		if ( text.empty() || CCmdInfoStore::m_nilText == text )
			return false;

		if ( m_pTttA != nullptr )
		{
			static std::string s_narrowText;			// static buffer (can be longer than 80 characters default limit)
			s_narrowText = str::AsNarrow( text );
			m_pTttA->lpszText = const_cast<char*>( s_narrowText.c_str() );
		}
		else if ( m_pTttW != nullptr )
		{
			static std::wstring s_wideText;				// static buffer (can be longer than 80 characters default limit)
			s_wideText = str::AsWide( text );
			m_pTttW->lpszText = const_cast<wchar_t*>( s_wideText.c_str() );
		}
		else
			ASSERT( false );

		// bring the tooltip window above other popup windows
		if ( m_pTooltip != nullptr )
			ui::BringWndToTop( *m_pTooltip );			// move window to the top/bottom of Z-order (above other popup windows)

		return true;			// valid tooltip text
	}

	bool CTooltipTextMessage::IgnoreResourceString( void ) const
	{
		return m_hCtrl != nullptr && IgnoreResourceString( m_hCtrl );
	}

	bool CTooltipTextMessage::IgnoreResourceString( HWND hCtrl )
	{
		if ( CWnd* pCtrl = CWnd::FromHandlePermanent( hCtrl ) )
			return		// ignore column layout descriptors (list controls, grids, etc)
				is_a<CReportListControl>( pCtrl ) ||
				is_a<CStatusStatic>( pCtrl );

		return false;
	}
}
