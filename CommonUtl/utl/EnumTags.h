#ifndef EnumTags_h
#define EnumTags_h
#pragma once


class CEnumTags
{
public:
	CEnumTags( const std::tstring& uiTags, const TCHAR* pKeyTags = NULL, int defaultValue = -1, int baseValue = 0 );
	~CEnumTags();

	const std::vector< std::tstring >& GetUiTags( void ) const { return m_uiTags; }

	std::tstring FormatUi( int value ) const { return _Format( value, m_uiTags ); }
	int ParseUi( const std::tstring& text ) const { int value; _Parse( value, text, m_uiTags ); return value; }

	std::tstring FormatKey( int value ) const { return _Format( value, m_keyTags ); }
	int ParseKey( const std::tstring& text ) const { int value; _Parse( value, text, m_keyTags ); return value; }

	template< typename EnumType >
	bool ParseUiAs( EnumType& rValue, const std::tstring& text ) const { return _Parse( (int&)rValue, text, m_uiTags ); }

	template< typename EnumType >
	bool ParseKeyAs( EnumType& rValue, const std::tstring& text ) const { return _Parse( (int&)rValue, text, m_keyTags ); }

	enum TagType { UiTag, KeyTag };

	std::tstring Format( int value, TagType tagType ) const { return _Format( value, UiTag == tagType ? m_uiTags : m_keyTags ); }

	template< typename EnumType >
	bool ParseAs( EnumType& rValue, const std::tstring& text ) const { return UiTag == tagType ? ParseUiAs< EnumType >( rValue, text ) : ParseKeyAs< EnumType >( rValue, text ); }

	const std::tstring& LookupTag( TagType tag, int flag ) const;

	template< typename EnumType >
	EnumType GetDefaultValue( void ) const
	{
		return static_cast< EnumType >( m_defaultValue );
	}
private:
	void Construct( const std::tstring& uiTags, const TCHAR* pKeyTags );
	size_t TagIndex( int value, const std::vector< std::tstring >& tags ) const;
	bool _Parse( int& rValue, const std::tstring& text, const std::vector< std::tstring >& tags ) const;

	std::tstring _Format( int value, const std::vector< std::tstring >& tags ) const;
	static bool Contains( const std::vector< std::tstring >& strings, const std::tstring& value );
private:
	const int m_defaultValue;
	const int m_baseValue;						// translate to 0-based tag index
	std::vector< std::tstring > m_uiTags;
	std::vector< std::tstring > m_keyTags;		// registry tags (not mandatory)
public:
	static const TCHAR m_listSep[];
	static const TCHAR m_tagSep[];
};


#endif // EnumTags_h
