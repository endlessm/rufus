#pragma once

#include "SevenZipLibrary.h"
#include <atlbase.h>
#include "FileInfo.h"
#include "CompressionFormat.h"
#include "CompressionLevel.h"

struct IInArchive;

namespace SevenZip
{
	class SevenZipArchive
	{
	public:
		SevenZipArchive(const SevenZipLibrary& library, const TString& archivePath);
		virtual ~SevenZipArchive();

		virtual void SetCompressionFormat(const CompressionFormatEnum& format);
		virtual CompressionFormatEnum GetCompressionFormat();

		virtual void SetCompressionLevel(const CompressionLevelEnum& level);
		virtual CompressionLevelEnum GetCompressionLevel();

		virtual bool DetectCompressionFormat();

		virtual size_t GetNumberOfItems();
		virtual std::vector<TString> GetItemsNames();
		virtual std::vector<UINT64>  GetOrigSizes();

	protected:
		bool EnsureInArchive();
		virtual bool ReadInArchiveMetadata();

		bool m_ReadMetadata = false;
		bool m_OverrideCompressionFormat = false;
		const SevenZipLibrary& m_library;
		TString m_archivePath;
		CComPtr<IInArchive> m_inArchive = NULL;
		CompressionFormatEnum m_compressionFormat;
		CompressionLevelEnum m_compressionLevel;
		size_t m_numberofitems = 0;
		std::vector<TString> m_itemnames;
		std::vector<UINT64> m_origsizes;

	private:
		bool pri_GetItemsNames();
		bool pri_DetectCompressionFormat(CompressionFormatEnum & format);
		bool pri_DetectCompressionFormat();
	};
}
