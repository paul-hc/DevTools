
#include "pch.h"
#include "ComUtils.h"
#include "utl/Path.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace com
{
	struct CPropKeyPtrName	// for efficient sorting, by-ref
	{
		CPropKeyPtrName( void ) : m_pPropKey( nullptr ) {}
		CPropKeyPtrName( const PROPERTYKEY& propKey )
			: m_pPropKey( &propKey )
			, m_canonicalName( com::GetPropCanonicalName( propKey ) )
		{
		}

		const PROPERTYKEY& GetKey( void ) const { ASSERT_PTR( m_pPropKey ); return *m_pPropKey; }
	public:
		const PROPERTYKEY* m_pPropKey;
		std::tstring m_canonicalName;
	};


	// IPropertyStore utils

	CComPtr<IPropertyStore> OpenFileProperties( const TCHAR* pFilePath, GETPROPERTYSTOREFLAGS flags /*= GPS_DEFAULT*/ )
	{
		CComPtr<IPropertyStore> pPropertyStore;

		if ( !HR_OK( ::SHGetPropertyStoreFromParsingName( pFilePath, NULL, flags, IID_PPV_ARGS( &pPropertyStore ) ) ) )		// use GPS_DEFAULT or GPS_BESTEFFORT
			return nullptr;

		return pPropertyStore;
	}

	template< typename PropKeyT >
	bool QueryPropertiesImpl( OUT std::vector<PropKeyT>& rPropKeys, IPropertyStore* pPropertyStore, bool sortByCanName )
	{
		ASSERT_PTR( pPropertyStore );

		DWORD propCount = 0;

		rPropKeys.clear();
		if ( !HR_OK( pPropertyStore->GetCount( &propCount ) ) )
			return false;

		rPropKeys.reserve( propCount );

		for ( DWORD i = 0; i != propCount; ++i )
		{
			// Get the property key at a given index.
			PROPERTYKEY key;
			if ( HR_OK( pPropertyStore->GetAt( i, &key ) ) )
				rPropKeys.push_back( key );		// for com::CPropKeyName, rely on implicit conversion
		}

		if ( sortByCanName )
			SortProperties( rPropKeys );

		return true;
	}

	bool QueryProperties( OUT std::vector<PROPERTYKEY>& rPropKeys, IPropertyStore* pPropertyStore, bool sortByCanName /*= true*/ )
	{
		return QueryPropertiesImpl( rPropKeys, pPropertyStore, sortByCanName );
	}

	bool QueryProperties( OUT std::vector<CPropKeyName>& rPropKeyNames, IPropertyStore* pPropertyStore, bool sortByCanName /*= true*/ )
	{
		return QueryPropertiesImpl( rPropKeyNames, pPropertyStore, sortByCanName );
	}

	void SortProperties( IN OUT std::vector<PROPERTYKEY>& rPropKeys )
	{
		std::vector<CPropKeyPtrName> orderedPropKeys;
		utl::transform( rPropKeys, orderedPropKeys, func::ToSelf() );

		utl::sort( orderedPropKeys, pred::TLess_PropKeyNameIntuitive() );

		std::vector<PROPERTYKEY> newPropKeys;
		utl::transform( orderedPropKeys, newPropKeys, std::mem_fn( &CPropKeyPtrName::GetKey ) );

		rPropKeys.swap( newPropKeys );		// 'return' the ordered property keys
	}

	void SortProperties( IN OUT std::vector<CPropKeyName>& rPropKeyNames )
	{
		utl::sort( rPropKeyNames, pred::TLess_PropKeyNameIntuitive() );
	}

	std::tstring GetPropCanonicalName( const PROPERTYKEY& propKey )
	{
		TCHAR* pCanonicalName = nullptr;
		std::tstring propCanonicalName;

		if ( HR_OK( ::PSGetNameFromPropertyKey( propKey, &pCanonicalName ) ) )
		{
			propCanonicalName = str::SafePtr( pCanonicalName );
			::CoTaskMemFree( pCanonicalName );
		}

		return propCanonicalName;
	}

	bool GetPropKeyFromCanonicalName( OUT PROPERTYKEY* pOutPropKey, const TCHAR* pCanonicalName )
	{
		ASSERT_PTR( pOutPropKey );
		PROPERTYKEY propKey;

		return HR_OK( ::PSGetPropertyKeyFromName( pCanonicalName, &propKey ) );
	}

	std::tstring FormatPropDisplayValue( const PROPERTYKEY& propKey, const PROPVARIANT& propValue, PROPDESC_FORMAT_FLAGS fmtFlags /*= PDFF_DEFAULT*/ )
	{
		TCHAR* pDisplayValue = nullptr;
		std::tstring displayValue;

		if ( HR_OK( ::PSFormatForDisplayAlloc( propKey, propValue, fmtFlags, &pDisplayValue ) ) )
		{
			displayValue = str::SafePtr( pDisplayValue );
			::CoTaskMemFree( pDisplayValue );
		}

		return displayValue;
	}

	CComPtr<IPropertyDescription> GetPropertyDescription( const TCHAR* pPropCanonicalName )
	{
		CComPtr<IPropertyDescription> pPropDescr;

		if ( !HR_OK( ::PSGetPropertyDescriptionByName( pPropCanonicalName, IID_PPV_ARGS( &pPropDescr ) ) ) )
			return nullptr;

		return pPropDescr;
	}

	std::tstring GetPropertyLabel( IPropertyDescription* pPropDescr )
	{
		ASSERT_PTR( pPropDescr );

		TCHAR* pPropertyLabel = nullptr;
		std::tstring propertyLabel;

		if ( HR_OK( pPropDescr->GetDisplayName( &pPropertyLabel ) ) )
		{
			propertyLabel = str::SafePtr( pPropertyLabel );
			::CoTaskMemFree( pPropertyLabel );
		}

		return propertyLabel;
	}
}


