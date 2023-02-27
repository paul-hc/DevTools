/////
#ifndef _CodeSamples_h
#define _CodeSamples_h
#pragma once

#include <vector>
#include "FileType.h"
#include "utl/Path.h"
#include <vector>


	CString GetWindowText( const CWnd* pWnd );			// comment
	void SetWindowText( CWnd* pWnd, const TCHAR* pText );		a = 5;

	CString GetControlText( const CWnd* pWnd, UINT ctrlId );
	void SetControlText( CWnd* pWnd, UINT ctrlId, const TCHAR* pText );

	HICON smartLoadIcon( LPCTSTR iconID, bool asLarge = true, HINSTANCE hResInst = AfxGetResourceHandle() );

	inline HICON smartLoadIcon( UINT iconId, bool asLarge = true, HINSTANCE hResInst = AfxGetResourceHandle() )
	{
		return smartLoadIcon( MAKEINTRESOURCE( iconId ), asLarge, hResInst );
	}

	bool smartEnableWindow( CWnd* pWnd, bool enable = true );
	void smartEnableControls( CWnd* pDlg, const UINT* pCtrlIds, size_t ctrlCount, bool enable = true );

	void commitMenuEnabling( CWnd& targetWnd, CMenu& popupMenu );
	void updateControlsUI( CWnd& targetWnd );
	CString GetCommandText( CCmdUI& rCmdUI );


	enum HorzAlign { Horz_NoAlign, Horz_AlignLeft, Horz_AlignCenter, Horz_AlignRight };
	enum VertAlign { Vert_NoAlign, Vert_AlignTop, Vert_AlignCenter, Vert_AlignBottom };

	CRect& alignRect( CRect& restDest, const CRect& rectFixed, HorzAlign horzAlign = Horz_AlignCenter,
					  VertAlign vertAlign = Vert_AlignCenter, bool limitDest = false );

	CRect& centerRect( CRect& restDest, const CRect& rectFixed, bool horizontally = true, bool vertically = true,
					   bool limitDest = false, const CSize& addOffset = CSize( 0, 0 ) );


template< typename Type, typename Ownership >
class CAssociation : public CRelationship
{
	CAssociation( void );
	CAssociation( const CAssociation& rRight );
	virtual ~CAssociation();

	CAssociation& operator=( const CAssociation& rRight );

	bool IsEmpty( void ) const;

	void Clear( void );
	bool BuildIndexes( bool onId, bool useHashIndex = true, const CString& typeClass = RUNTIME_CLASS( "MyClass" ) );
	bool BuildIndexes( bool onId, bool useHashIndex = true, const CString& typeClass = RUNTIME_CLASS( "MyClass" ) );
	bool BuildIndexes( bool onId, bool useHashIndex = true, const CString& typeClass = RUNTIME_CLASS( "MyClass" ) );
	bool BuildIndexes( bool onId, bool useHashIndex = true, const CString& typeClass = RUNTIME_CLASS( "MyClass" ) );
};

	line abc
	line 754
	line 3
	line 123


template 2
typename 3
void 4

	CString implementMethod( const TCHAR* methodPrototype, const TCHAR* templateDecl, const TCHAR* typeQualifier,
							 bool isInline );
	CString InputDocTypeDescriptor( const TCHAR* methodPrototype, const TCHAR* templateDecl,
									const TCHAR* typeQualifier = _T("text"), bool isInline,
									CString& templateDecl, CString& typeQualifier,
									const TCHAR* typeDescriptor );
	CString inputDocTypeDescriptor( const TCHAR* methodPrototype, const TCHAR* templateDecl, const TCHAR* typeQualifier, bool isInline, CString& templateDecl, CString& typeQualifier, const TCHAR* typeDescriptor );



/**
	CAssociation< Type, Ownership > implementation
*/

template< typename Type, typename Ownership >
CAssociation< Type, Ownership >::CAssociation( void )
{
}

template< typename Type, typename Ownership >
inline void CAssociation< Type, Ownership >::Clear( void )
{
}

inputDocTypeDescriptor


	bool hasPrompt( int flags, bool strict = false ) const;


class LineParser
{
public:
	LineParser( void );
	LineParser( const LineParser& src );
	LineParser( CString src );
	~LineParser();

	LineParser& operator=( const LineParser& src );

	bool operator==( const LineParser& cmp ) const;

	bool hasPrompt( int flags, bool strict = false ) const;
	bool hasSamePromptFile( const LineParser& cmp ) const;

	void reset( void );
public:
	static const LPCTSTR m_warning;
	static const LPCTSTR m_error;
};



/**
	LineParser inline code
*/

inline CString& LineParser::refLine( void ) const
{
	return const_cast< CString& >( m_line );
}


#endif // _CodeSamples_h



<?xml version=_T("1.0") encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
	<noInherit/>
	<assemblyIdentity processorArchitecture="*"
					  type="win32"
					  name="IDETools"
					  version="1.0.0.0"/>
	<description>IDETools COM objects</description>
	<dependency optional="yes">
		<dependentAssembly>
			<assemblyIdentity type="win32"
							  name="Microsoft.Windows.Common-Controls"
							  version="6.0.1.0"
							  publicKeyToken="6595b64144ccf1df"
							  language="*"
							  processorArchitecture="*"/>
		</dependentAssembly>
	</dependency>
</assembly>
