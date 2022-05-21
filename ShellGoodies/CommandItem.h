#ifndef CommandItem_h
#define CommandItem_h
#pragma once

#include "utl/ICommand.h"
#include "utl/Subject.h"


class CToolStrip;


class CCommandItem : public TSubject		// proxy items to be inserted into the list control
{
public:
	CCommandItem( utl::ICommand* pCmd = NULL ) { SetCmd( pCmd ); }

	utl::ICommand* GetCmd( void ) const { return m_pCmd; }
	void SetCmd( utl::ICommand* pCmd );

	template< typename Cmd_T >
	Cmd_T* GetCmdAs( void ) const { return dynamic_cast<Cmd_T*>( m_pCmd ); }

	int GetImageIndex( void ) const { return m_imageIndex; }

	// ISubject interface
	virtual const std::tstring& GetCode( void ) const;

	static CImageList* GetImageList( void );

	struct ToCmd
	{
		utl::ICommand* operator()( const CCommandItem* pCmdItem ) const
		{
			return pCmdItem->GetCmd();
		}
	};
private:
	static CToolStrip& GetCmdTypeStrip( void );
	static int LookupImageIndex( utl::ICommand* pCmd );
private:
	utl::ICommand* m_pCmd;
	int m_imageIndex;
	std::tstring m_code;
};


#endif // CommandItem_h
