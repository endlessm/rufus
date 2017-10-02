#pragma once
#include <memory>
#include "PGPSignature.h"

typedef void(*UnpackProgressCallback)(const uint64_t current_bytes, const uint64_t total_bytes);

class EndlessISO {
public:
    EndlessISO();
    virtual ~EndlessISO();
    bool Init();
    void Uninit();

    ULONGLONG GetExtractedSize(const CString &image, const BOOL isInstallerImage);
    bool UnpackSquashFS(const CString &image, const CString &destination, UnpackProgressCallback callback, HANDLE cancelEvent);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;

    class ProgressCallback;
};