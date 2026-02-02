#ifndef FlagTags_h
#define FlagTags_h
#pragma once


#define FLAG_TAG( flag )  flag, _T(#flag)		// pair to be used in static initializer lists: { MyFlag, _T("MyFlag") }


// formatter of bit field flags with values in sequence: 2^0, 2^1, ... 2^31
//
class CFlagTags
{
public:
	struct FlagDef
	{
		unsigned long m_flag;
		const TCHAR* m_pKeyTag;
	};

	// for contiguous flags starting with 0b1
	CFlagTags( const std::tstring& uiTags, const TCHAR* pKeyTags = nullptr );

	// for sparse individual flags; uiTags follows the same order
	CFlagTags( const FlagDef flagDefs[], unsigned int count, const std::tstring& uiTags = str::GetEmpty() );

	~CFlagTags();

	int GetFlagsMask( void ) const;
	const std::vector<std::tstring>& GetKeyTags( void ) const { return m_keyTags; }
	const std::vector<std::tstring>& GetUiTags( void ) const { return m_uiTags; }

	std::tstring FormatKey( int flags, const TCHAR* pSep = m_tagSep ) const { return Format( flags, m_keyTags, pSep ); }
	void ParseKey( OUT int* pFlags, const std::tstring& text, const TCHAR* pSep = m_tagSep ) const { Parse( pFlags, text, m_keyTags, pSep, str::Case ); }

	std::tstring FormatUi( int flags, const TCHAR* pSep = m_tagSep ) const { return Format( flags, m_uiTags, pSep ); }
	void ParseUi( OUT int* pFlags, const std::tstring& text, const TCHAR* pSep = m_tagSep ) const { Parse( pFlags, text, m_uiTags, pSep, str::IgnoreCase ); }

	enum TagType { KeyTag, UiTag };

	const std::tstring& LookupTag( TagType tag, int flag ) const;
	int FindFlag( TagType tag, const std::tstring& flagOn ) const;

	static int FindBitPos( unsigned int flag );
private:
	int LookupBitPos( unsigned int flag ) const { int pos = FindBitPos( flag ); ENSURE( pos < (int)m_uiTags.size() ); return pos; }

	static std::tstring Format( int flags, const std::vector<std::tstring>& tags, const TCHAR* pSep );
	static void Parse( OUT int* pFlags, const std::tstring& text, const std::vector<std::tstring>& tags, const TCHAR* pSep, str::CaseType caseType );
	static bool Contains( const std::vector<std::tstring>& strings, const std::tstring& value, str::CaseType caseType );
private:
	enum { MaxBits = 8 * sizeof( int ) };

	// indexes are powers of 2 of corresponding flags
	std::vector<std::tstring> m_keyTags;		// e.g. xml tags (not mandatory)
	std::vector<std::tstring> m_uiTags;
public:
	static const TCHAR m_listSep[];
	static const TCHAR m_tagSep[];
};


#include <unordered_map>

#define VALUE_TAG( value )  value, _T(#value)		// pair to be used in static initializer lists: { MyValue, _T("MyFlag") }


// formatter of value with tags
//
class CValueTags
{
public:
	struct ValueDef
	{
		long m_value;
		const TCHAR* m_pKeyTag;
	};

	CValueTags( const ValueDef valueDefs[], unsigned int count, const TCHAR* pUiTags = nullptr );

	const std::tstring& FormatKey( long value ) const { return Format( value, KeyTag ); }
	const std::tstring& FormatUi( long value ) const { return Format( value, UiTag ); }

	template< typename ValueT >
	bool ParseKey( OUT ValueT* pValue, const std::tstring& text ) const
	{
		long value = 0;
		if ( !Parse( &value, text, KeyTag ) )
			return false;

		*pValue = static_cast<ValueT>( value );
		return true;
	}

	template< typename ValueT >
	bool ParseUi( OUT ValueT* pValue, const std::tstring& text ) const
	{
		long value = 0;
		if ( !Parse( &value, text, UiTag ) )
			return false;

		*pValue = static_cast<ValueT>( value );
		return true;
	}
private:
	typedef std::pair<std::tstring, std::tstring> TKeyUiTags;
	enum TagType { KeyTag, UiTag };

	const TKeyUiTags& LookupTags( long value ) const;
	const std::tstring& Format( long value, TagType tagType ) const;
	bool Parse( OUT long* pValue, const std::tstring& text, TagType tagType ) const;
private:
	std::unordered_map<long, TKeyUiTags> m_valueTags;
};


#endif // FlagTags_h
