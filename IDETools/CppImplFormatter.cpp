
#include "pch.h"
#include "IdeUtilities.h"
#include "CppImplFormatter.h"
#include "IterationSlices.h"
	#include "CodeUtilities.h"
	#include "LanguageSearchEngine.h"
	#include "BraceParityStatus.h"
#include "FormatterOptions.h"
#include "MethodPrototype.h"
#include "CppParser.h"
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


namespace ui
{
	UINT TrackMakeCodeTemplate( void )
	{
		CPoint screenPos;
		GetCursorPos( &screenPos );

		CMenu contextMenu;
		ui::LoadPopupMenu( contextMenu, IDR_CONTEXT_MENU, app::AutoMakeCodePopup );

		ide::CScopedWindow scopedIDE;
		return scopedIDE.TrackPopupMenu( contextMenu, screenPos );
	}
}


namespace code
{
	// CCppImplFormatter implementation

	CCppImplFormatter::CCppImplFormatter( const CFormatterOptions& options )
		: CFormatter( options )
	{
		setDocLanguage( DocLang_Cpp );
	}

	CCppImplFormatter::~CCppImplFormatter()
	{
	}

	bool CCppImplFormatter::isCppTypeQualifier( std::tstring typeQualifier )
	{
		str::Replace( typeQualifier, _T("::"), _T("") );
		return word::IsAlphaNumericWord( typeQualifier );
	}

	std::tstring CCppImplFormatter::ImplementMethodBlock( const TCHAR* pMethodPrototypes, const TCHAR* pTypeDescriptor, bool isInline ) throws_( CRuntimeException )
	{
		resetInternalState();
		LoadCodeTemplates();

		CTypeDescriptor tdInfo( this );

		tdInfo.Parse( pTypeDescriptor );

		std::tstring prototypeBlock = pMethodPrototypes;
		str::ToWindowsLineEnds( &prototypeBlock );

		CCppCodeParser protoParser( &prototypeBlock );
		static const std::tstring s_protoSep = _T("\r\r");

		for ( int pos = 0; pos != protoParser.m_length; )
		{
			// for each prototype line: skip arg-list(s) to locate the line-end:
			TokenRange argList;
			if ( !protoParser.FindArgList( &argList, pos, '(' ) )
				break;

			if ( 2 == argList.getLength() )		// "()" empty arg-list, i.e. from "operator()( ... )"?
			{
				TokenRange nextArgList;
				if ( protoParser.FindArgList( &nextArgList, argList.m_end, '(' ) )
					argList = nextArgList;		// skip to the following
			}

			TokenRange endOfLine;
			if ( !protoParser.FindNextSequence( &endOfLine, argList.m_end, code::lineEnd ) )
				break;					// we're done with last line

			endOfLine.ReplaceWithToken( &prototypeBlock, s_protoSep.c_str() );
			pos = endOfLine.m_end;		// skip to next line
		}

		std::vector<std::tstring> prototypes;
		str::Split( prototypes, prototypeBlock.c_str(), s_protoSep.c_str());

		std::tstring implementedMethods;

		for ( std::vector<std::tstring>::const_iterator itProto = prototypes.begin(); itProto != prototypes.end(); ++itProto )
		{
			std::tstring implementation = ImplementMethod( *itProto, tdInfo.m_templateDecl, tdInfo.m_typeQualifier, isInline );

			if ( !implementation.empty() )
				implementedMethods += implementation;
		}

		return implementedMethods;
	}

	bool CCppImplFormatter::LoadCodeTemplates( void )
	{
		m_voidFunctionBody = _T("{\r\n}\r\n");
		m_returnFunctionBody = _T("{\r\n\treturn ;\r\n}\r\n");

		m_commentDecorationTemplate.clear();

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

		return !m_commentDecorationTemplate.empty();
	}

