!IFNDEF ENDLESS_STATIC_BUILD
LIBS = $(LIBS) oleaut32.lib ole32.lib
!ENDIF

!IFNDEF MY_NO_UNICODE
CFLAGS = $(CFLAGS) -DUNICODE -D_UNICODE
!ENDIF

# CFLAGS = $(CFLAGS) -FAsc -Fa$O/Asm/

!IFNDEF O
!IFDEF CPU
O=$(CPU)
!ELSE
O=O
!ENDIF
!ENDIF

!IF "$(CPU)" == "AMD64"
MY_ML = ml64 -Dx64
!ELSEIF "$(CPU)" == "ARM"
MY_ML = armasm
!ELSE
MY_ML = ml
!IFDEF ENDLESS_STATIC_BUILD
# The asm code here does not contain any structured exception handlers, so mark
# the objects as containing no (unsafe) exception handlers. This allows the
# final executable to be linked with -safeseh.
MY_ML = $(MY_ML) -safeseh
!ENDIF
!ENDIF


!IFDEF UNDER_CE
RFLAGS = $(RFLAGS) -dUNDER_CE
!IFDEF MY_CONSOLE
LFLAGS = $(LFLAGS) /ENTRY:mainACRTStartup
!ENDIF
!ELSE
!IFNDEF NEW_COMPILER
LFLAGS = $(LFLAGS) -OPT:NOWIN98
!ENDIF
CFLAGS = $(CFLAGS) -Gr
!IFNDEF ENDLESS_STATIC_BUILD
LIBS = $(LIBS) user32.lib advapi32.lib shell32.lib
!ENDIF
!ENDIF

!IF "$(CPU)" == "ARM"
COMPL_ASM = $(MY_ML) $** $O/$(*B).obj
!ELSE
COMPL_ASM = $(MY_ML) -c -Fo$O/ $**
!ENDIF

CFLAGS = $(CFLAGS) -nologo -c -Fo$O/ -W4 -WX -EHsc -Gy -GR- -GF -Zi -Fd$O/7z.pdb

!IFDEF MY_STATIC_LINK
!IFNDEF MY_SINGLE_THREAD
!IFDEF MY_DEBUG
CFLAGS = $(CFLAGS) -MTd
!ELSE
CFLAGS = $(CFLAGS) -MT
!ENDIF
!ENDIF
!ELSE
CFLAGS = $(CFLAGS) -MD
!ENDIF

!IFDEF NEW_COMPILER
CFLAGS = $(CFLAGS) -GS- -Zc:forScope
!IFNDEF UNDER_CE
CFLAGS = $(CFLAGS) -MP2
!ENDIF
!ELSE
CFLAGS = $(CFLAGS)
!ENDIF

!IFDEF MY_DEBUG
CFLAGS_O1 = $(CFLAGS)
CFLAGS_O2 = $(CFLAGS)
!ELSE
CFLAGS_O1 = $(CFLAGS) -O1
CFLAGS_O2 = $(CFLAGS) -O2
!ENDIF

LFLAGS = $(LFLAGS) -nologo
!IFNDEF ENDLESS_STATIC_BUILD
LFLAGS = $(LFLAGS) -OPT:REF -OPT:ICF
!ENDIF

!IFNDEF UNDER_CE
!IFNDEF ENDLESS_STATIC_BUILD
LFLAGS = $(LFLAGS) /LARGEADDRESSAWARE
!ENDIF
!ENDIF

!IFDEF DEF_FILE
LFLAGS = $(LFLAGS) -DLL -DEF:$(DEF_FILE)
!ENDIF

MY_SUB_SYS_VER=6.0
!IFDEF MY_CONSOLE
# LFLAGS = $(LFLAGS) /SUBSYSTEM:console,$(MY_SUB_SYS_VER)
!ELSE
# LFLAGS = $(LFLAGS) /SUBSYSTEM:windows,$(MY_SUB_SYS_VER)
!ENDIF

PROGPATH = $O\$(PROG)

COMPL_O1   = $(CC) $(CFLAGS_O1) $**
COMPL_O2   = $(CC) $(CFLAGS_O2) $**
COMPL_PCH  = $(CC) $(CFLAGS_O1) -Yc"StdAfx.h" -Fp$O/a.pch $**
COMPL      = $(CC) $(CFLAGS_O1) -Yu"StdAfx.h" -Fp$O/a.pch $**

COMPLB    = $(CC) $(CFLAGS_O1) -Yu"StdAfx.h" -Fp$O/a.pch $<
# COMPLB_O2 = $(CC) $(CFLAGS_O2) -Yu"StdAfx.h" -Fp$O/a.pch $<
COMPLB_O2 = $(CC) $(CFLAGS_O2) $<

CCOMPL_PCH  = $(CC) $(CFLAGS_O2) -Yc"Precomp.h" -Fp$O/a.pch $**
CCOMPL_USE  = $(CC) $(CFLAGS_O2) -Yu"Precomp.h" -Fp$O/a.pch $**
CCOMPL      = $(CC) $(CFLAGS_O2) $**
CCOMPLB     = $(CC) $(CFLAGS_O2) $<


all: $(PROGPATH)

clean:
	-del /Q $(PROGPATH) $O\*.exe $O\*.dll $O\*.obj $O\*.lib $O\*.exp $O\*.res $O\*.pch $O\*.asm

$O:
	if not exist "$O" mkdir "$O"
$O/Asm:
	if not exist "$O/Asm" mkdir "$O/Asm"

!IFDEF ENDLESS_STATIC_BUILD
$(PROGPATH): $O $O/Asm $(OBJS) $(DEF_FILE)
	lib $(LFLAGS) -out:$(PROGPATH) $(OBJS) $(LIBS)
!ELSE
$(PROGPATH): $O $O/Asm $(OBJS) $(DEF_FILE)
       link $(LFLAGS) -out:$(PROGPATH) $(OBJS) $(LIBS)
!ENDIF


!IFNDEF NO_DEFAULT_RES
$O\resource.res: $(*B).rc
	rc $(RFLAGS) -fo$@ $**
!ENDIF
$O\StdAfx.obj: $(*B).cpp
	$(COMPL_PCH)
