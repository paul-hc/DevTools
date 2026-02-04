#ifndef FlagSet_h
#define FlagSet_h
#pragma once


namespace utl
{
	// Manages a type-safe set of flags defined on EnumT flags enumerations of individual bit flags.
	//
	template< typename EnumT, typename UnderlyingT = int >
	class CFlagSet
	{
	public:
		CFlagSet( void ) : m_flags( UnderlyingT() ) {}
		CFlagSet( EnumT flag ) : m_flags( flag ) {}
		CFlagSet( const CFlagSet& src ) : m_flags( src.m_flags ) {}

		static CFlagSet Make( UnderlyingT flags ) { CFlagSet flagSet; flagSet.m_flags |= flags; return flagSet; }
		static CFlagSet MakeFlag( EnumT flag, bool on ) { CFlagSet flagSet; flagSet.Set( flag, on ); return flagSet; }

		UnderlyingT Value( void ) const { return m_flags; }

		bool Has( EnumT flag ) const { return ::HasFlag( m_flags, flag ); }
		bool HasAny( UnderlyingT flags ) const { return ::HasFlag( m_flags, flags ); }

		CFlagSet& Clear( EnumT flag ) { ::ClearFlag( m_flags, flag ); return *this; }
		CFlagSet& Set( EnumT flag, bool on = true ) { ::SetFlag( m_flags, flag, on ); return *this; }
		CFlagSet& Toggle( EnumT flag ) { ::ToggleFlag( m_flags, flag ); return *this; }

		CFlagSet& operator|=( const CFlagSet& right )
		{
			m_flags |= right.m_flags;
			return *this;
		}

		CFlagSet& operator&=( const CFlagSet& right )
		{
			m_flags &= right.m_flags;
			return *this;
		}

		CFlagSet operator~( void ) const
		{
			CFlagSet result( *this );
			result.m_flags = ~result.m_flags;
			return result;
		}

		friend CFlagSet operator|( const CFlagSet& left, const CFlagSet& right ) { return CFlagSet( left ) |= right; }
		friend CFlagSet operator&( const CFlagSet& left, const CFlagSet& right ) { return CFlagSet( left ) &= right; }
	protected:
		UnderlyingT m_flags;
	};
}


#endif // FlagSet_h
