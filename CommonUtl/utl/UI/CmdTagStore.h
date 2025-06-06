#ifndef CmdTagStore_h
#define CmdTagStore_h
#pragma once

#include <unordered_map>


namespace ui
{
	interface ICustomCmdInfo;


	struct CCmdTag
	{
		CCmdTag( UINT cmdId = 0 );
		CCmdTag( const std::tstring& cmdSpec ) { Setup( cmdSpec ); }

		bool IsValid( void ) const { return !m_statusText.empty() || !m_tooltipText.empty(); }
		void Setup( const std::tstring& cmdSpec );			// "<statusbar-text>\n<tooltip-text>"
	public:
		std::tstring m_statusText;
		std::tstring m_tooltipText;
	};


	class CCmdTagStore : private utl::noncopyable
	{
		CCmdTagStore( void ) {}
	public:
		static CCmdTagStore& Instance( void );

		const CCmdTag* FindTag( UINT cmdId ) const;
		const CCmdTag* RetrieveTag( UINT cmdId );

		void RegisterCustom( UINT cmdId, const std::tstring& cmdSpec );
		void ClearTag( UINT cmdId ) { RegisterCustom( cmdId, std::tstring() ); }

		bool HandleGetMessageString( std::tstring& rMessage, UINT cmdId );		// status bar info for
		bool HandleTooltipNeedText( NMHDR* pNmHdr, LRESULT* pResult, const ui::ICustomCmdInfo* pCustomInfo = nullptr );	// tooltip notifications (TTN_NEEDTEXTA, TTN_NEEDTEXTW)
	private:
		std::unordered_map<UINT, CCmdTag> m_cmdTags;
	public:
		static int s_autoPopDuration;
	};
}


#endif // CmdTagStore_h
