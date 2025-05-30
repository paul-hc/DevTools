#ifndef FileOperation_h
#define FileOperation_h
#pragma once

#include "utl/FlexPath.h"
#include "utl/FileSystem.h"
#include "utl/ErrorHandler.h"


class CEnumTags;
class CLogger;


class CFileOperation : public CErrorHandler
{
public:
	CFileOperation( utl::ErrorHandling handlingMode = utl::CheckMode ) : CErrorHandler( handlingMode ) {}

	bool Copy( const fs::CFlexPath& srcFilePath, const fs::CFlexPath& destFilePath ) throws_( CException* );
	bool Move( const fs::CFlexPath& srcFilePath, const fs::CFlexPath& destFilePath ) throws_( CException* );
	bool Delete( const fs::CFlexPath& filePath ) throws_( CException* );

	const std::vector<std::tstring>& GetLogLines( void ) const { return m_logLines; }
	void ClearLog( void ) { m_logLines.clear(); }
	void Log( CLogger& rLogger );
private:
	enum Operation { CopyFile, MoveFile, DeleteFile };
	static const CEnumTags& GetTags_Operation( void );

	void AddLogMessage( Operation operation, const fs::CPath& srcFilePath, const fs::CPath* pDestFilePath = nullptr );
	void AugmentLogError( const std::tstring& errorMessage );

	bool HandleError( CException* pExc );
	bool HandleError( const std::tstring& errorMessage );
	bool HandleLastError( void );
private:
	std::vector<std::tstring> m_logLines;
};


namespace svc
{
	size_t CopyFiles( const std::vector<fs::CFlexPath>& srcFilePaths, const std::vector<fs::CPath>& destFilePaths, RecursionDepth destDepth = Shallow );
	size_t RelocateFiles( const std::vector<fs::CFlexPath>& srcFilePaths, const std::vector<fs::CPath>& destFilePaths, RecursionDepth destDepth = Shallow );		// mix: MOVE physical, COPY complex


	bool PickDestImagePaths( std::vector<fs::CPath>& rDestFilePaths, const std::vector<fs::CFlexPath>& srcFilePaths );
	bool CheckOverrideExistingFiles( const std::vector<fs::CPath> destFilePaths, const TCHAR* pTitle = nullptr );


	void MakeDestFilePaths( std::vector<fs::CPath>& rDestFilePaths, const std::vector<fs::CFlexPath>& srcFilePaths, const fs::CPath& destFolderPath, RecursionDepth destDepth = Shallow );
}


#endif // FileOperation_h
