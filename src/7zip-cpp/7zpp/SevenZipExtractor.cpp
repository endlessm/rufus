#include "stdafx.h"
#include "SevenZipExtractor.h"
#include "GUIDs.h"
#include "FileSys.h"
#include "ArchiveOpenCallback.h"
#include "ArchiveExtractCallback.h"
#include "PropVariant.h"
#include "UsefulFunctions.h"
#include <memory>


namespace SevenZip
{
	namespace intl {
		class ExtractStream : public SevenZip::SevenZipExtractStream {
		public:
			ExtractStream(CComPtr< IInArchive > &archive, CComPtr< ISequentialInStream > &seqInStream) :
				m_archive(archive),
				m_seqInStream(seqInStream)
			{
			}

			~ExtractStream()
			{
				m_archive->Close();
			}

			virtual UINT32 Read(void *buf, UINT32 size) {
				UInt32 sizeRead = 0;
				HRESULT res = m_seqInStream->Read(buf, size, &sizeRead);

				if (SUCCEEDED(res)) {
					return sizeRead;
				} else {
					return 0;
				}
			}

		private:
			CComPtr< IInArchive > m_archive;
			CComPtr< ISequentialInStream > m_seqInStream;
		};
	}

	using namespace intl;

	SevenZipExtractor::SevenZipExtractor( const SevenZipLibrary& library, const TString& archivePath )
		: SevenZipArchive(library, archivePath)
	{
	}

	SevenZipExtractor::~SevenZipExtractor()
	{
	}

	bool SevenZipExtractor::ExtractArchive( const TString& destDirectory, ProgressCallback* callback )
	{
		CComPtr< IInStream > inFile = FileSys::OpenFileToRead( m_archivePath );

		if ( inFile == NULL )
		{
			return false;	//Could not open archive
		}

		CComPtr< IInArchive > archive = UsefulFunctions::GetArchiveReader( m_library, m_compressionFormat );
		CComPtr< ArchiveOpenCallback > openCallback = new ArchiveOpenCallback();

		HRESULT hr = archive->Open( inFile, 0, openCallback );
		if ( hr != S_OK )
		{
			return false;	//Open archive error
		}

		CComPtr< ArchiveExtractCallback > extractCallback = new ArchiveExtractCallback( archive, destDirectory, callback );

		hr = archive->Extract( NULL, -1, false, extractCallback );
		if ( hr != S_OK ) // returning S_FALSE also indicates error
		{
			return false;	//Extract archive error
		}

		if (callback)
		{
			callback->OnDone(m_archivePath);
		}
		archive->Close();		
		return true;
	}

	bool SevenZipExtractor::ExtractBytes(const UINT32 index, void *data, const UINT32 size)
	{
		std::unique_ptr<SevenZipExtractStream> stream(ExtractStream(index));

		if (stream == nullptr) {
			return false;
		}

		UINT32 sizeRead = stream->Read(data, size);
		return sizeRead == size;
	}

	SevenZipExtractStream * SevenZipExtractor::ExtractStream(const UINT32 index)
	{
		CComPtr< IInStream > inFile = FileSys::OpenFileToRead(m_archivePath);

		if (inFile == NULL)
		{
			return nullptr;	//Could not open archive
		}

		CComPtr< IInArchive > archive = UsefulFunctions::GetArchiveReader(m_library, m_compressionFormat);
		CComPtr< ArchiveOpenCallback > openCallback = new ArchiveOpenCallback();

		HRESULT hr = archive->Open(inFile, 0, openCallback);
		if (hr != S_OK)
		{
			return nullptr;
		}

		CComPtr< IInArchiveGetStream > getStream;
		if (archive->QueryInterface(IID_IInArchiveGetStream, (void**)&getStream) != S_OK || !getStream)
		{
			archive->Close();
			return nullptr;
		}

		CComPtr< ISequentialInStream > seqInStream;
		if (getStream->GetStream(index, &seqInStream) != S_OK || !seqInStream)
		{
			archive->Close();
			return nullptr;
		}

		return new intl::ExtractStream(archive, seqInStream);
	}

}
