#ifndef FlagTags_h
#define FlagTags_h
#pragma once


#define FLAG_TAG( flag )  flag, _T(#flag)
	// pair to be used in static initializer lists: { MyFlag, _T("MyFlag") }


// formatter of bit field flags with values in sequence: 2^0, 2^1, ... 2^31

class CFlagTags
{
public:
	struct FlagDef
	{
		int m_flag;
		const TCHAR* m_pKeyTag;
	};

	// for contiguous flags starting with 0b1
	CFlagTags( const std::tstring& uiTags, const TCHAR* pKeyTags = NULL );

	// for sparse individual flags; uiTags follows the same order
	CFlagTags( const FlagDef flagDefs[], unsigned int count, const std::tstring& uiTags = std::tstring() );

	~CFlagTags();

	int GetFlagsMask( void ) const;
	const std::vector<std::tstring>& GetKeyTags( void ) const { return m_keyTags; }
	const std::vector<std::tstring>& GetUiTags( void ) const { return m_uiTags; }

	std::tstring FormatKey( int flags, const TCHAR* pSep = m_tagSep ) const { return Format( flags, m_keyTags, pSep ); }
	void ParseKey( int* pFlags, const std::tstring& text, const TCHAR* pSep = m_tagSep ) const { Parse( pFlags, text, m_keyTags, pSep, str::Case ); }

	std::tstring FormatUi( int flags, const TCHAR* pSep = m_tagSep ) const { return Format( flags, m_uiTags, pSep ); }
	void ParseUi( int* pFlags, const std::tstring& text, const TCHAR* pSep = m_tagSep ) const { Parse( pFlags, text, m_uiTags, pSep, str::IgnoreCase ); }

	enum TagType { KeyTag, UiTag };

	const std::tstring& LookupTag( TagType tag, int flag ) const;
	int FindFlag( TagType tag, const std::tstring& flagOn ) const;

	static int FindBitPos( unsigned int flag );
private:
	int LookupBitPos( unsigned int flag ) const { int pos = FindBitPos( flag ); ENSURE( pos < (int)m_uiTags.size() ); return pos; }

	static std::tstring Format( int flags, const std::vector<std::tstring>& tags, const TCHAR* pSep );
	static void Parse( int* pFlags, const std::tstring& text, const std::vector<std::tstring>& tags, const TCHAR* pSep, str::CaseType caseType );
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


#endif // FlagTags_h
