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
			ExtractStream(CComPtr< ISequentialInStream > &seqInStream) :
				m_seqInStream(seqInStream)
			{
			}

			~ExtractStream()
			{
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
			CComPtr< ISequentialInStream > m_seqInStream;
		};
	}

	using namespace intl;

	SevenZipExtractor::SevenZipExtractor( const SevenZipLibrary& library, const TString& archivePath )
		: SevenZipArchive(library, archivePath)
	{
	}

	SevenZipExtractor::SevenZipExtractor(const SevenZipLibrary & library, IInStream * stream, CompressionFormatEnum format)
		: SevenZipArchive(library, stream, format)
	{
	}

	SevenZipExtractor::~SevenZipExtractor()
	{
	}

	bool SevenZipExtractor::ExtractArchive( const TString& destDirectory, ProgressCallback* callback )
	{
		if (!EnsureInArchive()) {
			return false;
		}

		CComPtr< ArchiveExtractCallback > extractCallback = new ArchiveExtractCallback( m_inArchive, destDirectory, callback );

		HRESULT hr = m_inArchive->Extract( NULL, -1, false, extractCallback );
		if ( hr != S_OK ) // returning S_FALSE also indicates error
		{
			return false;	//Extract archive error
		}

		if (callback)
		{
			callback->OnDone(m_archivePath);
		}
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
		CComPtr< ISequentialInStream > seqInStream = GetStream(index);
		if (!seqInStream)
		{
			return nullptr;
		}

		return new intl::ExtractStream(seqInStream);
	}

	SevenZipExtractor * SevenZipExtractor::GetSubArchive(const UINT32 index, const CompressionFormatEnum format)
	{
		CComPtr< ISequentialInStream > seqInStream = GetStream(index);
		if (!seqInStream)
		{
			return nullptr;
		}

		CComPtr< IInStream > inStream;
		if (seqInStream->QueryInterface(IID_IInStream, (void**)&inStream) != S_OK || !inStream) {
			return nullptr;
		}

		// TODO: do we need to pass ourselves here to keep ourselves alive?
		return new SevenZipExtractor(m_library, inStream, format);
	}

	ISequentialInStream * SevenZipExtractor::GetStream(const UINT32 index)
	{
		if (!EnsureInArchive()) {
			return nullptr;
		}

		CComPtr< IInArchiveGetStream > getStream;
		if (m_inArchive->QueryInterface(IID_IInArchiveGetStream, (void**)&getStream) != S_OK || !getStream)
		{
			return nullptr;
		}

		ISequentialInStream * seqInStream = nullptr;
		if (getStream->GetStream(index, &seqInStream) != S_OK)
		{
			return nullptr;
		}

		return seqInStream;
	}

}
