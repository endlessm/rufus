#pragma once


#include "SevenZipLibrary.h"
#include "SevenZipArchive.h"
#include "CompressionFormat.h"
#include "ProgressCallback.h"

struct ISequentialInStream;


namespace SevenZip
{
	class SevenZipExtractStream
	{
	public:
		virtual ~SevenZipExtractStream() {}
		virtual UINT32 Read(void *buf, UINT32 size) = 0;
	};

	class SevenZipExtractor : public SevenZipArchive
	{
	public:

		SevenZipExtractor( const SevenZipLibrary& library, const TString& archivePath );
		SevenZipExtractor( const SevenZipLibrary& library, IInStream *stream, CompressionFormatEnum format );
		virtual ~SevenZipExtractor();

		virtual bool ExtractArchive( const TString& directory, ProgressCallback* callback);
		virtual bool ExtractBytes(const UINT32 index, void *data, const UINT32 size);

		virtual SevenZipExtractStream * ExtractStream(const UINT32 index);
		virtual SevenZipExtractor * GetSubArchive(const UINT32 index, const CompressionFormatEnum format);

	private:
		ISequentialInStream *GetStream(const UINT32 index);

		bool ExtractArchive( const CComPtr< IStream >& archiveStream, const TString& directory, ProgressCallback* callback);
	};
}
