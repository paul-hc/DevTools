
#include "pch.h"
#include "DirPathGroup.h"
#include "utl/EnumTags.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace inc
{
	const CEnumTags& GetTags_Location( void )
	{
		static const CEnumTags s_tags( _T("Include Path|Local Path|Additional Path|Absolute Path|Source Path|Library Path|Binary Path") );
		ASSERT( s_tags.GetUiTags().size() == ( BinaryPath + 1 ) );
		return s_tags;
	}


	// CDirPathGroup implementation

	CDirPathGroup::CDirPathGroup( inc::Location location )
		: m_location( location )
	{
	}

	CDirPathGroup::CDirPathGroup( const TCHAR envVarName[], inc::Location location )
		: m_location( location )
	{
		AddExpanded( envVarName );
	}

	std::tstring CDirPathGroup::Format( void ) const
	{
		return str::FormatNameValueSpec<TCHAR>( GetTags_Location().FormatUi( m_location ), Join() );
	}

	bool CDirPathGroup::Parse( const std::tstring& spec )
	{
		std::tstring tag, value;
		if ( str::ParseNameValue<TCHAR>( tag, value, spec ) )
			if ( m_location == GetTags_Location().ParseUi( tag ) )
			{
				Split( value.c_str() );
				return true;
			}

		return false;
	}
}
