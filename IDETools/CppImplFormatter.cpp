
#include "pch.h"
#include "IdeUtilities.h"
#include "CppImplFormatter.h"
#include "IterationSlices.h"
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

#include "utl/Language.hxx"
#include "CppParser.hxx"


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

		m_voidFunctionBody = _T("\
{\r\n\
}\r\n");
		m_returnFunctionBody = _T("\
{\r\n\
\treturn ?;\r\n\
}\r\n");
	}

	CCppImplFormatter::~CCppImplFormatter()
	{
	}

	bool CCppImplFormatter::IsCppTypeQualifier( std::tstring typeQualifier )
	{
		str::Replace( typeQualifier, _T("::"), _T("") );
		return word::IsAlphaNumericWord( typeQualifier );
	}

	std::tstring CCppImplFormatter::ImplementMethodBlock( const TCHAR* pMethodPrototypes, const TCHAR* pTypeDescriptor, bool isInline ) throws_( CRuntimeException )
	{
		resetInternalState();
		if ( !m_options.m_testMode )
			LoadCodeSnippets();

		CTypeDescriptor tdInfo( this, isInline );

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
			if ( !protoParser.FindNextSequence( &endOfLine, argList.m_end, code::g_pLineEnd ) )
				break;					// we're done with last line

			endOfLine.ReplaceWithToken( &prototypeBlock, s_protoSep.c_str() );
			pos = endOfLine.m_end;		// skip to next line
		}

		std::vector<std::tstring> prototypes;
		str::Split( prototypes, prototypeBlock.c_str(), s_protoSep.c_str());

		std::tstring implementedMethods;

		for ( std::vector<std::tstring>::const_iterator itProto = prototypes.begin(); itProto != prototypes.end(); ++itProto )
			if ( !itProto->empty() )
			{
				std::tstring implementation = ImplementMethod( *itProto, tdInfo );

				if ( !implementation.empty() )
					implementedMethods += implementation;
			}

		tdInfo.IndentCode( &implementedMethods );
		return implementedMethods;
	}

	bool CCppImplFormatter::LoadCodeSnippets( void )
	{
		TextContent codeSnippets;
		std::tstring filePath = app::GetModuleSession().m_codeTemplatePath.Get();

		if ( app::GetModuleSession().m_useCommentDecoration )
		{
			if ( codeSnippets.LoadFileSection( filePath.c_str(), _T("Single Line Decoration") ) )
				m_commentDecorationTemplate = codeSnippets.GetText();
		}

		if ( codeSnippets.LoadFileSection( filePath.c_str(), _T("void Function Body") ) )
			m_voidFunctionBody = codeSnippets.GetText();

		if ( codeSnippets.LoadFileSection( filePath.c_str(), _T("return Function Body") ) )
			m_returnFunctionBody = codeSnippets.GetText();

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
			"template<>\r\nMyClass< std::string, LessThanString, 5 >::"
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
			typeDescriptor = templateDecl + code::g_pLineEnd;

		typeDescriptor += typeQualifier;
		return typeDescriptor;
	}

	std::tstring CCppImplFormatter::ImplementMethod( const std::tstring& methodProto, const CTypeDescriptor& tdInfo )
	{
		std::tstring srcPrototype = makeNormalizedFormattedPrototype( methodProto.c_str(), true );

		if ( srcPrototype.empty() )
			return srcPrototype;

		const CCppParser cppParser;
		srcPrototype = cppParser.MakeRemoveDefaultParams( srcPrototype, m_options.m_commentOutDefaultParams );

		CMethodPrototype method;

		method.ParseCode( srcPrototype );		//m_validArgListOpenBraces

		std::tstring returnType = tdInfo.m_inlinePrefix;
		bool hasNoReturnType = true;

		if ( !method.m_returnType.IsEmpty() )
		{
			std::tstring srcReturnType = method.m_returnType.MakeToken( srcPrototype );

			hasNoReturnType = ( srcReturnType.empty() || srcReturnType == _T("void") );

			returnType += srcReturnType;

			returnType += m_options.m_returnTypeOnSeparateLine ? code::g_pLineEnd : _T(" ");
		}

		std::tstring implMethod;

		if ( !m_options.m_returnTypeOnSeparateLine && !returnType.empty() )
			implMethod = returnType;

		implMethod += tdInfo.m_typeQualifier;

		implMethod += TokenRange( method.m_qualifiedMethod.m_start, method.m_postArgListSuffix.m_end ).MakeToken( srcPrototype );
		implMethod += code::g_pLineEnd;

		if ( !m_options.m_testMode )		// factor-out splitting in test mode
			implMethod = splitArgumentList( implMethod.c_str() );

		if ( m_options.m_returnTypeOnSeparateLine && !returnType.empty() )
			implMethod = returnType + implMethod;

		if ( !tdInfo.m_templateDecl.empty() )
			implMethod = tdInfo.m_templateDecl + code::g_pLineEnd + implMethod;

		if ( !m_commentDecorationTemplate.empty() )
		{
			std::tstring decorationCore = tdInfo.m_typeQualifier + TokenRange( method.m_qualifiedMethod.m_start, method.m_qualifiedMethod.m_end ).MakeToken( srcPrototype );
			std::tstring commentDecoration = MakeCommentDecoration( decorationCore );

			if ( !commentDecoration.empty() )
				implMethod = commentDecoration + code::g_pLineEnd + implMethod;
		}

		implMethod += hasNoReturnType ? m_voidFunctionBody : m_returnFunctionBody;

		for ( int i = 0; i != m_options.m_linesBetweenFunctionImpls; ++i )
			implMethod += code::g_pLineEnd;

		return implMethod;
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
				if ( IsCppTypeQualifier( clipText ) )
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


	std::tstring CCppImplFormatter::AutoMakeCode( const TCHAR* pCodeText )
	{
		resetInternalState();

		std::tstring outCode;
		UINT commandId = ui::TrackMakeCodeTemplate();

		switch ( commandId )
		{
			case CM_MAKE_LOOP_const_iterator:
			case CM_MAKE_LOOP_iterator:
				outCode = MakeIteratorLoop( pCodeText, CM_MAKE_LOOP_const_iterator == commandId );
				break;
			case CM_MAKE_LOOP_index:
				outCode = MakeIndexLoop( pCodeText );
				break;
		}

		return outCode;
	}

	std::tstring CCppImplFormatter::MakeIteratorLoop( const TCHAR* pCodeText, bool isConstIterator ) throws_( CRuntimeException )
	{
		CIterationSlices slices;
		slices.ParseCode( pCodeText );

		TextContent codeSnippets;
		std::tstring outCode;

		if ( codeSnippets.LoadFileSection( app::GetModuleSession().m_codeTemplatePath.GetPtr(), _T("ForLoopIterator") ) )
		{
			codeSnippets.ReplaceText( _T("%ContainerType%"), slices.m_containerType.GetToken( pCodeText ).c_str(), TRUE);
			codeSnippets.ReplaceText( _T("%IteratorType%"), isConstIterator ? _T("const_iterator") : _T("iterator"), TRUE );
			codeSnippets.ReplaceText( _T("%IteratorName%"), slices.m_iteratorName.c_str(), TRUE);
			codeSnippets.ReplaceText( _T("%Container%"), slices.m_containerName.GetToken( pCodeText ).c_str(), TRUE );
			codeSnippets.ReplaceText( _T("%Selector%"), slices.m_pObjSelOp, TRUE );
			codeSnippets.ReplaceText( _T("%LeadingSpaces%"), slices.m_leadingWhiteSpace.GetToken( pCodeText ).c_str(), TRUE );

			outCode = codeSnippets.GetText();
		}

		return outCode;
	}

	std::tstring CCppImplFormatter::MakeIndexLoop( const TCHAR* pCodeText ) throws_( CRuntimeException )
	{
		CIterationSlices slices;
		slices.ParseCode( pCodeText );

		TextContent codeSnippets;
		std::tstring outCode;

		if ( codeSnippets.LoadFileSection( app::GetModuleSession().m_codeTemplatePath.GetPtr(), slices.m_isMfcList ? _T("ForLoopPosition") : _T("ForLoopIndex") ) )
		{
			codeSnippets.ReplaceText( _T("%IndexType%"), CIterationSlices::STL == slices.m_libraryType ? _T("size_t") : _T("int"), TRUE );
			codeSnippets.ReplaceText( _T("%ObjectType%"), slices.m_valueType.getString( pCodeText ), TRUE );
			codeSnippets.ReplaceText( _T("%ObjectName%"), slices.m_iteratorName.c_str(), TRUE);
			codeSnippets.ReplaceText( _T("%Index%"), _T("i"), TRUE );
			codeSnippets.ReplaceText( _T("%Container%"), slices.m_containerName.getString( pCodeText ), TRUE );
			codeSnippets.ReplaceText( _T("%Selector%"), slices.m_pObjSelOp, TRUE );
			codeSnippets.ReplaceText( _T("%GetSizeMethod%"), CIterationSlices::MFC == slices.m_libraryType ? _T("GetSize") : _T("size"), TRUE );
			codeSnippets.ReplaceText( _T("%LeadingSpaces%"), slices.m_leadingWhiteSpace.getString( pCodeText ), TRUE );

			outCode = codeSnippets.GetText();
		}

		return outCode;
	}

	std::tstring CCppImplFormatter::TokenizeText( const TCHAR* pCodeText )
	{
		ide::CScopedWindow scopedIDE;
		CTokenizeTextDialog dialog( scopedIDE.GetMainWnd() );
		dialog.m_sourceText = pCodeText;

		if ( IDCANCEL == dialog.DoModal() )
			return str::GetEmpty();

		return dialog.m_outputText;
	}

} // namespace code


