#include "stdafx.h"
#include "SevenZipArchive.h"
#include "UsefulFunctions.h"


namespace SevenZip
{
	SevenZipArchive::SevenZipArchive(const SevenZipLibrary& library, const TString& archivePath)
		: m_library(library),
		m_archivePath(archivePath),
		// The default compression type will be unknown
		m_compressionFormat(CompressionFormat::Unknown),
		m_compressionLevel(CompressionLevel::None),
		m_inArchive(NULL)
	{
	}

	SevenZipArchive::~SevenZipArchive()
	{
		m_itemnames.clear();
		m_origsizes.clear();

		if (m_inArchive != NULL) {
			m_inArchive->Close();
		}
	}

	void SevenZipArchive::SetCompressionFormat(const CompressionFormatEnum& format)
	{
		m_OverrideCompressionFormat = true;
		m_ReadMetadata = false;
		m_compressionFormat = format;
	}

	void SevenZipArchive::SetCompressionLevel(const CompressionLevelEnum& level)
	{
		m_compressionLevel = level;
	}

	CompressionLevelEnum SevenZipArchive::GetCompressionLevel()
	{
		return m_compressionLevel;
	}

	CompressionFormatEnum SevenZipArchive::GetCompressionFormat()
	{
		if (!m_ReadMetadata && !m_OverrideCompressionFormat)
		{
			m_ReadMetadata = ReadInArchiveMetadata();
		}
		return m_compressionFormat;
	}

	size_t SevenZipArchive::GetNumberOfItems()
	{
		if (!m_ReadMetadata)
		{
			m_ReadMetadata = ReadInArchiveMetadata();
		}
		return m_numberofitems;
	}

	std::vector<TString> SevenZipArchive::GetItemsNames()
	{
		if (!m_ReadMetadata)
		{
			m_ReadMetadata = ReadInArchiveMetadata();
		}
		return m_itemnames;
	}

	std::vector<UINT64> SevenZipArchive::GetOrigSizes()
	{
		if (!m_ReadMetadata)
		{
			m_ReadMetadata = ReadInArchiveMetadata();
		}
		return m_origsizes;
	}

	// Ensures that m_inArchive is non-NULL and open; returns FALSE if this is
	// not possible.
	bool SevenZipArchive::EnsureInArchive()
	{
		if (m_inArchive) {
			return true;
		}

		if (m_compressionFormat == CompressionFormat::Unknown && !m_OverrideCompressionFormat) {
			if (!pri_DetectCompressionFormat()) {
				return false;
			}
		}

		CComPtr< IInStream > inFile = FileSys::OpenFileToRead(m_archivePath);

		if (inFile == NULL)
		{
			return false;
			//throw SevenZipException( StrFmt( _T( "Could not open archive \"%s\"" ), m_archivePath.c_str() ) );
		}

		m_inArchive = UsefulFunctions::GetArchiveReader(m_library, m_compressionFormat);
		CComPtr< ArchiveOpenCallback > openCallback = new ArchiveOpenCallback();

		HRESULT hr = m_inArchive->Open(inFile, 0, openCallback);
		if (hr != S_OK)
		{
			m_inArchive = NULL;
			return false;
			//throw SevenZipException( GetCOMErrMsg( _T( "Open archive" ), hr ) );
		}

		return true;
	}

	// Sets up all the metadata for an archive file
	bool SevenZipArchive::ReadInArchiveMetadata()
	{
		return EnsureInArchive() && pri_GetItemsNames();
	}

	bool SevenZipArchive::DetectCompressionFormat()
	{
		m_OverrideCompressionFormat = false;
		return pri_DetectCompressionFormat();
	}

	bool SevenZipArchive::pri_DetectCompressionFormat()
	{
		m_OverrideCompressionFormat = false;
		return pri_DetectCompressionFormat(m_compressionFormat);
	}

	bool SevenZipArchive::pri_DetectCompressionFormat(CompressionFormatEnum & format)
	{
		m_OverrideCompressionFormat = false;
		return UsefulFunctions::DetectCompressionFormat(m_library, m_archivePath, format);
	}

	bool SevenZipArchive::pri_GetItemsNames()
	{
		return UsefulFunctions::GetItemsNames(m_inArchive,
			m_numberofitems, m_itemnames, m_origsizes);
	}
}

