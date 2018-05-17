#ifndef EnumTags_h
#define EnumTags_h
#pragma once


class CEnumTags
{
public:
	CEnumTags( const std::tstring& uiTags, const TCHAR* pKeyTags = NULL, int defaultValue = -1 );
	~CEnumTags();

	const std::vector< std::tstring >& GetUiTags( void ) const { return m_uiTags; }

	std::tstring FormatUi( int value ) const { return Format( value, m_uiTags ); }
	int ParseUi( const std::tstring& text ) const { int value; Parse( value, text, m_uiTags ); return value; }

	std::tstring FormatKey( int value ) const { return Format( value, m_keyTags ); }
	int ParseKey( const std::tstring& text ) const { int value; Parse( value, text, m_keyTags ); return value; }

	template< typename EnumType >
	bool ParseUiAs( EnumType& rValue, const std::tstring& text ) const { return Parse( (int&)rValue, text, m_uiTags ); }

	template< typename EnumType >
	bool ParseKeyAs( EnumType& rValue, const std::tstring& text ) const { return Parse( (int&)rValue, text, m_keyTags ); }

	enum TagType { KeyTag, UiTag };

	const std::tstring& LookupTag( TagType tag, int flag ) const;

	template< typename EnumType >
	EnumType GetDefaultValue( void ) const
	{
		return static_cast< EnumType >( m_defaultValue );
	}
private:
	void Construct( const std::tstring& uiTags, const TCHAR* pKeyTags );
	bool Parse( int& rValue, const std::tstring& text, const std::vector< std::tstring >& tags ) const;
	static std::tstring Format( int value, const std::vector< std::tstring >& tags );
	static bool Contains( const std::vector< std::tstring >& strings, const std::tstring& value );
private:
	const int m_defaultValue;
	std::vector< std::tstring > m_uiTags;
	std::vector< std::tstring > m_keyTags;		// registry tags (not mandatory)
public:
	static const TCHAR m_listSep[];
	static const TCHAR m_tagSep[];
};


#endif // EnumTags_h
