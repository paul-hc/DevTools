
#include "pch.h"
#include "Command.h"
#include "CommandModel.h"
#include "Algorithms.h"
#include "EnumTags.h"
#include "Serialization.h"
#include "SerializeStdTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "Serialization.hxx"


// CBaseCommand implementation

CBaseCommand::CBaseCommand( int typeId, utl::ISubject* pSubject )
	: m_typeId( typeId )
	, m_pSubject( pSubject )
	, m_pOriginCmd( NULL )
{
}

int CBaseCommand::GetTypeID( void ) const override
{
	return m_typeId;
}

std::tstring CBaseCommand::FormatExecTitle( utl::Verbosity verbosity /*= utl::Detailed*/ ) const
{
	if ( HasOriginCmd() )		// this is a temporary command used to unexecute the origin command execute action?
		return m_pOriginCmd->FormatExecTitle( verbosity );

	return CCommandModel::PrefixExecMessage( Format( verbosity ) );
}

void CBaseCommand::NotifyObservers( void )
{
	if ( m_pSubject != NULL )
		m_pSubject->UpdateAllObservers( this );
}

void CBaseCommand::Serialize( CArchive& archive )
{
	archive & m_typeId;
}


// CCommand implementation

CCommand::CCommand( int typeId, utl::ISubject* pSubject, const CEnumTags* pCmdTags /*= NULL*/ )
	: CBaseCommand( typeId, pSubject )
	, m_pCmdTags( pCmdTags )
{
}

CCommand::~CCommand()
{
}

std::tstring CCommand::Format( utl::Verbosity verbosity ) const override
{
	if ( HasOriginCmd() )
		return GetOriginCmd()->Format( verbosity );

	ASSERT_PTR( m_pCmdTags );
	return m_pCmdTags->Format( GetTypeID(), verbosity != utl::Brief ? CEnumTags::UiTag : CEnumTags::KeyTag );
}

bool CCommand::Unexecute( void ) override
{
	ASSERT( false );
	return false;
}

bool CCommand::IsUndoable( void ) const override
{
	return true;
}


// CMacroCommand implementation

CMacroCommand::CMacroCommand( const std::tstring& userInfo /*= std::tstring()*/, int typeId /*= MacroCmdId*/ )
	: CBaseCommand( typeId, NULL )
	, m_userInfo( userInfo )
	, m_pMainCmd( NULL )
{
}

CMacroCommand::~CMacroCommand()
{
	utl::ClearOwningContainer( m_subCommands );
}

void CMacroCommand::AddCmd( utl::ICommand* pSubCmd )
{
	ASSERT_PTR( pSubCmd );
	ASSERT( !utl::Contains( m_subCommands, pSubCmd ) );

	m_subCommands.push_back( pSubCmd );
}

std::tstring CMacroCommand::Format( utl::Verbosity verbosity ) const override
{
	if ( HasOriginCmd() )
		return GetOriginCmd()->Format( verbosity );

	if ( m_pMainCmd != NULL )
		return m_pMainCmd->Format( verbosity );			// main commmand provides all the info

	std::tstring text;

	if ( !m_userInfo.empty() )
		text = m_userInfo + _T(":");

	if ( !IsEmpty() )
	{
		enum { MaxSubCmds = 5 };
		std::vector<std::tstring> cmdInfos; cmdInfos.reserve( m_subCommands.size() );

		size_t count = 0;
		for ( std::vector<utl::ICommand*>::const_iterator itSubCmd = m_subCommands.begin(); itSubCmd != m_subCommands.end(); ++itSubCmd, ++count )
			if ( utl::Detailed == verbosity || count != MaxSubCmds )
				cmdInfos.push_back( ( *itSubCmd )->Format( verbosity ) );
			else
			{
				cmdInfos.push_back( _T("...") );
				break;
			}

		static const TCHAR* s_pCmdSep[ 3 ] = { _T(" "), _T("\n"), _T("; ") };

		stream::Tag( text, str::Join( cmdInfos, s_pCmdSep[ verbosity ] ), utl::Detailed == verbosity ? _T("\n") : _T(" ") );
	}

	return text;
}

bool CMacroCommand::Execute( void ) override
{
	bool succeded = !m_subCommands.empty();

	for ( std::vector<utl::ICommand*>::const_iterator itSubCmd = m_subCommands.begin(); itSubCmd != m_subCommands.end(); ++itSubCmd )
		if ( !( *itSubCmd )->Execute() )
			succeded = false;

	return succeded;
}

bool CMacroCommand::Unexecute( void ) override
{
	bool succeded = !m_subCommands.empty();

	for ( std::vector<utl::ICommand*>::const_iterator itSubCmd = m_subCommands.begin(); itSubCmd != m_subCommands.end(); ++itSubCmd )
		if ( !( *itSubCmd )->Unexecute() )
			succeded = false;

	return succeded;
}

bool CMacroCommand::IsUndoable( void ) const override
{
	for ( std::vector<utl::ICommand*>::const_iterator itSubCmd = m_subCommands.begin(); itSubCmd != m_subCommands.end(); ++itSubCmd )
		if ( ( *itSubCmd )->IsUndoable() )
			return true;

	return false;
}

void CMacroCommand::Serialize( CArchive& archive )
{
	CBaseCommand::Serialize( archive );

	archive & &m_userInfo;
	serial::Serialize_CObjects_Mixed( archive, m_subCommands );
	serial::Serialize_CObject_Dynamic( archive, m_pMainCmd );
}
