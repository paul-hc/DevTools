
#include "stdafx.h"
#include "ModelSchema.h"
#include "utl/UI/MfcUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace app
{
	ModelSchema GetLoadingSchema( const CArchive& rArchive )
	{
		return serial::CScopedLoadingArchive::GetModelSchema< app::ModelSchema >( rArchive );
	}

	std::tstring FormatModelVersion( ModelSchema modelSchemaVersion )
	{
		if ( Slider_v3_1 == modelSchemaVersion )
			return _T("v3.1 (-)");			// avoid reporting "Slider v0.0"

		return str::Format( _T("v%x.%x"), ( modelSchemaVersion & 0xF0 ) >> 4, modelSchemaVersion & 0x0F );
	}

	std::tstring FormatSliderVersion( ModelSchema modelSchemaVersion )
	{
		static const std::tstring s_appName = _T("Slider ");
		return s_appName + FormatModelVersion( modelSchemaVersion );
	}
}
