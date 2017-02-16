#pragma once

#include "SevenZipException.h"
#include "CompressionFormat.h"

namespace SevenZip
{
	class SevenZipLibrary
	{
	public:

		SevenZipLibrary();
		~SevenZipLibrary();

		bool Load();
		void Free();

		bool CreateObject( const GUID& clsID, const GUID& interfaceID, void** outObject ) const;
	};
}
