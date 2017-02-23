#pragma once

#include <stdint.h>

// Needed for 'uprintf'
#ifdef __cplusplus
extern "C" {
#endif
#include "rufus.h"
#ifdef __cplusplus
}
#endif

// helper methods
#define TOSTR(value) case value: return _T(#value)

#define PRINT_ERROR_MSG(__ERROR_MSG__) uprintf("%s:%d %s (GLE=[%d])",__FILE__, __LINE__, __ERROR_MSG__, GetLastError())

void _print_hresult(const char *file, int line, const char *msg, HRESULT hr);
#define PRINT_HRESULT(hr, msg) _print_hresult(__FILE__, __LINE__, msg, hr)

#define IFFALSE_PRINTERROR(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { if(strlen(__ERRROR_MSG__)) PRINT_ERROR_MSG(__ERRROR_MSG__); }
#define IFFALSE_GOTOERROR(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { if(strlen(__ERRROR_MSG__)) PRINT_ERROR_MSG(__ERRROR_MSG__); goto error; }
#define IFFALSE_GOTO(__CONDITION__, __ERRROR_MSG__, __LABEL__) if(!(__CONDITION__)) { PRINT_ERROR_MSG(__ERRROR_MSG__); goto __LABEL__; }
#define IFFALSE_RETURN(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { PRINT_ERROR_MSG(__ERRROR_MSG__); return; }
#define IFFALSE_RETURN_VALUE(__CONDITION__, __ERRROR_MSG__, __RET__) if(!(__CONDITION__)) { PRINT_ERROR_MSG(__ERRROR_MSG__); return __RET__; }
#define IFFALSE_BREAK(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { PRINT_ERROR_MSG(__ERRROR_MSG__); break; }
#define IFFALSE_CONTINUE(__CONDITION__, __ERRROR_MSG__) if(!(__CONDITION__)) { PRINT_ERROR_MSG(__ERRROR_MSG__); continue; }

#define IFTRUE_GOTO(__CONDITION__, __ERRROR_MSG__, __LABEL__) if((__CONDITION__)) { PRINT_ERROR_MSG(__ERRROR_MSG__); goto __LABEL__; }
#define IFTRUE_GOTOERROR(__CONDITION__, __ERRROR_MSG__) if((__CONDITION__)) { PRINT_ERROR_MSG(__ERRROR_MSG__); goto error; }
#define IFTRUE_RETURN(__CONDITION__, __ERRROR_MSG__) if((__CONDITION__)) { PRINT_ERROR_MSG(__ERRROR_MSG__); return; }

#define IFFAILED_PRINTERROR(hr, msg)  { HRESULT __hr__ = (hr); if (FAILED(__hr__)) { PRINT_HRESULT(__hr__, msg); } }
#define IFFAILED_GOTO(hr, msg, label) { HRESULT __hr__ = (hr); if (FAILED(__hr__)) { PRINT_HRESULT(__hr__, msg); goto label; } }
#define IFFAILED_CONTINUE(hr, msg)    { HRESULT __hr__ = (hr); if (FAILED(__hr__)) { PRINT_HRESULT(__hr__, msg); continue; } }
#define IFFAILED_GOTOERROR(hr, msg)   IFFAILED_GOTO(hr, msg, error)

#define safe_closefile(__file__) if (__file__ != NULL) { fclose(__file__); __file__ = NULL; }

#define FUNCTION_ENTER uprintf("%s:%d %s", __FILE__, __LINE__, __FUNCTION__)
#define FUNCTION_ENTER_FMT(fmt, ...) uprintf("%s:%d %s " fmt, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)



#ifdef __cplusplus
// methods for adding our entry in BIOS EFI
bool EFIRequireNeededPrivileges();
bool EFICreateNewEntry(const wchar_t *drive, wchar_t *path, wchar_t *desc);
bool EFIRemoveEntry(wchar_t *desc);
#endif