namespace code
{
	// CTypeDescriptor implementation

	CTypeDescriptor::CTypeDescriptor( const CFormatter* pFmt, bool isInline )
		: m_pFmt( pFmt )
	{
		ASSERT_PTR( pFmt );

		if ( isInline )
			m_inlinePrefix = _T("inline ");
	}

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
					std::tstring templateInstanceArgs = tqParser.ExtractTemplateInstance( m_templateDecl, m_pFmt->MustSpaceBrace( _T('<') ) );

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

		str::Split( items, pTypeDescriptor, code::g_pLineEnd );
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
				throw CRuntimeException( str::Format( _T("Bad type descriptor/qualifier: '%s'"), pTypeDescriptor ), UTL_FILE_LINE );
		}
	}

	void CTypeDescriptor::IndentCode( std::tstring* pCodeText ) const
	{
		ASSERT_PTR( pCodeText );

		if ( m_indentPrefix.empty() )
			return;			// nothing to change

		std::vector<std::tstring> lines;
		str::Split( lines, pCodeText->c_str(), code::g_pLineEnd );

		for ( std::vector<std::tstring>::iterator itLine = lines.begin(); itLine != lines.end(); ++itLine )
			if ( !itLine->empty() )
				itLine->insert( 0, m_indentPrefix );		// indent only non-empty lines

		*pCodeText = str::Join( lines, code::g_pLineEnd );
	}
}
