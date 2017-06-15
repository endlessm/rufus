#include "stdafx.h"
#include "EndlessISO.h"

#include "7zpp/7zpp.h"
#include "GeneralCode.h"
#include "Images.h"
#include "StringHelperMethods.h"
#include "gpt/gpt.h"

class EndlessISO::Impl {
public:
    SevenZip::SevenZipLibrary sevenZip;
};

class EndlessISO::ProgressCallback
    : public SevenZip::ProgressCallback {
public:
    ProgressCallback(UnpackProgressCallback callback, HANDLE cancelEvent)
        : SevenZip::ProgressCallback(),
        m_totalBytes(0),
        m_callback(callback),
        m_cancelEvent(cancelEvent) {}

    virtual void OnStartWithTotal(SevenZip::TString filePath, unsigned __int64 totalBytes) {
        m_totalBytes = totalBytes;
    }

    virtual bool OnProgress(SevenZip::TString filePath, unsigned __int64 bytesCompleted) {
        if (WaitForSingleObject(m_cancelEvent, 0) == WAIT_OBJECT_0) {
            return false;
        }

        if (m_totalBytes > 0) {
            // 7z's API doesn't specify whether bytesCompleted is measured in
            // terms of the input or output data, and it in practice varies by
            // format. For SquashFS it happens to be decompressed bytes. We
            // pass both values to the callback in case a future format is
            // different; the callback only really cares about a %age.
            m_callback(bytesCompleted, m_totalBytes);
        }

        return true;
    }

private:
    UnpackProgressCallback m_callback;
    HANDLE m_cancelEvent;
    UINT64 m_totalBytes;
};

EndlessISO::EndlessISO()
    : pImpl(std::make_unique<Impl>())
{
}

EndlessISO::~EndlessISO()
{
}

bool EndlessISO::Init()
{
    bool sevenZipLoaded = pImpl->sevenZip.Load();
    return sevenZipLoaded;
}

void EndlessISO::Uninit()
{
    pImpl->sevenZip.Free();
}

ULONGLONG EndlessISO::GetExtractedSize(const CString & image, const BOOL isInstallerImage)
{
    auto extractor = SevenZip::SevenZipExtractor(pImpl->sevenZip, image.GetString());
    extractor.SetCompressionFormat(SevenZip::CompressionFormat::SquashFS);

    auto n = extractor.GetNumberOfItems();
    IFFALSE_RETURN_VALUE(n == 1, "not exactly 1 item in squashfs image", 0);
    UINT32 i = 0;

    auto name = extractor.GetItemsNames()[i];
    IFFALSE_RETURN_VALUE(name == _T(ENDLESS_IMG_FILE_NAME), "squashfs item has wrong name", 0);

    struct ptable pt;
    auto gotPt = extractor.ExtractBytes(i, &pt, sizeof(pt));
    IFFALSE_RETURN_VALUE(gotPt, "Failed to read partition table", 0);

    if (!is_eos_gpt_valid(&pt, isInstallerImage)) {
        return 0;
    }

    return get_disk_size(&pt);
}

static_assert(sizeof(size_t) == sizeof(UINT32), "this is assumed below");

bool EndlessISO::VerifySquashFS(const CString & image, const CString & signatureFilename, HashingCallback_t hashingCallback, LPVOID hashingContext)
{
    FUNCTION_ENTER;

    auto extractor = SevenZip::SevenZipExtractor(pImpl->sevenZip, image.GetString());
    extractor.SetCompressionFormat(SevenZip::CompressionFormat::SquashFS);

    UINT32 i = 0;
    auto size = extractor.GetOrigSizes()[i];
    std::unique_ptr<SevenZip::SevenZipExtractStream> stream(extractor.ExtractStream(i));

    auto reader = [&](void *buf, size_t bytes) {
        return stream->Read(buf, (UINT32)bytes);
    };
    return VerifyStream(reader, size, signatureFilename, hashingCallback, hashingContext);
}

bool EndlessISO::UnpackSquashFS(const CString & image, const CString & destination, UnpackProgressCallback callback, HANDLE cancelEvent)
{
    FUNCTION_ENTER;

    // TODO: assumes the squashfs contains exactly one file with the correct name
    const CString destinationDir = CSTRING_GET_PATH(destination, L'\\');

    auto extractor = SevenZip::SevenZipExtractor(pImpl->sevenZip, image.GetString());
    extractor.SetCompressionFormat(SevenZip::CompressionFormat::SquashFS);
    auto cb = ProgressCallback(callback, cancelEvent);
    auto ret = extractor.ExtractArchive(destinationDir.GetString(), &cb);
    IFFALSE_RETURN_VALUE(ret, "ExtractArchive returned false", false);

    return true;
}
