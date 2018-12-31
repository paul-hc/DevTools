
#include "stdafx.h"
#include "IdeUtilities.h"
#include "CppImplementationFormatter.h"
#include "ContainerComponents.h"
#include "CodeUtilities.h"
#include "LanguageSearchEngine.h"
#include "BraceParityStatus.h"
#include "FormatterOptions.h"
#include "CppMethodComponents.h"
#include "InputTypeQualifierDialog.h"
#include "TokenizeTextDialog.h"
#include "TextContent.h"
#include "ModuleSession.h"
#include "Application.h"
#include "resource.h"
#include "utl/MenuUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace code
{
	// CppImplementationFormatter implementation

	CppImplementationFormatter::CppImplementationFormatter( const CFormatterOptions& _options )
		: CFormatter( _options )
	{
		setDocLanguage( DocLang_Cpp );
	}

	CppImplementationFormatter::~CppImplementationFormatter()
	{
	}

	CString CppImplementationFormatter::implementMethodBlock( const TCHAR* methodPrototypes, const TCHAR* typeDescriptor, bool isInline ) throws_( mfc::CRuntimeException )
	{
		resetInternalState();
		loadCodeTemplates();

		CString templateDecl;
		CString typeQualifier;

		splitTypeDescriptor( templateDecl, typeQualifier, typeDescriptor );

		CString separatedPrototypes = methodPrototypes;

		code::convertToWindowsLineEnds( separatedPrototypes );

		static const TCHAR* prototypeSeparator = _T("\r\r");
		BraceParityStatus braceStatus;

		for ( TokenRange prototypeRange( 0 ); str::charAt( separatedPrototypes, prototypeRange.m_end ) != _T('\0'); )
		{
			prototypeRange = braceStatus.findArgList( separatedPrototypes, prototypeRange.m_end, _T("("), m_docLanguage );

			if ( prototypeRange.IsValid() && prototypeRange.getLength() == 2 )
				if ( str::charAt( separatedPrototypes, prototypeRange.m_end ) == _T('(') )
					prototypeRange = braceStatus.findArgList( separatedPrototypes, prototypeRange.m_end, _T("("), m_docLanguage );

			if ( !prototypeRange.IsEmpty() )
			{
				TokenRange endOfLinePos = m_languageEngine.findString( separatedPrototypes, code::lineEnd, prototypeRange.m_end );

				ASSERT( endOfLinePos.IsValid() );

				if ( !endOfLinePos.IsEmpty() )
					endOfLinePos.replaceWithToken( separatedPrototypes, prototypeSeparator );

				prototypeRange.m_end = endOfLinePos.m_end;
			}
		}

		std::vector< CString > sourcePrototypes;

		str::split( sourcePrototypes, separatedPrototypes, prototypeSeparator );

		CString implementedMethods;

		for ( std::vector< CString >::const_iterator itPrototype = sourcePrototypes.begin();
			  itPrototype != sourcePrototypes.end(); ++itPrototype )
		{
			CString implementation = implementMethod( *itPrototype, templateDecl, typeQualifier, isInline );

			if ( !implementation.IsEmpty() )
				implementedMethods += implementation;
		}

		return implementedMethods;
	}

	bool CppImplementationFormatter::loadCodeTemplates( void )
	{
		m_voidFunctionBody = _T("{\r\n}\r\n");
		m_returnFunctionBody = _T("{\r\n\treturn ;\r\n}\r\n");

		m_commentDecorationTemplate.Empty();

		TextContent codeTemplateFile;
		std::tstring codeTemplateFilePath = app::GetModuleSession().m_codeTemplatePath.Get();

		if ( app::GetModuleSession().m_useCommentDecoration )
		{
			if ( codeTemplateFile.LoadFileSection( codeTemplateFilePath.c_str(), _T("Single Line Decoration") ) )
				m_commentDecorationTemplate = codeTemplateFile.GetText();
		}

		if ( codeTemplateFile.LoadFileSection( codeTemplateFilePath.c_str(), _T("void Function Body") ) )
			m_voidFunctionBody = codeTemplateFile.GetText();

		if ( codeTemplateFile.LoadFileSection( codeTemplateFilePath.c_str(), _T("return Function Body") ) )
			m_returnFunctionBody = codeTemplateFile.GetText();

		return !m_commentDecorationTemplate.IsEmpty();
	}

	CString CppImplementationFormatter::makeCommentDecoration( const TCHAR* decorationCore ) const
	{
		ASSERT( decorationCore != NULL );

		CString commentDecoration;

		if ( !m_commentDecorationTemplate.IsEmpty() )
		{
			commentDecoration = m_commentDecorationTemplate;
			commentDecoration.Replace( _T("%TypeName%"), decorationCore );
		}

		return commentDecoration;
	}

	/**
		Extracts a type descriptor from functionImplLine, or prompts the user to input the type qualifier explicitly.

		Type descriptor syntax:
			"[[templateDecl]\r\n][typeQualifier]"

		Examples:
			"template< typename T, class Pred, int dim >\r\nMyClass< T, Pred, i >::"
			"template<>\r\nMyClass< CString, LessThanString, 5 >::"
			"MyClass::"
			"MyClass::NestedClass::"
	*/
	CString CppImplementationFormatter::extractTypeDescriptor( const TCHAR* functionImplLine, const TCHAR* docFileExt )
	{
		resetInternalState();

		CString templateDecl;
		CString typeQualifier;

		{
			CppMethodComponents ranges( functionImplLine );

			ranges.splitMethod( m_validArgListOpenBraces );
			if ( !ranges.m_templateDecl.IsEmpty() )
				templateDecl = ranges.m_templateDecl.getString( functionImplLine );
			if ( !ranges.m_typeQualifier.IsEmpty() )
				typeQualifier = ranges.m_typeQualifier.getString( functionImplLine );
		}

		if ( typeQualifier.IsEmpty() )
		{
			CString userTypeDescriptor = inputDocTypeDescriptor( docFileExt );

			if ( userTypeDescriptor == m_cancelTag )
				return userTypeDescriptor; // canceled by user

			CppMethodComponents ranges( userTypeDescriptor );

			ranges.splitMethod( m_validArgListOpenBraces );
			if ( !ranges.m_templateDecl.IsEmpty() )
				templateDecl = ranges.m_templateDecl.getString( userTypeDescriptor );

			if ( !ranges.m_typeQualifier.IsEmpty() )
				typeQualifier = ranges.m_typeQualifier.getString( userTypeDescriptor );
		}

		CString typeDescriptor;

		if ( !templateDecl.IsEmpty() )
			typeDescriptor = templateDecl + code::lineEnd;

		typeDescriptor += typeQualifier;
		return typeDescriptor;
	}

	CString CppImplementationFormatter::implementMethod( const TCHAR* methodPrototype, const TCHAR* templateDecl,
														 const TCHAR* typeQualifier, bool isInline )
	{
		CString sourcePrototype = makeNormalizedFormattedPrototype( methodPrototype, true );

		prototypeResolveDefaultParameters( sourcePrototype );

		CString implementedMethod;

		if ( !sourcePrototype.IsEmpty() )
		{
			CppMethodComponents method( sourcePrototype );

			method.splitMethod( m_validArgListOpenBraces );

			CString returnType;
			bool hasNoReturnType = true;

			if ( isInline )
				returnType += _T("inline ");

			if ( !method.m_returnType.IsEmpty() )
			{
				CString theReturnType = method.m_returnType.getString( sourcePrototype );

				hasNoReturnType = theReturnType == _T("void");

				returnType += theReturnType;
				returnType += m_options.m_returnTypeOnSeparateLine ? code::lineEnd : _T(" ");
			}

			if ( !m_options.m_returnTypeOnSeparateLine && !returnType.IsEmpty() )
				implementedMethod = returnType;

			if ( typeQualifier != NULL )
				implementedMethod += typeQualifier;

			implementedMethod += TokenRange( method.m_methodName.m_start, method.m_postArgListSuffix.m_end ).getString( sourcePrototype );
			implementedMethod += code::lineEnd;
			implementedMethod = splitArgumentList( implementedMethod );

			if ( m_options.m_returnTypeOnSeparateLine && !returnType.IsEmpty() )
				implementedMethod = returnType + implementedMethod;

			if ( templateDecl != NULL && templateDecl[ 0 ] != _T('\0') )
				implementedMethod = CString( templateDecl ) + code::lineEnd + implementedMethod;

			if ( !m_commentDecorationTemplate.IsEmpty() )
			{
				CString decorationCore = typeQualifier + TokenRange( method.m_methodName.m_start, method.m_methodName.m_end ).getString( sourcePrototype );
				CString commentDecoration = makeCommentDecoration( decorationCore );

				if ( !commentDecoration.IsEmpty() )
					implementedMethod = commentDecoration + code::lineEnd + implementedMethod;
			}

			implementedMethod += hasNoReturnType ? m_voidFunctionBody : m_returnFunctionBody;

			for ( int i = 0; i != m_options.m_linesBetweenFunctionImpls; ++i )
				implementedMethod += code::lineEnd;
		}

		return implementedMethod;
	}

	// example: converts "template< typename Type, class MyClass, struct MyStruct >" to "< Type, MyClass, MyStruct >"

	CString CppImplementationFormatter::buildTemplateInstanceTypeList( const TokenRange& templateDecl, const TCHAR* methodPrototype ) const
	{
		CString outTemplateInstanceList;
		BraceParityStatus braceStatus;
		TokenRange templTypeList = braceStatus.findArgList( methodPrototype, templateDecl.m_start, _T("<"), m_docLanguage );

		if ( templTypeList.m_start >= templateDecl.m_start && templTypeList.m_end <= templateDecl.m_end )
		{
			templTypeList.deflateBy( 1 );
			if ( !templTypeList.IsEmpty() )
			{
				std::vector< CString > typeArgs;

				str::split( typeArgs, templTypeList.getString( methodPrototype ), _T(",") );

				for ( unsigned int i = 0; i != typeArgs.size(); ++i )
				{
					const TCHAR* typeArg = typeArgs[ i ];
					TokenRange argRange = TokenRange::endOfString( typeArg );

					while ( argRange.m_end > 0 && code::isWhitespaceChar( typeArg[ argRange.m_end - 1 ] ) )
						--argRange.m_end;

					argRange.m_start = argRange.m_end;

					while ( argRange.m_start > 0 && !code::isWhitespaceChar( typeArg[ argRange.m_start - 1 ] ) )
						--argRange.m_start;

					if ( !argRange.IsEmpty() )
					{
						if ( !outTemplateInstanceList.IsEmpty() )
							outTemplateInstanceList += _T(", ");

						outTemplateInstanceList += argRange.getString( typeArg );
					}
					else
						TRACE( _T(" * SYNTAX ERROR: cannot extract type argument at index %d from the template '%s'\n"),
							   (LPCTSTR)templateDecl.getString( methodPrototype ) );
				}
			}
		}

		if ( !outTemplateInstanceList.IsEmpty() )
		{
			CString tiList;
			const TCHAR* pSpacer = MustSpaceBrace( _T('<') ) == InsertOneSpace ? _T(" ") : _T("");

			tiList.Format( _T("<%s%s%s>"), pSpacer, (LPCTSTR)outTemplateInstanceList, pSpacer );
			outTemplateInstanceList = tiList;
		}

		return outTemplateInstanceList;
	}

	void CppImplementationFormatter::prototypeResolveDefaultParameters( CString& targetString ) const
	{
		TCHAR openBrace = _T('\0');
		bool inArgList = false;

		for ( TokenRange breakToken( 0 ), prevBreakToken( -1 ); str::charAt( targetString, breakToken.m_end ) != _T('\0'); )
		{
			LineBreakTokenMatch match = findLineBreakToken( breakToken, targetString, breakToken.m_end );

			switch ( match )
			{
				case LBT_OpenBrace:
					if ( !inArgList )
					{
						openBrace = targetString[ breakToken.m_start ];
						if ( str::charAt( targetString, breakToken.m_end ) != code::getMatchingBrace( openBrace ) )
						{
							inArgList = true; // non-empty argument list -> enter arg-list mode
							prevBreakToken = breakToken;
						}
					}
					else
					{
						int nestedBraceEnd = BraceParityStatus().findMatchingBracePos( targetString, breakToken.m_start, DocLang_Cpp );

						if ( nestedBraceEnd != -1 )
							breakToken.m_end = nestedBraceEnd + 1;
						else
							return;
					}
					break;
				case LBT_CloseBrace:
				case LBT_BreakSeparator:
					if ( inArgList )
					{
						int defaultParamPos = m_languageEngine.findString( targetString, _T("="), prevBreakToken.m_end != -1 ? prevBreakToken.m_end : 0 ).m_start;

						if ( defaultParamPos > prevBreakToken.m_end && defaultParamPos < breakToken.m_start )
						{
							if ( match == LBT_CloseBrace )
								--breakToken.m_start; // skip the leading space form " )"

							TokenRange defaultParamRange( defaultParamPos, breakToken.m_start );
							CString newDefaultParam;

							if ( m_options.m_commentOutDefaultParams )
								newDefaultParam.Format( _T("/*%s*/"), (LPCTSTR)defaultParamRange.getString( targetString ) );
							else
								while ( defaultParamRange.m_start > 0 && code::isWhitespaceChar( targetString[ defaultParamRange.m_start - 1 ] ) )
									--defaultParamRange.m_start;

							defaultParamRange.replaceWithToken( targetString, newDefaultParam );
							if ( match == LBT_CloseBrace )
								++defaultParamRange.m_end; // skip the space
							breakToken.assign( defaultParamRange.m_end, defaultParamRange.m_end + 1 );
						}

						if ( match == LBT_CloseBrace )
							inArgList = false; // exit current arg-list mode

						prevBreakToken = breakToken;
					}
					break;
				case LBT_NoMatch:
					return;
			}
		}
	}

	CString CppImplementationFormatter::inputDocTypeDescriptor( const TCHAR* docFileExt ) const
	{
		ASSERT( m_docLanguage == DocLang_Cpp );

		CString docTypeQualifier;

		if ( docFileExt != NULL && docFileExt[ 0 ] != _T('\0') )
		{
			TCHAR fname[ _MAX_FNAME ];

			_tsplitpath( docFileExt, NULL, NULL, fname, NULL );
			if ( fname[ 0 ] != _T('\0') )
				docTypeQualifier.Format( _T("%s%s::"), app::GetModuleSession().m_classPrefix.c_str(), fname );
		}

		enum MenuCommand
		{
			cmdCancel = 0,
			cmdUseDocQualifier = 789,
			cmdUseEmptyQualifier,
			cmdUseCustomQualifier,
		};

		CMenu typeChoiceMenu;

		typeChoiceMenu.CreatePopupMenu();

		if ( !docTypeQualifier.IsEmpty() )
			typeChoiceMenu.AppendMenu( MF_STRING, cmdUseDocQualifier, docTypeQualifier );

		typeChoiceMenu.AppendMenu( MF_STRING, cmdUseEmptyQualifier, _T("&Global Function (No Type Qualifier)") );
		typeChoiceMenu.AppendMenu( MF_SEPARATOR, 0 );
		typeChoiceMenu.AppendMenu( MF_STRING, cmdUseCustomQualifier, _T("&Custom Type Qualifier...") );
		typeChoiceMenu.AppendMenu( MF_SEPARATOR, 0 );
		typeChoiceMenu.AppendMenu( MF_STRING, cmdCancel, _T("Cancel") );

		typeChoiceMenu.SetDefaultItem( cmdUseDocQualifier );

		ide::CScopedWindow scopedIDE;
		MenuCommand command = static_cast< MenuCommand >( scopedIDE.TrackPopupMenu( typeChoiceMenu, ide::GetMouseScreenPos() ) );

		switch ( command )
		{
			case cmdUseDocQualifier:
				return docTypeQualifier;
			case cmdUseEmptyQualifier:
				break;
			case cmdUseCustomQualifier:
			{
				CInputTypeQualifierDialog dlg( docTypeQualifier, scopedIDE.GetMainWnd() );
				if ( dlg.DoModal() != IDCANCEL )
					return dlg.m_typeQualifier;
			}
			case cmdCancel:
				return m_cancelTag;
		}

		return CString();
	}

	//	Type descriptor syntax:
	//		"[[templateDecl]\r\n][typeQualifier]"

	void CppImplementationFormatter::splitTypeDescriptor( CString& templateDecl, CString& typeQualifier, const TCHAR* typeDescriptor ) const throws_( mfc::CRuntimeException )
	{
		std::vector< CString > typeDescriptorComponents;

		switch ( str::split( typeDescriptorComponents, typeDescriptor, code::lineEnd ) )
		{
			case 0:
				break;
			case 2:
				templateDecl = typeDescriptorComponents.front();
			case 1:
				typeQualifier = typeDescriptorComponents.back();
				break;
			default:
				throw new mfc::CRuntimeException( str::Format( _T("Bad type descriptor/qualifier: '%s'"), typeDescriptor ) );
		}

		if ( !typeQualifier.IsEmpty() )
			if ( typeQualifier.GetLength() < 2 || typeQualifier.Right( 2 ) != _T("::") )
				typeQualifier += _T("::");

		if ( !templateDecl.IsEmpty() && !typeQualifier.IsEmpty() )
		{
			// This has a template declaration, therefore outer class (as in "OuterClass::EmbeddedClass::")
			// should have the concrete template type arguments.
			//
			// Example:
			//	for descriptor: "template< typename T, class C >\r\nOuterClass::Embedded::"
			//	typeQualifier should be: "OuterClass< T, C >::Embedded::"

			TokenRange endOfOuterClassRange = m_languageEngine.findString( typeQualifier, _T("::") );

			if ( !endOfOuterClassRange.IsEmpty() )
			{
				TokenRange outerClass( 0, endOfOuterClassRange.m_end );
				BraceParityStatus braceStatus;
				TokenRange templateInstanceArgsRange = braceStatus.findArgList( typeQualifier, outerClass.m_start, _T("<"), m_docLanguage );

				if ( templateInstanceArgsRange.IsEmpty() || templateInstanceArgsRange.m_start > outerClass.m_end )
				{
					CString templateInstanceArgs = buildTemplateInstanceTypeList( TokenRange( templateDecl ), templateDecl );

					endOfOuterClassRange.m_end = endOfOuterClassRange.m_start;
					endOfOuterClassRange.replaceWithToken( typeQualifier, templateInstanceArgs );
				}
			}
		}
	}

	UINT TrackMakeCodeTemplate( void )
	{
		CPoint screenPos;
		GetCursorPos( &screenPos );

		CMenu contextMenu;
		ui::LoadPopupMenu( contextMenu, IDR_CONTEXT_MENU, app::AutoMakeCodePopup );

		ide::CScopedWindow scopedIDE;
		return scopedIDE.TrackPopupMenu( contextMenu, screenPos );
	}

	CString CppImplementationFormatter::autoMakeCode( const TCHAR* codeText )
	{
		resetInternalState();

		CString outcome;
		UINT commandId = TrackMakeCodeTemplate();

		switch ( commandId )
		{
			case CM_MAKE_LOOP_const_iterator:
				outcome = makeIteratorLoop( codeText, true );
				break;
			case CM_MAKE_LOOP_iterator:
				outcome = makeIteratorLoop( codeText, false );
				break;
			case CM_MAKE_LOOP_index:
				outcome = makeIndexLoop( codeText );
				break;
			default:
				return CString();
		}

		return outcome;
	}

	CString CppImplementationFormatter::makeIteratorLoop( const TCHAR* codeText, bool isConstIterator ) throws_( mfc::CRuntimeException )
	{
		ContainerComponents components( codeText );
		TextContent codeTemplateFile;
		CString outcome;

		if ( codeTemplateFile.LoadFileSection( app::GetModuleSession().m_codeTemplatePath.GetPtr(), _T("ForLoopIterator") ) )
		{
			codeTemplateFile.ReplaceText( _T("%ContainerType%"), components.m_containerType.getString( codeText ), TRUE );
			codeTemplateFile.ReplaceText( _T("%IteratorType%"), isConstIterator ? _T("const_iterator") : _T("iterator"), TRUE );
			codeTemplateFile.ReplaceText( _T("%IteratorName%"), components.m_objectName, TRUE );
			codeTemplateFile.ReplaceText( _T("%Container%"), components.m_container.getString( codeText ), TRUE );
			codeTemplateFile.ReplaceText( _T("%Selector%"), components.m_objectSelector, TRUE );
			codeTemplateFile.ReplaceText( _T("%LeadingSpaces%"), components.m_leadingWhiteSpace.getString( codeText ), TRUE );

			outcome = codeTemplateFile.GetText();
		}

		return outcome;
	}

	CString CppImplementationFormatter::makeIndexLoop( const TCHAR* codeText ) throws_( mfc::CRuntimeException )
	{
		ContainerComponents components( codeText );
		TextContent codeTemplateFile;
		CString outcome;

		if ( codeTemplateFile.LoadFileSection( app::GetModuleSession().m_codeTemplatePath.GetPtr(), components.m_isMfcList ? _T("ForLoopPosition") : _T("ForLoopIndex") ) )
		{
			codeTemplateFile.ReplaceText( _T("%IndexType%"), STL == components.m_libraryType ? _T("size_t") : _T("int"), TRUE );
			codeTemplateFile.ReplaceText( _T("%ObjectType%"), components.m_objectType.getString( codeText ), TRUE );
			codeTemplateFile.ReplaceText( _T("%ObjectName%"), components.m_objectName, TRUE );
			codeTemplateFile.ReplaceText( _T("%Index%"), _T("i"), TRUE );
			codeTemplateFile.ReplaceText( _T("%Container%"), components.m_container.getString( codeText ), TRUE );
			codeTemplateFile.ReplaceText( _T("%Selector%"), components.m_objectSelector, TRUE );
			codeTemplateFile.ReplaceText( _T("%GetSizeMethod%"), MFC == components.m_libraryType ? _T("GetSize") : _T("size"), TRUE );
			codeTemplateFile.ReplaceText( _T("%LeadingSpaces%"), components.m_leadingWhiteSpace.getString( codeText ), TRUE );

			outcome = codeTemplateFile.GetText();
		}

		return outcome;
	}

	CString CppImplementationFormatter::tokenizeText( const TCHAR* codeText )
	{
		ide::CScopedWindow scopedIDE;
		CTokenizeTextDialog dialog( scopedIDE.GetMainWnd() );
		dialog.m_sourceText = codeText;

		if ( IDCANCEL == dialog.DoModal() )
			return CString();

		return dialog.m_outputText.c_str();
	}

} // namespace code
