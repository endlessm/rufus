#include "stdafx.h"
#include "GeneralCode.h"

void _print_hresult(const char *file, int line, const char *msg, HRESULT hr)
{
    _com_error err(hr);
    LPCTSTR errMsg = err.ErrorMessage();

    uprintf("%s:%d %s (HRESULT: 0x%x %ls)", file, line, msg, hr, errMsg);
}
