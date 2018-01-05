
#include "stdafx.h"
#include "ContainerComponents.h"
#include "CodeUtilities.h"
#include "BraceParityStatus.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace code
{
	ContainerComponents::ContainerComponents( const TCHAR* pCodeText )
		: m_pCodeText( pCodeText )
		, m_length( str::Length( m_pCodeText ) )
		, m_leadingWhiteSpace( m_length )
		, m_containerType( m_length )
		, m_objectType( m_length )
		, m_container( m_length )
		, m_isConst( false )
		, m_isMfcList( false )
		, m_objectSelector( NULL )
		, m_libraryType( STL )
	{
		ASSERT( m_pCodeText != NULL );

		parseStatement();
	}

	ContainerComponents::~ContainerComponents()
	{
	}

	void ContainerComponents::parseStatement( void ) throws_( mfc::CRuntimeException )
	{
		int pos = 0;

		str::skipWhiteSpace( pos, m_pCodeText );
		m_leadingWhiteSpace.m_start = 0;
		m_leadingWhiteSpace.m_end = pos;
		m_isConst = str::skipToken( pos, m_pCodeText, _T("const") );

		m_containerType.m_start = m_containerType.m_end = pos;

		while ( m_pCodeText[ pos ] != _T('\0') )
			if ( m_pCodeText[ pos ] == _T('<') )
			{
				BraceParityStatus braceStatus;
				int braceEndPos = braceStatus.findMatchingBracePos( m_pCodeText, pos, DocLang_Cpp );

				if ( braceEndPos == -1 )
					throw new mfc::CRuntimeException( str::Format( _T("Syntax error: cannot find ending template brace for statement '%s'"), m_pCodeText ) );

				++pos;
				str::skipWhiteSpace( pos, m_pCodeText );
				m_objectType.m_start = pos;
				m_objectType.m_end = braceEndPos;
				while ( m_objectType.m_end > m_objectType.m_start && _istspace( m_pCodeText[ m_objectType.m_end - 1 ] ) )
					--m_objectType.m_end;

				m_containerType.m_end = pos = braceEndPos + 1;
				break;
			}
			else if ( str::isCharOneOf( m_pCodeText[ pos ], _T(" \t\r\n*&") ) )
			{
				m_containerType.m_end = pos;
				break;
			}
			else
				++pos;

		if ( m_containerType.IsEmpty() )
			throw new mfc::CRuntimeException( str::Format( _T("Syntax error: cannot find container type in statement '%s'"), m_pCodeText ) );

		m_objectSelector = _T(".");

		str::skipWhiteSpace( pos, m_pCodeText );

		if ( m_pCodeText[ pos ] == _T('*') )
			m_objectSelector = _T("->");	// use pointer selector

		str::skipCharSet( pos, m_pCodeText, _T("*&") );
		str::skipWhiteSpace( pos, m_pCodeText );
		m_isConst |= str::skipToken( pos, m_pCodeText, _T("const") );

		m_container.m_start = m_container.m_end = pos;
		str::skipNotCharSet( m_container.m_end, m_pCodeText, _T(",; \t\r\n") );

		if ( m_container.IsEmpty() )
			throw new mfc::CRuntimeException( str::Format( _T("Syntax error: cannot find container variable in statement '%s'"), m_pCodeText ) );

		extractIteratorName();

		CString containerType = m_containerType.getString( m_pCodeText );

		m_isMfcList = containerType.Find( _T("List") ) != -1;

		if ( containerType.Find( _T("Array") ) != -1 ||
			 containerType.Find( _T("List") ) != -1 ||
			 containerType.Find( _T("Map") ) != -1 )
			m_libraryType = MFC;
		else
			m_libraryType = STL;
	}

	/**
		sample container statement: "rObject.GetParent()->GetAllItems()"
	*/
	void ContainerComponents::extractIteratorName( void )
	{
		m_objectName.Empty();

		CString mirror = code::getMirrorStatement( m_container.getString( m_pCodeText ) );
		const TCHAR* pMirror = mirror;

		int pos = 0;

		if ( pMirror[ pos ] == _T('(') )
			code::skipArgList( pos, pMirror );

		if ( pMirror[ pos ] == _T('<') )
			code::skipArgList( pos, pMirror );

		TokenRange coreRange( pos );

		code::skipCppKeyword( coreRange.m_end, pMirror );

		CString core = coreRange.getString( pMirror );
		core.MakeReverse();

		if ( core.GetLength() > 1 )
		{
			const TCHAR* pCore = core;
			coreRange.setString( pCore );

			if ( pCore[ coreRange.m_start ] == _T('p') || pCore[ coreRange.m_start ] == _T('r') )
				if ( _istupper( pCore[ coreRange.m_start + 1 ] ) )
					++coreRange.m_start;

			str::skipToken( coreRange.m_start, pCore, _T("Get"), str::IgnoreCase );

			if ( pCore[ coreRange.m_end - 1 ] == _T('s') )
				--coreRange.m_end;

			if ( coreRange.IsValid() && !coreRange.IsEmpty() )
			{
				m_objectName = coreRange.getString( pCore );
				m_objectName.SetAt( 0, _totupper( m_objectName[ 0 ] ) );
			}
		}
	}

	void ContainerComponents::showComponents( void )
	{
		CString message;

		message.Format( _T("Container components for:\n'%s'\n\n")
						_T("m_containerType='%s'\n")
						_T("m_objectType='%s'\n")
						_T("m_container='%s'\n")
						_T("m_objectName='%s'"),
						m_pCodeText,
						(LPCTSTR)m_containerType.getString( m_pCodeText ),
						(LPCTSTR)m_objectType.getString( m_pCodeText ),
						(LPCTSTR)m_container.getString( m_pCodeText ),
						(LPCTSTR)m_objectName );

		AfxMessageBox( message );
	}

} //namespace code