	std::tstring CCppImplFormatter::MakeCommentDecoration( const std::tstring& decorationCore ) const
	{
		std::tstring commentDecoration = m_commentDecorationTemplate;

		if ( !commentDecoration.empty() )
			str::Replace( commentDecoration, _T("%TypeName%"), decorationCore.c_str() );

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
	std::tstring CCppImplFormatter::ExtractTypeDescriptor( const std::tstring& functionImplLine, const fs::CPath& docPath )
	{
		resetInternalState();

		CMethodPrototype proto;

		proto.ParseCode( functionImplLine );		//m_validArgListOpenBraces

		std::tstring templateDecl = proto.m_templateDecl.MakeToken( functionImplLine );
		std::tstring typeQualifier = proto.m_classQualifier.MakeToken( functionImplLine );

		if ( typeQualifier.empty() )
		{
			std::tstring userTypeDescriptor = InputDocTypeDescriptor( docPath );

			if ( userTypeDescriptor == s_cancelTag )
				return userTypeDescriptor;			// canceled by user

			proto.ParseCode( userTypeDescriptor );		//m_validArgListOpenBraces

			templateDecl = proto.m_templateDecl.MakeToken( userTypeDescriptor );
			typeQualifier = proto.m_classQualifier.MakeToken( userTypeDescriptor );
		}

		std::tstring typeDescriptor;

		if ( !templateDecl.empty() )
			typeDescriptor = templateDecl + code::lineEnd;

		typeDescriptor += typeQualifier;
		return typeDescriptor;
	}

	std::tstring CCppImplFormatter::ImplementMethod( const std::tstring& methodProto, const std::tstring& templateDecl, const std::tstring& typeQualifier, bool isInline )
	{
		std::tstring srcPrototype = makeNormalizedFormattedPrototype( methodProto.c_str(), true );
		std::tstring implementedMethod;

		if ( !srcPrototype.empty() )
		{
			ResolveDefaultParameters( &srcPrototype );

			CMethodPrototype method;

			method.ParseCode( srcPrototype );		//m_validArgListOpenBraces

			std::tstring returnType;
			bool hasNoReturnType = true;

			if ( isInline )
				returnType += _T("inline ");

			if ( !method.m_returnType.IsEmpty() )
			{
				std::tstring theReturnType = method.m_returnType.MakeToken( srcPrototype );

				hasNoReturnType = ( theReturnType == _T("void") );

				returnType += theReturnType;
				returnType += m_options.m_returnTypeOnSeparateLine ? code::lineEnd : _T(" ");
			}

			if ( !m_options.m_returnTypeOnSeparateLine && !returnType.empty() )
				implementedMethod = returnType;

			implementedMethod += typeQualifier;

			implementedMethod += TokenRange( method.m_qualifiedMethod.m_start, method.m_postArgListSuffix.m_end ).MakeToken( srcPrototype );
			implementedMethod += code::lineEnd;
			implementedMethod = splitArgumentList( implementedMethod.c_str() );

			if ( m_options.m_returnTypeOnSeparateLine && !returnType.empty() )
				implementedMethod = returnType + implementedMethod;

			if ( !templateDecl.empty() )
				implementedMethod = templateDecl + code::lineEnd + implementedMethod;

			if ( !m_commentDecorationTemplate.empty() )
			{
				std::tstring decorationCore = typeQualifier + TokenRange( method.m_qualifiedMethod.m_start, method.m_qualifiedMethod.m_end ).MakeToken( srcPrototype );
				std::tstring commentDecoration = MakeCommentDecoration( decorationCore );

				if ( !commentDecoration.empty() )
					implementedMethod = commentDecoration + code::lineEnd + implementedMethod;
			}

			implementedMethod += hasNoReturnType ? m_voidFunctionBody : m_returnFunctionBody;

			for ( int i = 0; i != m_options.m_linesBetweenFunctionImpls; ++i )
				implementedMethod += code::lineEnd;
		}

		return implementedMethod;
	}

	void CCppImplFormatter::ResolveDefaultParameters( std::tstring* pProto ) const
	{
		ASSERT_PTR( pProto );
	#pragma message( __WARN__ "TODO: simplify this mess, pretty please!" )

		const TCHAR* pProtoText = pProto->c_str();
		int length = static_cast<int>( pProto->length() );
		TCHAR openBrace = '\0';
		bool inArgList = false;

		for ( TokenRange breakToken( 0 ), prevBreakToken( -1 ); breakToken.m_end != length; )
		{
			LineBreakTokenMatch match = findLineBreakToken( &breakToken, pProtoText, breakToken.m_end );

			switch ( match )
			{
				case LBT_OpenBrace:
					if ( !inArgList )
					{
						openBrace = pProtoText[ breakToken.m_start ];
						if ( pProtoText[ breakToken.m_end ] != code::getMatchingBrace( openBrace ) )
						{
							inArgList = true;				// non-empty argument list -> enter arg-list mode
							prevBreakToken = breakToken;
						}
					}
					else
					{
						int nestedBraceEnd = BraceParityStatus().findMatchingBracePos( pProtoText, breakToken.m_start, DocLang_Cpp );

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
						int defaultParamPos = m_languageEngine.findString( pProtoText, _T("="), prevBreakToken.m_end != -1 ? prevBreakToken.m_end : 0 ).m_start;

						if ( defaultParamPos > prevBreakToken.m_end && defaultParamPos < breakToken.m_start )
						{
							if ( LBT_CloseBrace == match )
								--breakToken.m_start;		// skip the leading space from " )"

							TokenRange defaultParamRange( defaultParamPos, breakToken.m_start );
							std::tstring newDefaultParam;
							pred::IsSpace isSpacePred;

							if ( m_options.m_commentOutDefaultParams )
								newDefaultParam = str::Format( _T("/*%s*/"), defaultParamRange.MakeToken( *pProto ).c_str() );
							else
								while ( defaultParamRange.m_start > 0 && isSpacePred( pProtoText[ defaultParamRange.m_start - 1 ] ) )
									--defaultParamRange.m_start;

							defaultParamRange.ReplaceWithToken( pProto, newDefaultParam );

							if ( LBT_CloseBrace == match )
								++defaultParamRange.m_end; // skip the space

							breakToken.assign( defaultParamRange.m_end, defaultParamRange.m_end + 1 );
						}

						if ( match == LBT_CloseBrace )
							inArgList = false;			// exit current arg-list mode

						prevBreakToken = breakToken;
					}
					break;
				case LBT_NoMatch:
					return;
			}
		}
	}

	std::tstring CCppImplFormatter::InputDocTypeDescriptor( const fs::CPath& docPath ) const
	{
		ASSERT( DocLang_Cpp == m_docLanguage );

		ide::CScopedWindow scopedIDE;

		std::tstring docTypeQualifier;
		std::tstring fname = docPath.GetFname();

		if ( !fname.empty() )
			docTypeQualifier = str::Format( _T("%s%s::"), app::GetModuleSession().m_classPrefix.c_str(), fname.c_str() );

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
				return docTypeQualifier;
			case cmdUseClipboardQualifier:
				return clipTypeQualifier;
			case cmdUseEmptyQualifier:
				break;
			case cmdUseCustomQualifier:
			{
				CInputTypeQualifierDialog dlg( docTypeQualifier.c_str(), scopedIDE.GetMainWnd() );
				if ( dlg.DoModal() != IDCANCEL )
					return dlg.m_typeQualifier.GetString();
			}
			case cmdCancel:
				return s_cancelTag;
		}

		return str::GetEmpty();
	}


