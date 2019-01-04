#ifndef FilterStore_h
#define FilterStore_h
#pragma once

#include "Path.h"
#include "ShellDialogs_fwd.h"
#include <hash_map>


namespace fs
{
	typedef std::pair< std::tstring, std::tstring > FilterPair;		// <tag, specs> - e.g. <"BMP Decoder", "*.bmp;*.dib;*.rle">


	// repository of extensions mapped to filter entries
	//
	class CKnownExtensions : private utl::noncopyable
	{
	public:
		CKnownExtensions( void ) {}

		// specs
		bool IsEmpty( void ) const { return m_knownExts.empty(); }
		const stdext::hash_map< CPath, size_t >& GetMap( void ) const { return m_knownExts; }
		const std::tstring& GetAllSpecs( void ) const { return m_allSpecs; }
		const std::tstring& GetDefaultExt( void ) const { return m_defaultExt; }

		// extensions
		size_t FindExtFilterPos( const TCHAR* pFilePath ) const;
		bool ContainsExt( const TCHAR* pFilePath ) const { return FindExtFilterPos( pFilePath ) != utl::npos; }
		std::tstring MakeAllExts( void ) const;
		void QueryAllExts( std::vector< std::tstring >& rAllExts ) const;

		void Register( const std::vector< std::tstring >& exts, size_t filterPos );
		void Register( const std::tstring& item, size_t filterPos );			// either ext ".bmp" or spec "*.bmp"
		void RegisterSpecs( const std::tstring& specs, size_t filterPos );		// either ".bmp;.dib" or "*.bmp;*.dib"
	private:
		stdext::hash_map< CPath, size_t > m_knownExts;				// known extensions to a filter entry pos (CFilterStore::m_filters)
		std::tstring m_allSpecs;
		std::tstring m_defaultExt;
	};


	enum BrowseFlags
	{
		BrowseOpen		= BIT_FLAG( 0 ),
		BrowseSave		= BIT_FLAG( 1 ),
		BrowseBoth		= BrowseOpen | BrowseSave,
	};


	// store of file filters to pass to CFileDialog and manager of know extensions
	//
	class CFilterStore : private utl::noncopyable
	{
	public:
		CFilterStore( const std::tstring& classTag, BrowseFlags browseFlags = BrowseBoth );
		~CFilterStore();

		const std::tstring& GetClassTag( void ) const { return m_classTag; }						// name of all known file types
		const std::tstring& GetClassSpecs( void ) const { return m_knownExts.GetAllSpecs(); }		// specs for all known extensions

		const std::vector< FilterPair >& GetFilters( void ) const { return m_filters; }
		const CKnownExtensions& GetKnownExtensions( void ) const { return m_knownExts; }

		void AddFilter( const FilterPair& filter );
		void AddFilter( const COMDLG_FILTERSPEC& filterSpec ) { AddFilter( FilterPair( filterSpec.pszName, filterSpec.pszSpec ) ); }

		// file dialog formatting
		void StreamFilters( std::tostringstream& oss ) const;
		void StreamFilters( std::tostringstream& oss, const UINT positions[], size_t count ) const;
		void StreamClassFilter( std::tostringstream& oss ) const { StreamFilter( oss, FilterPair( m_classTag, GetClassSpecs() ) ); }
		static void StreamAllFiles( std::tostringstream& oss ) { StreamFilter( oss, s_allFiles ); }
		static void StreamArrayEnd( std::tostringstream& oss ) { oss << s_filterSep; }
		static void StreamFilter( std::tostringstream& oss, const FilterPair& filter );

		// file dialog filters
		std::tstring MakeFilters( void ) const;
		std::tstring MakeFilters( const UINT positions[], size_t count ) const;		// subset of filters for file dialog
	private:
		std::tstring m_classTag;					// unique tag of this filter class (displayed as all known file types)
		BrowseFlags m_browseFlags;
		std::vector< FilterPair > m_filters;
		CKnownExtensions m_knownExts;
	public:
		static const FilterPair s_allFiles;
		static const TCHAR s_specSep[];
		static const TCHAR s_filterSep[];
	};


	// joins multiple file filter stores for passing to CFileDialog and managing know extensions
	//
	class CFilterJoiner
	{
	public:
		CFilterJoiner( void ) {}
		CFilterJoiner( const CFilterStore& singleStore ) { Add( singleStore.GetClassTag() ); }

		void Add( const std::tstring& classTag ) { m_classTags.push_back( classTag ); }

		bool IsEmpty( void ) const { return m_classTags.empty(); }
		std::tstring MakeFilters( shell::BrowseMode browseMode ) const;
		std::tstring MakeSpecs( shell::BrowseMode browseMode ) const;			// joined specs for all known extensions

		bool BrowseFile( std::tstring& rFilePath, shell::BrowseMode browseMode,
						 DWORD flags = 0, const TCHAR* pDefaultExt = NULL,
						 CWnd* pParentWnd = NULL, const TCHAR* pTitle = NULL ) const;

		// selected filter persistence (1-based)
		std::tstring RetrieveSelFilterSpec( void ) const;
		void StoreSelFilterSpec( const TCHAR* pSelFilterSpec ) const;
		void StoreSelFilterSpec( const COMDLG_FILTERSPEC& selSpec ) const { StoreSelFilterSpec( selSpec.pszSpec ); }
	private:
		const std::tstring& GetKey( void ) const;

		static std::tstring FormatHashKey( const std::tstring& key );
		static UINT HashKey( const std::tstring& key ) { return static_cast< UINT >( stdext::hash_value( key.c_str() ) ); }
	private:
		std::vector< std::tstring > m_classTags;		// for filters
		mutable std::tstring m_key;						// self-encapsulated concatenation of class tags
		static const TCHAR s_regSection[];				// section where last selected filter index is persisted
	};


	class CFilterRepository : private utl::noncopyable
	{
		CFilterRepository( void ) {}
	public:
		static CFilterRepository& Instance( void );

		CFilterStore* Lookup( const std::tstring& classTag, shell::BrowseMode browseMode ) const;

		void Register( CFilterStore* pFilterStore, BrowseFlags browseFlags );
		void Unregister( CFilterStore* pFilterStore, BrowseFlags browseFlags );
	private:
		typedef std::pair< CFilterStore*, CFilterStore* > OpenSavePair;			// <filter_open, filter_save>

		stdext::hash_map< std::tstring, OpenSavePair > m_stores;				// <class_tag, OpenSavePair>
	};

} //namespace fs


#endif // FilterStore_h
