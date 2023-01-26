#ifndef ValueRepository_h
#define ValueRepository_h
#pragma once

#include <unordered_map>


struct CValueStore;


struct CValueInfo
{
	std::tstring Format( void ) const;
public:
	short m_value;
	const TCHAR* m_pTag;
	const CValueStore* m_pStore;		// back pointer
};


struct CValueStore
{
	CValueStore( const TCHAR* pName, CValueInfo valueInfos[], unsigned int count );

	const std::tstring& GetName( void ) const { return m_name; }
	const std::vector< const CValueInfo* >& GetValues( void ) const { return m_valueInfos; }
private:
	std::tstring m_name;
	std::vector< const CValueInfo* > m_valueInfos;
};


class CIdentRepository
{
	CIdentRepository( void );
	~CIdentRepository();
public:
	static const CIdentRepository& Instance( void );

	const CValueInfo* FindValue( int value ) const;
public:
	std::vector< CValueStore* > m_stores;
	mutable std::unordered_map< int, const CValueInfo* > m_idToValuesMap;
};


#endif // ValueRepository_h