	CString CCppImplFormatter::autoMakeCode( const TCHAR* pCodeText )
	{
		resetInternalState();

		CString outcome;
		UINT commandId = ui::TrackMakeCodeTemplate();

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

	CString CCppImplFormatter::makeIteratorLoop( const TCHAR* pCodeText, bool isConstIterator ) throws_( CRuntimeException )
	{
		CIterationSlices slices;
		slices.ParseCode( pCodeText );

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

	CString CCppImplFormatter::makeIndexLoop( const TCHAR* pCodeText ) throws_( CRuntimeException )
	{
		CIterationSlices slices;
		slices.ParseCode( pCodeText );

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

	CString CCppImplFormatter::tokenizeText( const TCHAR* pCodeText )
	{
		ide::CScopedWindow scopedIDE;
		CTokenizeTextDialog dialog( scopedIDE.GetMainWnd() );
		dialog.m_sourceText = pCodeText;

		if ( IDCANCEL == dialog.DoModal() )
			return CString();

		return dialog.m_outputText.c_str();
	}

} // namespace code


namespace code
{
	// CTypeDescriptor implementation

	void CTypeDescriptor::Parse( const TCHAR* pTypeDescriptor ) throws_( CRuntimeException )
	{
		Split( pTypeDescriptor );

		if ( !m_typeQualifier.empty() )
			if ( !str::HasSuffix( m_typeQualifier.c_str(), _T("::") ) )
				m_typeQualifier += _T("::");

		if ( !m_templateDecl.empty() && !m_typeQualifier.empty() )
		{
			// This has a template declaration, therefore outer class (as in "OuterClass::EmbeddedClass::")
			// should have the concrete template type arguments.
			//
			// Example:
			//	for descriptor: "template< typename T, class C >\r\nOuterClass::Embedded::"
			//	rTypeQualifier should be: "OuterClass< T, C >::Embedded::"

			CCppCodeParser tqParser( &m_typeQualifier );
			TokenRange endOfOuterClassRange;

			if ( tqParser.FindNextSequence( &endOfOuterClassRange, 0, _T("::") ) )
			{
				TokenRange outerClass( 0, endOfOuterClassRange.m_end );
				TokenRange templateInstanceArgsRange;

				if ( !tqParser.FindArgList( &templateInstanceArgsRange, outerClass.m_start, '<' ) || templateInstanceArgsRange.m_start > outerClass.m_end )
				{
					std::tstring templateInstanceArgs = buildTemplateInstanceTypeList( TokenRange( m_templateDecl ), m_templateDecl.c_str() );

					endOfOuterClassRange.m_end = endOfOuterClassRange.m_start;
					endOfOuterClassRange.ReplaceWithToken( &m_typeQualifier, templateInstanceArgs );
				}
			}
		}
	}

	//	Type descriptor syntax:
	//		"[[templateDecl]\r\n][typeQualifier]"
	//
	void CTypeDescriptor::Split( const TCHAR* pTypeDescriptor ) throws_( CRuntimeException )
	{
		ASSERT_PTR( pTypeDescriptor );

		const TCHAR* pLead = pTypeDescriptor;
		pTypeDescriptor = str::SkipWhitespace( pTypeDescriptor, _T(" \t") );	// skip leading indent prefix (if any)

		if ( pTypeDescriptor != pLead )
			m_indentPrefix.assign( pLead, pTypeDescriptor );

		std::vector<std::tstring> items;

		str::Split( items, pTypeDescriptor, code::lineEnd );
		switch ( items.size() )
		{
			case 0:
				break;
			case 2:
				m_templateDecl = items.front();
				// fall-through
			case 1:
				m_typeQualifier = items.back();
				break;
			default:
				throw CRuntimeException( str::Format( _T("Bad type descriptor/qualifier: '%s'"), pTypeDescriptor ) );
		}
	}

	CString CTypeDescriptor::buildTemplateInstanceTypeList( const TokenRange& templateDecl, const TCHAR* pMethodPrototype ) const
	{
		// example: converts "template< typename Type, class MyClass, struct MyStruct >" to "< Type, MyClass, MyStruct >"
		CString outTemplateInstanceList;
		BraceParityStatus braceStatus;
		TokenRange templTypeList = braceStatus.findArgList( pMethodPrototype, templateDecl.m_start, _T("<"), m_pFmt->getDocLanguage() );

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
			const TCHAR* pSpacer = InsertOneSpace == m_pFmt->MustSpaceBrace( _T('<') ) ? _T(" ") : _T("");

			tiList.Format( _T("<%s%s%s>"), pSpacer, (LPCTSTR)outTemplateInstanceList, pSpacer );
			outTemplateInstanceList = tiList;
		}

		return outTemplateInstanceList;
	}
}
