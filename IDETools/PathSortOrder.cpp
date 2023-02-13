
#include "pch.h"
#include "PathSortOrder.h"
#include "utl/Algorithms.h"
#include "utl/EnumTags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	// CPathSortParts implementation

	CPathSortParts::CPathSortParts( const CPath& filePath )
		: CPathParts( filePath.Get() )
		, m_parentDirPath( filePath.GetParentPath() )
		, m_parentDirName( m_parentDirPath.GetFilename() )
		, m_pathExclExt( filePath.GetRemoveExt() )
	{
	}


	// CExtCustomOrder implementation

	const TCHAR CExtCustomOrder::s_defaultExtOrder[] =
		_T("|")			// extensionless headers
		_T(".h|")
		_T(".cpp|.lcc|.c|")
		_T(".hxx|.hpp|.inl|.cxx|.t|")
		_T(".idl|.odl|.tlb|")
		_T(".rc|.rc2|.rh|.dlg|.res|")
		_T(".def|")
		_T(".sln|.vcproj|.vcxproj|.filters|")
		_T(".dsp|.dsw|")
		_T(".dsm|.bas|.vbs|.wsh|")
		_T(".dll|.pkg|.lib|.exe|")
		_T(".sql|.tab|.pk|.pkg|.pkb|.pks|.pac|")
		_T(".ot|.otb");

	CExtCustomOrder& CExtCustomOrder::Instance( void )
	{
		static CExtCustomOrder s_extCustomOrder;
		return s_extCustomOrder;
	}

	void CExtCustomOrder::RegisterCustomOrder( const TCHAR extOrderList[] /*= s_defaultExtOrder*/, const TCHAR sep[] /*= _T("|")*/ )
	{
		std::vector< fs::CPath > extensions;
		str::Split( extensions, extOrderList, sep );

		for ( size_t order = 0; order != extensions.size(); ++order )
			m_extToOrderMap[ extensions[ order ] ] = order;
	}

	UINT CExtCustomOrder::LookupExtOrder( const std::tstring& ext ) const
	{
		// straight cast to fs::CPath since it's are binary compatible (avoid creating temporary variables)
		std::unordered_map< CPath, UINT >::const_iterator itFound = m_extToOrderMap.find( (const fs::CPath&)ext );
		if ( itFound != m_extToOrderMap.end() )
			return itFound->second;

		return UINT_MAX;			// unknown extension
	}

	pred::CompareResult CExtCustomOrder::CompareExtension( const std::tstring& leftExt, const std::tstring& rightExt )
	{
		return pred::Compare_Scalar( Instance().LookupExtOrder( leftExt ), Instance().LookupExtOrder( rightExt ) );
	}
}


namespace pred
{
	CompareResult ComparePathField( const fs::CPathSortParts& left, const fs::CPathSortParts& right, PathField pathField )
	{
		static const CompareNaturalPath s_compare;

		switch ( pathField )
		{
			default: ASSERT( false );
			case pfFullPath:
			{
				CompareResult result = s_compare( left.m_pathExclExt, right.m_pathExclExt );
				if ( Equal == result )
					result = ComparePathField( left, right, pfExt );		// custom extension order for files with the same path-name
				return result;
			}
			case pfDrive:		return s_compare( left.m_drive, right.m_drive );
			case pfDir:			return s_compare( left.m_dir, right.m_dir );
			case pfName:		return s_compare( left.m_fname, right.m_fname );
			case pfExt:			return fs::CExtCustomOrder::CompareExtension( left.m_ext, right.m_ext );
			case pfDirName:		return s_compare( left.m_parentDirName, right.m_parentDirName );
			case pfDirPath:		return s_compare( left.m_parentDirPath, right.m_parentDirPath );
			case pfDirNameExt:	return s_compare( left.m_parentDirName, right.m_parentDirName );
			case pfNameExt:		return s_compare( left.GetFilename(), right.GetFilename() );
			case pfCoreExt:		return s_compare( left.m_ext, right.m_ext );
		}
	}
}


// CPathSortOrder implementation

const std::vector< PathField >& CPathSortOrder::GetDefaultOrder( void )
{
	static const PathField s_defaultFields[] = { pfDirName, pfName, pfExt };
	static std::vector< PathField > s_defaultOrder( s_defaultFields, END_OF( s_defaultFields ) );

	return s_defaultOrder;
}

size_t CPathSortOrder::FindFieldPos( PathField field ) const
{
	return utl::FindPos( m_fields, field );
}

void CPathSortOrder::Clear( void )
{
	m_fields.clear();
	m_orderText.clear();
}

void CPathSortOrder::Add( PathField field )
{
	utl::RemoveValue( m_fields, field );
	m_fields.push_back( field );

	StoreOrderText();
}

void CPathSortOrder::Toggle( PathField field )
{
	std::vector< PathField >::iterator itFound = std::find( m_fields.begin(), m_fields.end(), field );
	if ( itFound != m_fields.end() )
		m_fields.erase( itFound );
	else
		m_fields.push_back( field );

	StoreOrderText();
}

void CPathSortOrder::StoreOrderText( void )
{
	std::vector< std::tstring > tags; tags.reserve( m_fields.size() );

	for ( std::vector< PathField >::const_iterator itField = m_fields.begin(); itField != m_fields.end(); ++itField )
		tags.push_back( GetTags_PathField().FormatUi( *itField ) );

	m_orderText = str::Join( tags, _T("|") );
}

bool CPathSortOrder::ParseOrderText( void )
{
	std::vector< PathField > newFields;

	std::vector< std::tstring > tags;
	str::Split( tags, m_orderText.c_str(), _T("|") );

	for ( std::vector< std::tstring >::const_iterator itTag = tags.begin(); itTag != tags.end(); ++itTag )
	{
		PathField field;

		if ( GetTags_PathField().ParseUiAs( field, *itTag ) )
			newFields.push_back( field );
		else
			return false;
	}

	m_fields.swap( newFields );
	return true;
}

const CEnumTags& CPathSortOrder::GetTags_PathField( void )
{
	static const CEnumTags s_tags( _T("Drive|Dir|Name|Ext|DirName|FullPath|DirPath|DirNameExt|NameExt|CoreExt") );
	return s_tags;
}
