#ifndef XferOptions_h
#define XferOptions_h
#pragma once

#include <vector>
#include "utl/Path.h"


class CRuntimeException;
struct CTransferItem;

enum FileAction;
enum TransferMode { ExecuteTransfer, JustDisplaySourceFile, JustDisplayTargetFile };
enum UserPrompt { PromptOnIssues, PromptAlways, PromptNever };
enum CheckFileChanges { NoCheck, CheckTimestamp, CheckFileSize, CheckFullContent };


struct CXferOptions
{
	CXferOptions( void );
	~CXferOptions();

	bool PassFilter( const CTransferItem& transferNode ) const;

	void ParseCommandLine( int argc, TCHAR* argv[] ) throws_( CRuntimeException );
private:
	enum CaseCvt { AsIs, UpperCase, LowerCase };

	static bool ParseValue( std::tstring& rValue, const TCHAR* pArg, const TCHAR* pNameList, CaseCvt caseCvt = AsIs );

	void ParseFileAction( const std::tstring& value ) throws_( CRuntimeException );
	void ParseFileChangesFilter( const std::tstring& value ) throws_( CRuntimeException );
	void ParseFileAttributes( const std::tstring& value ) throws_( CRuntimeException );
	void ParseTimestamp( const std::tstring& value ) throws_( CRuntimeException );

	void PostProcessArguments( void ) throws_( CRuntimeException );

	void ThrowInvalidArgument( void ) throws_( CRuntimeException );
private:
	const TCHAR* m_pArg;								// current argument parsed
public:
	bool m_helpMode;
	bool m_recurseSubDirectories;
	fs::CPath m_sourceDirPath;
	fs::CPath m_targetDirPath;
	std::tstring m_searchSpecs;
	std::tstring m_excludeWildSpec;						// multiple list separated by ';' or ','
	std::vector< std::tstring > m_excludeFindSpecs;
	std::auto_ptr<fs::CPath> m_pBackupDirPath;

	BYTE m_mustHaveFileAttr;
	BYTE m_mustNotHaveFileAttr;

	CheckFileChanges m_filterBy;
	CTime m_earliestTimestamp;

	bool m_justCreateTargetDirs;
	bool m_overrideReadOnlyFiles;
	bool m_transferOnlyExistentTargetFiles;
	bool m_transferOnlyToExistentTargetDirs;

	bool m_displayFileNames;

	FileAction m_fileAction;
	TransferMode m_transferMode;
	UserPrompt m_userPrompt;
public:
	static const TCHAR m_specDelims[];
};


#endif // XferOptions_h
