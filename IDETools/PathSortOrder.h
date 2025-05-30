#ifndef PathSortOrder_h
#define PathSortOrder_h
#pragma once

#include "PublicEnums.h"
#include "utl/Path.h"
#include <unordered_map>


namespace fs
{
	struct CPathSortParts : public CPathParts, private utl::noncopyable
	{
		CPathSortParts( const CPath& filePath );
	public:
		const CPath m_parentDirPath;
		const CPath m_parentDirName;
		const CPath m_pathExclExt;			// original path with no extension
	};


	class CExtCustomOrder : private utl::noncopyable
	{
		CExtCustomOrder( void ) {}
	public:
		static CExtCustomOrder& Instance( void );

		static pred::CompareResult CompareExtension( const std::tstring& leftExt, const std::tstring& rightExt );

		void RegisterCustomOrder( const TCHAR extOrderList[] = s_defaultExtOrder, const TCHAR sep[] = _T("|") );		// list of extensions
		UINT LookupExtOrder( const std::tstring& ext ) const;
	private:
		std::unordered_map<CPath, UINT> m_extToOrderMap;		// use fs::CPath for natural compare
		static const TCHAR s_defaultExtOrder[];
	};
}


namespace pred
{
	CompareResult ComparePathField( const fs::CPathSortParts& left, const fs::CPathSortParts& right, PathField pathField );
}


class CEnumTags;


class CPathSortOrder
{
public:
	CPathSortOrder( void ) { ResetDefaultOrder(); }

	static const std::vector<PathField>& GetDefaultOrder( void );

	bool IsEmpty( void ) const { return m_fields.empty(); }
	size_t FindFieldPos( PathField field ) const;

	const std::vector<PathField>& GetFields( void ) const { return m_fields; }
	void SetFields( const std::vector<PathField>& fields ) { m_fields = fields; StoreOrderText(); }

	bool IsDefaultOrder( void ) const { return m_fields == GetDefaultOrder(); }
	void ResetDefaultOrder( void ) { SetFields( GetDefaultOrder() ); }

	void Clear( void );
	void Add( PathField field );
	void Toggle( PathField field );

	std::tstring* GetOrderTextPtr( void ) { return &m_orderText; }
	const std::tstring& GetOrderText( void ) const { return m_orderText; }
	bool SetOrderText( const std::tstring& orderText ) { m_orderText = orderText; return ParseOrderText(); }
	bool ParseOrderText( void );
private:
	void StoreOrderText( void );

	static const CEnumTags& GetTags_PathField( void );
private:
	std::vector<PathField> m_fields;
	std::tstring m_orderText;
};


#endif // PathSortOrder_h
