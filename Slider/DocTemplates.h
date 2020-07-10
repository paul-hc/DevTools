#ifndef DocTemplates_h
#define DocTemplates_h
#pragma once

#include "utl/UI/AccelTable.h"
#include "utl/UI/FilterStore.h"


class CAlbumDoc;
class CSearchPattern;
namespace fs { class CPath; }


namespace app
{
	// Manages multiple extensions and file filters (overrides CDocTemplate::filterExt field stored in idResource string).
	// Shares menu and accelerator resources (prevents destructor from destroying them).
	//
	class CSharedDocTemplate : public CMultiDocTemplate
	{
	protected:
		CSharedDocTemplate( UINT idResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass );
		virtual ~CSharedDocTemplate();

		void SetFilterStore( const fs::CFilterStore* pFilterStore );
	public:
		// base overrides
		virtual BOOL GetDocString( CString& rString, enum DocStringIndex index ) const;
		virtual Confidence MatchDocType( LPCTSTR pPathName, CDocument*& rpDocMatch );

		const fs::CFilterStore* GetFilterStore( void ) const { return m_pFilterStore; }
		const std::vector< std::tstring >& GetAllExts( void ) const { return m_allExts; }

		void RegisterAdditionalDocExtensions( void );
		bool PromptFileDialog( CString& rFilePath, UINT titleId, DWORD flags, shell::BrowseMode browseMode ) const;
	protected:
		virtual void AlterSaveAsPath( CString& rFilePath ) const;
	protected:
		const fs::CFilterStore* m_pFilterStore;			// for file filters and known extensions
		std::tstring m_fileFilters;
		std::tstring m_knownExts;
		std::vector< std::tstring > m_allExts;
		bool m_acceptDirPath;							// true: open document on directory path
	public:
		CMenu m_menu;
		CAccelTable m_accel;
		static bool s_useSingleFilterExt;
	};


	class CImageDocTemplate : public CSharedDocTemplate
	{
		CImageDocTemplate( void );
	public:
		static CImageDocTemplate* Instance( void );
	};


	class CAlbumDocTemplate : public CSharedDocTemplate
	{
		CAlbumDocTemplate( void );
	public:
		static CAlbumDocTemplate* Instance( void );

		enum OpenPathType { DirPath, SlideAlbum, CatalogStorageDoc, InvalidPath };

		static OpenPathType GetOpenPathType( const TCHAR* pPath );
		static bool IsSlideAlbumFile( const TCHAR* pFilePath );

		void RegisterAlbumShellDirectory( bool doRegister );
	protected:
		// base overrides
		virtual void AlterSaveAsPath( CString& rFilePath ) const;
	};


	class CDocManager : public ::CDocManager
	{
	public:
		CDocManager( void );

		// base overrides
		virtual void OnFileNew( void );
		virtual BOOL DoPromptFileName( CString& rFilePath, UINT titleId, DWORD flags, BOOL openDlg, CDocTemplate* pTemplate );
		virtual void RegisterShellFileTypes( BOOL compatMode );

		void RegisterImageAdditionalShellExt( bool doRegister );
	};

}


namespace app
{
	class CAlbumFilterStore : public fs::CFilterStore
	{
		CAlbumFilterStore( void );
	public:
		static CAlbumFilterStore& Instance( void );

		std::tstring MakeAlbumFilters( void ) const;
		std::tstring MakeCatalogStgFilters( void ) const;

		enum AlbumFilter { SlideFilter, CatalogStgFilter };
	};


	class CSliderFilters : public fs::CFilterJoiner		// images + albums
	{
		CSliderFilters( void );
	public:
		static CSliderFilters& Instance( void );
	};
}


#endif // DocTemplates_h
