#ifndef PathGroup_h
#define PathGroup_h
#pragma once

#include "Path.h"


namespace fs
{
	class CPathGroup
	{
	public:
		CPathGroup( void ) {}

		void Clear( void );

		bool IsEmpty( void ) const { return GetSpecs().empty(); }
		const std::vector<std::tstring>& GetSpecs( void ) const { return m_specs; }
		const std::vector<fs::CPath>& GetPaths( void ) const { return m_paths; }

		bool Contains( const fs::CPath& path ) const;
		bool AnyIsPartOf( const fs::CPath& path ) const;		// any of m_paths is sub-path of path?

		// specs
		std::tstring Join( const TCHAR sep[] = s_sep ) const;
		void Split( const TCHAR flat[], const TCHAR delims[] = s_sep );

		void Append( const CPathGroup& right );
		CPathGroup& operator+=( const CPathGroup& right ) { Append( right ); return *this; }
		CPathGroup operator+( const CPathGroup& right ) const { CPathGroup pathGroup = *this; return pathGroup += right; }

		void AddExpanded( const TCHAR envVarName[] );
	private:
		void ExpandPaths( void );
	private:
		std::vector<std::tstring> m_specs;			// path specifiers could contain environment variables
		std::vector<fs::CPath> m_paths;				// expanded environment variables - we could potentially have more m_paths then m_specs, for multi-path environment variables
	public:
		static const TCHAR s_sep[];
	};


	typedef CPathGroup TFilePathGroup;
}


#endif // PathGroup_h
