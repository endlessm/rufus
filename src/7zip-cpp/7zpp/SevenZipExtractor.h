#pragma once


#include "SevenZipLibrary.h"
#include "SevenZipArchive.h"
#include "CompressionFormat.h"
#include "ProgressCallback.h"


namespace SevenZip
{
	class SevenZipExtractStream
	{
	public:
		virtual UINT32 Read(void *buf, UINT32 size) = 0;
	};

	class SevenZipExtractor : public SevenZipArchive
	{
	public:

		SevenZipExtractor( const SevenZipLibrary& library, const TString& archivePath );
		virtual ~SevenZipExtractor();

		virtual bool ExtractArchive( const TString& directory, ProgressCallback* callback);
		virtual bool ExtractBytes(const UINT32 index, void *data, const UINT32 size);

		virtual SevenZipExtractStream * ExtractStream(const UINT32 index);

	private:

		bool ExtractArchive( const CComPtr< IStream >& archiveStream, const TString& directory, ProgressCallback* callback);
	};
}
