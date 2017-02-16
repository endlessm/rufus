#include "stdafx.h"
#include "SevenZipLibrary.h"
#include "GUIDs.h"

// This is the entrypoint to 7z, defined in
// src/7z/CPP/7zip/Archive/DllExports2.cpp. As the name suggests, it's normally
// exported as a DLL symbol and looked up with GetProcAddress(), so does not
// appear in 7z's headers.
STDAPI CreateObject(const GUID *clsid, const GUID *iid, void **outObject);

namespace SevenZip
{

SevenZipLibrary::SevenZipLibrary()
{
}

SevenZipLibrary::~SevenZipLibrary()
{
	Free();
}

bool SevenZipLibrary::Load()
{
	return true;
}

void SevenZipLibrary::Free()
{
}

bool SevenZipLibrary::CreateObject( const GUID& clsID, const GUID& interfaceID, void** outObject ) const
{
	HRESULT hr = ::CreateObject( &clsID, &interfaceID, outObject );
	if ( FAILED( hr ) )
	{
		return false;
		//throw SevenZipException( GetCOMErrMsg( _T( "CreateObject" ), hr ) );
	}
	return true;
}

}
