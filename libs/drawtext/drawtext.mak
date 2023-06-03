# Microsoft Developer Studio Generated NMAKE File, Based on drawtext.dsp
!IF "$(CFG)" == ""
CFG=drawtext - Win32 Debug
!MESSAGE No configuration specified. Defaulting to drawtext - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "drawtext - Win32 Release" && "$(CFG)" != "drawtext - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "drawtext.mak" CFG="drawtext - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "drawtext - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "drawtext - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "drawtext - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\drawtext.lib"


CLEAN :
	-@erase "$(INTDIR)\draw.obj"
	-@erase "$(INTDIR)\drawgl.obj"
	-@erase "$(INTDIR)\drawrast.obj"
	-@erase "$(INTDIR)\font.obj"
	-@erase "$(INTDIR)\utf8.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\drawtext.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "NO_FREETYPE" /D "NO_OPENGL" /Fp"$(INTDIR)\drawtext.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\drawtext.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\drawtext.lib" 
LIB32_OBJS= \
	"$(INTDIR)\draw.obj" \
	"$(INTDIR)\drawgl.obj" \
	"$(INTDIR)\drawrast.obj" \
	"$(INTDIR)\font.obj" \
	"$(INTDIR)\utf8.obj"

"$(OUTDIR)\drawtext.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "drawtext - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\drawtext.lib"


CLEAN :
	-@erase "$(INTDIR)\draw.obj"
	-@erase "$(INTDIR)\drawgl.obj"
	-@erase "$(INTDIR)\drawrast.obj"
	-@erase "$(INTDIR)\font.obj"
	-@erase "$(INTDIR)\utf8.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\drawtext.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "NO_FREETYPE" /D "NO_OPENGL" /Fp"$(INTDIR)\drawtext.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\drawtext.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\drawtext.lib" 
LIB32_OBJS= \
	"$(INTDIR)\draw.obj" \
	"$(INTDIR)\drawgl.obj" \
	"$(INTDIR)\drawrast.obj" \
	"$(INTDIR)\font.obj" \
	"$(INTDIR)\utf8.obj"

"$(OUTDIR)\drawtext.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("drawtext.dep")
!INCLUDE "drawtext.dep"
!ELSE 
!MESSAGE Warning: cannot find "drawtext.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "drawtext - Win32 Release" || "$(CFG)" == "drawtext - Win32 Debug"
SOURCE=.\draw.c

"$(INTDIR)\draw.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\drawgl.c

"$(INTDIR)\drawgl.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\drawrast.c

"$(INTDIR)\drawrast.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\font.c

"$(INTDIR)\font.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\utf8.c

"$(INTDIR)\utf8.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

