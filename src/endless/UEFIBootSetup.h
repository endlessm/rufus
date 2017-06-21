#pragma once

bool EFIRequireNeededPrivileges();
bool EFICreateNewEntry(const wchar_t *drive, wchar_t *path, wchar_t *desc);
bool EFIRemoveEntry(wchar_t *desc, bool &found_entry);