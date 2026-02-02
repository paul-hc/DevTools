
#include "pch.h"
#include "SubjectAdapter.h"
#include "ShellPidl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	// CCodeAdapter implementation

	ui::ISubjectAdapter* CCodeAdapter::Instance( void )
	{
		static CCodeAdapter s_codeAdapter;
		return &s_codeAdapter;
	}

	std::tstring CCodeAdapter::FormatCode( const utl::ISubject* pSubject ) const
	{
		return utl::GetSafeCode( pSubject );
	}


	// CDisplayCodeAdapter implementation

	ui::ISubjectAdapter* CDisplayCodeAdapter::Instance( void )
	{
		static CDisplayCodeAdapter s_displayCodeAdapter;
		return &s_displayCodeAdapter;
	}

	std::tstring CDisplayCodeAdapter::FormatCode( const utl::ISubject* pSubject ) const
	{
		return utl::GetSafeDisplayCode( pSubject );
	}


	// CPathPidlAdapter implementation

	ui::ISubjectAdapter* CPathPidlAdapter::InstanceUI( void )
	{	// SIGDN_DESKTOPABSOLUTEEDITING: "Control Panel\\All Control Panel Items\\Region"
		static CPathPidlAdapter s_uiAdapter( SIGDN_DESKTOPABSOLUTEEDITING );
		return &s_uiAdapter;
	}

	ui::ISubjectAdapter* CPathPidlAdapter::InstanceParsing( void )
	{	// SIGDN_DESKTOPABSOLUTEPARSING: "::{26EE0668-A00A-44D7-9371-BEB064C98683}\\0\\::{62D8ED13-C9D0-4CE8-A914-47DD628FB1B0}"
		static CPathPidlAdapter s_parsingAdapter( SIGDN_DESKTOPABSOLUTEPARSING );
		return &s_parsingAdapter;
	}

	ui::ISubjectAdapter* CPathPidlAdapter::InstanceDisplay( void )
	{	// SIGDN_NORMALDISPLAY: "Region"
		static CPathPidlAdapter s_normalAdapter( SIGDN_NORMALDISPLAY );
		return &s_normalAdapter;
	}

	std::tstring CPathPidlAdapter::FormatCode( const utl::ISubject* pSubject ) const
	{
		if ( const CPathPidlItem* pPathPidlItem = checked_static_cast<const CPathPidlItem*>( pSubject ) )
			if ( pPathPidlItem->IsFilePath() )
			{
				if ( SIGDN_NORMALDISPLAY == m_pidlDisplayNameType )
					return pPathPidlItem->GetDisplayCode();		// relative filename
				else
					return pPathPidlItem->GetCode();
			}
			else if ( pPathPidlItem->IsSpecialPidl() )
				return pPathPidlItem->GetPidl().GetName( m_pidlDisplayNameType );

		return str::GetEmpty();
	}
}
