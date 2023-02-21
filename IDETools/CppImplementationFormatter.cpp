
#include "pch.h"
#include "IdeUtilities.h"
#include "CppImplementationFormatter.h"
#include "IterationSlices.h"
#include "CodeUtilities.h"
#include "LanguageSearchEngine.h"
#include "BraceParityStatus.h"
#include "FormatterOptions.h"
#include "MethodPrototype.h"
#include "InputTypeQualifierDialog.h"
#include "TokenizeTextDialog.h"
#include "TextContent.h"
#include "ModuleSession.h"
#include "Application.h"
#include "resource.h"
#include "utl/StringUtilities.h"
#include "utl/TextClipboard.h"
#include "utl/UI/MenuUtilities.h"

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

	bool CppImplementationFormatter::isCppTypeQualifier( std::tstring typeQualifier )
	{
		str::Replace( typeQualifier, _T("::"), _T("") );
		return word::IsAlphaNumericWord( typeQualifier );
	}

	CString CppImplementationFormatter::implementMethodBlock( const TCHAR* pMethodPrototypes, const TCHAR* pTypeDescriptor, bool isInline ) throws_( mfc::CRuntimeException )
	{
		resetInternalState();
		loadCodeTemplates();

		CString templateDecl;
		CString typeQualifier;

		splitTypeDescriptor( templateDecl, typeQualifier, pTypeDescriptor );

		CString separatedPrototypes = pMethodPrototypes;

		code::convertToWindowsLineEnds( separatedPrototypes );

		static const std::tstring s_protoSep = _T("\r\r");
		BraceParityStatus braceStatus;

		for ( TokenRange range( 0 ); str::charAt( separatedPrototypes, range.m_end ) != _T('\0'); )
		{
			range = braceStatus.findArgList( separatedPrototypes, range.m_end, _T("("), m_docLanguage );

			if ( range.IsValid() && 2 == range.getLength() )
				if ( '(' == str::charAt( separatedPrototypes, range.m_end ) )
					range = braceStatus.findArgList( separatedPrototypes, range.m_end, _T("("), m_docLanguage );

			if ( !range.IsEmpty() )
			{
				TokenRange endOfLinePos = m_languageEngine.findString( separatedPrototypes, code::lineEnd, range.m_end );

				ASSERT( endOfLinePos.IsValid() );

				if ( !endOfLinePos.IsEmpty() )
					endOfLinePos.replaceWithToken( &separatedPrototypes, s_protoSep.c_str() );

				range.m_end = endOfLinePos.m_end;
			}
		}

		std::vector< CString > sourcePrototypes;

		str::split( sourcePrototypes, separatedPrototypes, s_protoSep.c_str() );

		CString implementedMethods;

		for ( std::vector< CString >::const_iterator itPrototype = sourcePrototypes.begin(); itPrototype != sourcePrototypes.end(); ++itPrototype )
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

	CString CppImplementationFormatter::makeCommentDecoration( const TCHAR* pDecorationCore ) const
	{
		ASSERT_PTR( pDecorationCore );

		CString commentDecoration;

		if ( !m_commentDecorationTemplate.IsEmpty() )
		{
			commentDecoration = m_commentDecorationTemplate;
			commentDecoration.Replace( _T("%TypeName%"), pDecorationCore );
		}

		return commentDecoration;
	}

	/**
		Extracts a type descriptor from pFunctionImplLine, or prompts the user to input the type qualifier explicitly.

		Type descriptor syntax:
			"[[templateDecl]\r\n][typeQualifier]"

		Examples:
			"template< typename T, class Pred, int dim >\r\nMyClass< T, Pred, i >::"
			"template<>\r\nMyClass< CString, LessThanString, 5 >::"
			"MyClass::"
			"MyClass::NestedClass::"
	*/
	CString CppImplementationFormatter::extractTypeDescriptor( const TCHAR* pFunctionImplLine, const TCHAR* pDocFilename )
	{
		resetInternalState();

		CString templateDecl;
		CString typeQualifier;

		{
			CMethodPrototype ranges;

			ranges.SplitMethod( pFunctionImplLine );		//m_validArgListOpenBraces
			if ( !ranges.m_templateDecl.IsEmpty() )
				templateDecl = ranges.m_templateDecl.getString( pFunctionImplLine );
			if ( !ranges.m_classQualifier.IsEmpty() )
				typeQualifier = ranges.m_classQualifier.getString( pFunctionImplLine );
		}

		if ( typeQualifier.IsEmpty() )
		{
			CString userTypeDescriptor = inputDocTypeDescriptor( pDocFilename );

			if ( userTypeDescriptor == m_cancelTag )
				return userTypeDescriptor; // canceled by user

			CMethodPrototype ranges;

			ranges.SplitMethod( userTypeDescriptor.GetString() );		//m_validArgListOpenBraces
			if ( !ranges.m_templateDecl.IsEmpty() )
				templateDecl = ranges.m_templateDecl.getString( userTypeDescriptor );

			if ( !ranges.m_classQualifier.IsEmpty() )
				typeQualifier = ranges.m_classQualifier.getString( userTypeDescriptor );
		}

		CString typeDescriptor;

		if ( !templateDecl.IsEmpty() )
			typeDescriptor = templateDecl + code::lineEnd;

		typeDescriptor += typeQualifier;
		return typeDescriptor;
	}

	CString CppImplementationFormatter::implementMethod( const TCHAR* pMethodPrototype, const TCHAR* pTemplateDecl,
														 const TCHAR* pTypeQualifier, bool isInline )
	{
		CString sourcePrototype = makeNormalizedFormattedPrototype( pMethodPrototype, true );

		prototypeResolveDefaultParameters( sourcePrototype );

		CString implementedMethod;

		if ( !sourcePrototype.IsEmpty() )
		{
			CMethodPrototype method;

			method.SplitMethod( sourcePrototype.GetString() );		//m_validArgListOpenBraces

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

			if ( pTypeQualifier != NULL )
				implementedMethod += pTypeQualifier;

			implementedMethod += TokenRange( method.m_qualifiedMethod.m_start, method.m_postArgListSuffix.m_end ).getString( sourcePrototype );
			implementedMethod += code::lineEnd;
			implementedMethod = splitArgumentList( implementedMethod );

			if ( m_options.m_returnTypeOnSeparateLine && !returnType.IsEmpty() )
				implementedMethod = returnType + implementedMethod;

			if ( pTemplateDecl != NULL && pTemplateDecl[ 0 ] != _T('\0') )
				implementedMethod = CString( pTemplateDecl ) + code::lineEnd + implementedMethod;

			if ( !m_commentDecorationTemplate.IsEmpty() )
			{
				CString decorationCore = pTypeQualifier + TokenRange( method.m_qualifiedMethod.m_start, method.m_qualifiedMethod.m_end ).getString( sourcePrototype );
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

	CString CppImplementationFormatter::buildTemplateInstanceTypeList( const TokenRange& templateDecl, const TCHAR* pMethodPrototype ) const
	{
		CString outTemplateInstanceList;
		BraceParityStatus braceStatus;
		TokenRange templTypeList = braceStatus.findArgList( pMethodPrototype, templateDecl.m_start, _T("<"), m_docLanguage );

		if ( templTypeList.m_start >= templateDecl.m_start && templTypeList.m_end <= templateDecl.m_end )
		{
			templTypeList.deflateBy( 1 );
			if ( !templTypeList.IsEmpty() )
			{
				std::vector< CString > typeArgs;

				str::split( typeArgs, templTypeList.getString( pMethodPrototype ), _T(",") );

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
							   (LPCTSTR)templateDecl.getString( pMethodPrototype ) );
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

	void CppImplementationFormatter::prototypeResolveDefaultParameters( CString& rTargetString ) const
	{
		TCHAR openBrace = _T('\0');
		bool inArgList = false;

		for ( TokenRange breakToken( 0 ), prevBreakToken( -1 ); str::charAt( rTargetString, breakToken.m_end ) != _T('\0'); )
		{
			LineBreakTokenMatch match = findLineBreakToken( breakToken, rTargetString, breakToken.m_end );

			switch ( match )
			{
				case LBT_OpenBrace:
					if ( !inArgList )
					{
						openBrace = rTargetString[ breakToken.m_start ];
						if ( str::charAt( rTargetString, breakToken.m_end ) != code::getMatchingBrace( openBrace ) )
						{
							inArgList = true; // non-empty argument list -> enter arg-list mode
							prevBreakToken = breakToken;
						}
					}
					else
					{
						int nestedBraceEnd = BraceParityStatus().findMatchingBracePos( rTargetString, breakToken.m_start, DocLang_Cpp );

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
						int defaultParamPos = m_languageEngine.findString( rTargetString, _T("="), prevBreakToken.m_end != -1 ? prevBreakToken.m_end : 0 ).m_start;

						if ( defaultParamPos > prevBreakToken.m_end && defaultParamPos < breakToken.m_start )
						{
							if ( match == LBT_CloseBrace )
								--breakToken.m_start; // skip the leading space form " )"

							TokenRange defaultParamRange( defaultParamPos, breakToken.m_start );
							CString newDefaultParam;

							if ( m_options.m_commentOutDefaultParams )
								newDefaultParam.Format( _T("/*%s*/"), (LPCTSTR)defaultParamRange.getString( rTargetString ) );
							else
								while ( defaultParamRange.m_start > 0 && code::isWhitespaceChar( rTargetString[ defaultParamRange.m_start - 1 ] ) )
									--defaultParamRange.m_start;

							defaultParamRange.replaceWithToken( &rTargetString, newDefaultParam );
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

	CString CppImplementationFormatter::inputDocTypeDescriptor( const TCHAR* pDocFilename ) const
	{
		ASSERT( DocLang_Cpp == m_docLanguage );

		ide::CScopedWindow scopedIDE;

		std::tstring docTypeQualifier;
		if ( !str::IsEmpty( pDocFilename ) )
		{
			std::tstring fname = fs::CPath( pDocFilename ).GetFname();
			if ( !fname.empty() )
				docTypeQualifier = str::Format( _T("%s%s::"), app::GetModuleSession().m_classPrefix.c_str(), fname.c_str() );
		}

		std::tstring clipTypeQualifier;
		if ( CTextClipboard::CanPasteText() )
		{
			std::tstring clipText;
			if ( CTextClipboard::PasteText( clipText, scopedIDE.GetMainWnd()->GetSafeHwnd() ) )
			{
				str::Trim( clipText );
				if ( isCppTypeQualifier( clipText ) )
				{
					if ( !str::IsUpperMatch( clipText.c_str(), 2 ) )		// not a type-prefixed token?
						clipTypeQualifier = app::GetModuleSession().m_classPrefix;

					clipTypeQualifier += clipText + _T("::");
				}
			}
		}

		enum MenuCommand
		{
			cmdCancel = 0,
			cmdUseDocQualifier = 789,
			cmdUseClipboardQualifier,
			cmdUseEmptyQualifier,
			cmdUseCustomQualifier,
		};

		CMenu typeChoiceMenu;

		typeChoiceMenu.CreatePopupMenu();

		if ( !docTypeQualifier.empty() )
			typeChoiceMenu.AppendMenu( MF_STRING, cmdUseDocQualifier, docTypeQualifier.c_str() );

		if ( !clipTypeQualifier.empty() )
		{
			static std::tstring s_itemText;

			s_itemText = clipTypeQualifier + _T("\t(Paste)");
			typeChoiceMenu.AppendMenu( MF_STRING, cmdUseClipboardQualifier, s_itemText.c_str() );
		}

		typeChoiceMenu.AppendMenu( MF_STRING, cmdUseEmptyQualifier, _T("&Global Function (No Type Qualifier)") );
		typeChoiceMenu.AppendMenu( MF_SEPARATOR, 0 );
		typeChoiceMenu.AppendMenu( MF_STRING, cmdUseCustomQualifier, _T("&Custom Type Qualifier...") );
		typeChoiceMenu.AppendMenu( MF_SEPARATOR, 0 );
		typeChoiceMenu.AppendMenu( MF_STRING, cmdCancel, _T("Cancel") );

		typeChoiceMenu.SetDefaultItem( cmdUseDocQualifier );

		MenuCommand command = static_cast<MenuCommand>( scopedIDE.TrackPopupMenu( typeChoiceMenu, ide::GetMouseScreenPos() ) );

		switch ( command )
		{
			case cmdUseDocQualifier:
				return docTypeQualifier.c_str();
			case cmdUseClipboardQualifier:
				return clipTypeQualifier.c_str();
			case cmdUseEmptyQualifier:
				break;
			case cmdUseCustomQualifier:
			{
				CInputTypeQualifierDialog dlg( docTypeQualifier.c_str(), scopedIDE.GetMainWnd() );
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
	//
	void CppImplementationFormatter::splitTypeDescriptor( CString& rTemplateDecl, CString& rTypeQualifier, const TCHAR* pTypeDescriptor ) const throws_( mfc::CRuntimeException )
	{
		std::vector<CString> typeDescriptorComponents;

		switch ( str::split( typeDescriptorComponents, pTypeDescriptor, code::lineEnd ) )
		{
			case 0:
				break;
			case 2:
				rTemplateDecl = typeDescriptorComponents.front();
			case 1:
				rTypeQualifier = typeDescriptorComponents.back();
				break;
			default:
				throw new mfc::CRuntimeException( str::Format( _T("Bad type descriptor/qualifier: '%s'"), pTypeDescriptor ) );
		}

		if ( !rTypeQualifier.IsEmpty() )
			if ( rTypeQualifier.GetLength() < 2 || rTypeQualifier.Right( 2 ) != _T("::") )
				rTypeQualifier += _T("::");

		if ( !rTemplateDecl.IsEmpty() && !rTypeQualifier.IsEmpty() )
		{
			// This has a template declaration, therefore outer class (as in "OuterClass::EmbeddedClass::")
			// should have the concrete template type arguments.
			//
			// Example:
			//	for descriptor: "template< typename T, class C >\r\nOuterClass::Embedded::"
			//	rTypeQualifier should be: "OuterClass< T, C >::Embedded::"

			TokenRange endOfOuterClassRange = m_languageEngine.findString( rTypeQualifier, _T("::") );

			if ( !endOfOuterClassRange.IsEmpty() )
			{
				TokenRange outerClass( 0, endOfOuterClassRange.m_end );
				BraceParityStatus braceStatus;
				TokenRange templateInstanceArgsRange = braceStatus.findArgList( rTypeQualifier, outerClass.m_start, _T("<"), m_docLanguage );

				if ( templateInstanceArgsRange.IsEmpty() || templateInstanceArgsRange.m_start > outerClass.m_end )
				{
					CString templateInstanceArgs = buildTemplateInstanceTypeList( TokenRange( rTemplateDecl ), rTemplateDecl );

					endOfOuterClassRange.m_end = endOfOuterClassRange.m_start;
					endOfOuterClassRange.replaceWithToken( &rTypeQualifier, templateInstanceArgs );
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

	CString CppImplementationFormatter::autoMakeCode( const TCHAR* pCodeText )
	{
		resetInternalState();

		CString outcome;
		UINT commandId = TrackMakeCodeTemplate();

		switch ( commandId )
		{
			case CM_MAKE_LOOP_const_iterator:
			case CM_MAKE_LOOP_iterator:
				outcome = makeIteratorLoop( pCodeText, CM_MAKE_LOOP_const_iterator == commandId );
				break;
			case CM_MAKE_LOOP_index:
				outcome = makeIndexLoop( pCodeText );
				break;
			default:
				return CString();
		}

		return outcome;
	}

	CString CppImplementationFormatter::makeIteratorLoop( const TCHAR* pCodeText, bool isConstIterator ) throws_( mfc::CRuntimeException )
	{
		CIterationSlices slices;
		slices.ParseStatement( pCodeText );

		TextContent codeTemplateFile;
		CString outcome;

		if ( codeTemplateFile.LoadFileSection( app::GetModuleSession().m_codeTemplatePath.GetPtr(), _T("ForLoopIterator") ) )
		{
			codeTemplateFile.ReplaceText( _T("%ContainerType%"), slices.m_containerType.getString( pCodeText ), TRUE );
			codeTemplateFile.ReplaceText( _T("%IteratorType%"), isConstIterator ? _T("const_iterator") : _T("iterator"), TRUE );
			codeTemplateFile.ReplaceText( _T("%IteratorName%"), slices.m_iteratorName.c_str(), TRUE);
			codeTemplateFile.ReplaceText( _T("%Container%"), slices.m_containerName.getString( pCodeText ), TRUE );
			codeTemplateFile.ReplaceText( _T("%Selector%"), slices.m_pObjSelOp, TRUE );
			codeTemplateFile.ReplaceText( _T("%LeadingSpaces%"), slices.m_leadingWhiteSpace.getString( pCodeText ), TRUE );

			outcome = codeTemplateFile.GetText();
		}

		return outcome;
	}

	CString CppImplementationFormatter::makeIndexLoop( const TCHAR* pCodeText ) throws_( mfc::CRuntimeException )
	{
		CIterationSlices slices;
		slices.ParseStatement( pCodeText );

		TextContent codeTemplateFile;
		CString outcome;

		if ( codeTemplateFile.LoadFileSection( app::GetModuleSession().m_codeTemplatePath.GetPtr(), slices.m_isMfcList ? _T("ForLoopPosition") : _T("ForLoopIndex") ) )
		{
			codeTemplateFile.ReplaceText( _T("%IndexType%"), CIterationSlices::STL == slices.m_libraryType ? _T("size_t") : _T("int"), TRUE );
			codeTemplateFile.ReplaceText( _T("%ObjectType%"), slices.m_valueType.getString( pCodeText ), TRUE );
			codeTemplateFile.ReplaceText( _T("%ObjectName%"), slices.m_iteratorName.c_str(), TRUE);
			codeTemplateFile.ReplaceText( _T("%Index%"), _T("i"), TRUE );
			codeTemplateFile.ReplaceText( _T("%Container%"), slices.m_containerName.getString( pCodeText ), TRUE );
			codeTemplateFile.ReplaceText( _T("%Selector%"), slices.m_pObjSelOp, TRUE );
			codeTemplateFile.ReplaceText( _T("%GetSizeMethod%"), CIterationSlices::MFC == slices.m_libraryType ? _T("GetSize") : _T("size"), TRUE );
			codeTemplateFile.ReplaceText( _T("%LeadingSpaces%"), slices.m_leadingWhiteSpace.getString( pCodeText ), TRUE );

			outcome = codeTemplateFile.GetText();
		}

		return outcome;
	}

	CString CppImplementationFormatter::tokenizeText( const TCHAR* pCodeText )
	{
		ide::CScopedWindow scopedIDE;
		CTokenizeTextDialog dialog( scopedIDE.GetMainWnd() );
		dialog.m_sourceText = pCodeText;

		if ( IDCANCEL == dialog.DoModal() )
			return CString();

		return dialog.m_outputText.c_str();
	}

} // namespace code
