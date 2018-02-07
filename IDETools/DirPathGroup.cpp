
#include "stdafx.h"
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
		static const CEnumTags tags( _T("Include Path|Local Path|Additional Path|Absolute Path|Source Path|Library Path|Binary Path") );
		ASSERT( tags.GetUiTags().size() == ( BinaryPath + 1 ) );
		return tags;
	}


	// CDirPathGroup implementation

	std::tstring CDirPathGroup::Format( void ) const
	{
		return str::FormatNameValueSpec< TCHAR >( GetTags_Location().FormatUi( m_location ), Join() );
	}

	bool CDirPathGroup::Parse( const std::tstring& spec )
	{
		std::tstring tag, value;
		if ( str::ParseNameValue< TCHAR >( tag, value, spec ) )
			if ( m_location == GetTags_Location().ParseUi( tag ) )
			{
				Split( value.c_str() );
				return true;
			}

		return false;
	}
}
