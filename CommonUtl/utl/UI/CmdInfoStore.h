#ifndef CmdInfoStore_h
#define CmdInfoStore_h
#pragma once

#include <unordered_map>


namespace ui
{
	interface ICustomCmdInfo;


	struct CCmdInfo
	{
		CCmdInfo( UINT cmdId = 0 );
		CCmdInfo( const std::tstring& info ) { Setup( info ); }

		bool IsValid( void ) const { return !m_statusText.empty() || !m_tooltipText.empty(); }
		void Setup( const std::tstring& info );			// "<statusbar-text>\n<tooltip-text>"
	public:
		std::tstring m_statusText;
		std::tstring m_tooltipText;
	};


	class CCmdInfoStore : private utl::noncopyable
	{
		CCmdInfoStore( void ) {}
	public:
		static CCmdInfoStore& Instance( void );

		const CCmdInfo* FindInfo( UINT cmdId ) const;
		const CCmdInfo* RetrieveInfo( UINT cmdId );

		void RegisterCustom( UINT cmdId, const std::tstring& info );
		void DisableInfo( UINT cmdId ) { RegisterCustom( cmdId, std::tstring() ); }

		bool HandleGetMessageString( std::tstring& rMessage, UINT cmdId );		// status bar info for
		bool HandleTooltipNeedText( NMHDR* pNmHdr, LRESULT* pResult, const ui::ICustomCmdInfo* pCustomInfo = nullptr );	// tooltip notifications (TTN_NEEDTEXTA, TTN_NEEDTEXTW)
	private:
		std::unordered_map<UINT, CCmdInfo> m_cmdInfos;
	public:
		static int s_autoPopDuration;
	};
}


#endif // CmdInfoStore_h
