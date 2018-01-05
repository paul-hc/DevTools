#ifndef XferOptions_h
#define XferOptions_h

#include <vector>


class CRuntimeException;
struct CTransferItem;

enum FileAction;
enum TransferMode { ExecuteTransfer, JustDisplaySourceFile, JustDisplayTargetFile };
enum UserPrompt { PromptOnIssues, PromptAlways, PromptNever };


struct CXferOptions
{
	CXferOptions( void );
	~CXferOptions();

	bool PassFilter( const CTransferItem& transferNode ) const;

	void ParseCommandLine( int argc, TCHAR* argv[] ) throws_( CRuntimeException );
	void CheckCreateTargetDirPath( void );
private:
	enum CaseCvt { PreserveCase, UpperCase, LowerCase };
	static bool ParseValue( std::tstring& rValue, const TCHAR* pArg, const TCHAR* pNameList, CaseCvt caseCvt = PreserveCase );

	void ParseFileAction( const std::tstring& value ) throws_( CRuntimeException );
	void ParseFileAttributes( const std::tstring& value ) throws_( CRuntimeException );
	void ParseTimestamp( const std::tstring& value ) throws_( CRuntimeException );

	void PostProcessArguments( void ) throws_( CRuntimeException );

	void ThrowInvalidArgument( void ) throws_( CRuntimeException );
private:
	const TCHAR* m_pArg;								// current argument parsed
public:
	bool m_recurseSubDirectories;
	std::tstring m_sourceDirPath;
	std::tstring m_targetDirPath;
	std::tstring m_searchSpecs;
	std::tstring m_excludeWildSpec;						// multiple list separated by ';' or ','
	std::vector< std::tstring > m_excludeFindSpecs;

	DWORD m_mustHaveFileAttr;
	DWORD m_mustNotHaveFileAttr;

	bool m_filterByTimestamp;
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