namespace com
{
	// CPropVariant implementation

	bool CPropVariant::GetBool( OUT bool* pValue ) const
	{
		ASSERT_PTR( pValue );
		BOOL value;
		if ( !HR_OK( ::PropVariantToBoolean( m_prop, &value ) ) )
			return false;

		*pValue = value != FALSE;
		return true;
	}

	bool CPropVariant::GetString( OUT std::tstring* pValue ) const
	{
		ASSERT_PTR( pValue );

		switch ( m_prop.vt )
		{
			case VT_EMPTY:
				pValue->clear();
				break;
			case VT_LPWSTR:
				*pValue = m_prop.pwszVal;
				break;
			case VT_BSTR:
				*pValue = m_prop.bstrVal;
				break;
			case VT_LPSTR:
				*pValue = str::FromUtf8( m_prop.pszVal );
				break;
			default:
				return false;
		}
		return true;
	}

	bool CPropVariant::QueryStrings( OUT std::vector<std::tstring>* pStrItems ) const
	{
		ASSERT_PTR( pStrItems );

		switch ( m_prop.vt & VT_TYPEMASK )
		{
			case VT_LPWSTR:
				if ( !HasFlag( m_prop.vt, VT_VECTOR ) )
					return false;
				break;
			case VT_BSTR:
				if ( !HasFlag( m_prop.vt, VT_VECTOR | VT_ARRAY ) )
					return false;
				break;
			case VT_EMPTY:
				pStrItems->clear();
				// fall-through
			default:
				return false;
		}

		PWSTR* pStringArray;
		ULONG cElem;

		// expected types: (VT_VECTOR | VT_LPWSTR), (VT_VECTOR | VT_BSTR) or (VT_ARRAY | VT_BSTR)
		if ( !HR_OK( PropVariantToStringVectorAlloc( m_prop, &pStringArray, &cElem ) ) )
			return false;

		// copy the allocated strings into the pStrItems
		pStrItems->reserve( cElem );

		for ( ULONG i = 0; i != cElem; ++i )
			pStrItems->push_back( str::SafePtr( pStringArray[i] ) );		// handle potential NULL BSTRs if necessary

		com::CoTaskMemFreeArray( pStringArray, cElem );
		return true;
	}

	bool CPropVariant::GetFilePath( OUT fs::CPath* pFilePath ) const
	{
		ASSERT_PTR( pFilePath );
		return GetString( &pFilePath->Ref() );
	}

	bool CPropVariant::QueryFilePaths( OUT std::vector<fs::CPath>* pOutFilePaths ) const
	{
		ASSERT_PTR( pOutFilePaths );

		std::vector<std::tstring> strItems;

		if ( !QueryStrings( &strItems ) )
			return false;

		pOutFilePaths->resize( strItems.size() );

		for ( size_t i = 0; i != strItems.size(); ++i )
			(*pOutFilePaths)[i].Ref().swap( strItems[i] );

		return true;
	}
}
