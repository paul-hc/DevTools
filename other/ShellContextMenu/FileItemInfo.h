#ifndef FileItemInfo_h
#define FileItemInfo_h
#pragma once


struct CFileItemInfo
{
	CFileItemInfo( const std::tstring& fullPath, const CFileFind* pFoundFile, const TCHAR* pFilename = NULL );

	// factory methods
	static CFileItemInfo* MakeItem( const std::tstring& filePath );			// uses CFileFind to access file info
	static CFileItemInfo* MakeItem( const CFileFind& foundFile );
	static CFileItemInfo* MakeParentDirItem( const std::tstring& dirPath );

	bool IsValid( void ) const { return m_fileAttributes != UINT_MAX; }
	bool IsDirectory( void ) const { ASSERT( IsValid() ); return HasFlag( m_fileAttributes, FILE_ATTRIBUTE_DIRECTORY ); }

	struct CDetails;

	const CDetails& GetDetails( void ) const { m_details.LazyInit( this ); return m_details; }
public:
	struct CDetails
	{
		CDetails( void ) : m_iconIndex( -1 ) {}

		bool IsValid( void ) const { return m_iconIndex != -1; }
	private:
		void LazyInit( const CFileItemInfo* pItem );
		static std::tstring FormatAttributes( DWORD fileAttributes );

		friend struct CFileItemInfo;
	public:
		int m_iconIndex;
		std::tstring m_typeName;

		std::tstring m_fileAttributesText;
		std::tstring m_fileSizeText;
		std::tstring m_modifiedDateText;
	};
private:
	void StoreFilename( const TCHAR* pFilename, const CFileFind* pFoundFile );
public:
	std::tstring m_fullPath;
	std::tstring m_filename;
	DWORD m_fileAttributes;
	UINT64 m_fileSize;
	CTime m_modifiedTime;

	mutable CDetails m_details;
};


#endif // FileItemInfo_h
