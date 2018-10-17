PLATFORM = DOS
BUILD = DEBUG

!ifndef PLATFORM
!error No platform specified!
!endif

!ifndef BUILD
!error No build specified!
!endif

!ifeq PLATFORM WIN
!else
!ifeq PLATFORM DOS
!else
!error Invalid platform specified!
!endif
!endif

!ifeq BUILD DEBUG
!else
!ifeq BUILD RELEASE
!else
!error Invalid build config specified!
!endif
!endif

P = PLATFORM_$(PLATFORM)
OBJ_DIR = $(PLATFORM)\$(BUILD)

EXE_NAME = tx.exe

!ifeq PLATFORM WIN
CC = wcc386
!else
CC = wcc
!endif
INCLUDE_DOS = -I=PICOTCP
INCLUDE_WIN = -I=PICOTCP
INCLUDE = $(INCLUDE_$(PLATFORM))

CFLAGS_DOS = -bt=dos -ml
CFLAGS_WIN = -bt=nt
CFLAGS_PLATFORM = $(CFLAGS_$(PLATFORM))
CFLAGS_RELEASE = -d$(P) $(CFLAGS_PLATFORM) $(INCLUDE) -oxtl+ -s
CFLAGS_DEBUG  = -d$(P) $(CFLAGS_PLATFORM) $(INCLUDE) -dDEBUG -wx -we -db -d2 -j
CFLAGS = $(CFLAGS_$(BUILD))

CPPFLAGS = $(CPPFLAGS_$(BUILD))

LINK_FLAGS_DOS = system dos libpath PICOTCP libfile PICOTCPL.LIB libfile PICODOSL.LIB
LINK_FLAGS_WIN = system nt libfile wsock32.lib
LINK_FLAGS_DEBUG = debug all
LINK_FLAGS_RELEASE =
LINK_FLAGS = $(LINK_FLAGS_$(BUILD))
LINK_FLAGS += $(LINK_FLAGS_$(PLATFORM))

OBJS = $(OBJ_DIR)\main.obj

# Additional search dirs for source files.
.c: PICOTCP

prebuild: .SYMBOLIC
    @echo Building $(PLATFORM) $(BUILD)..
    @if not exist $(PLATFORM) mkdir $(PLATFORM)
    @if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)

all: prebuild $(OBJ_DIR)\$(EXE_NAME) .SYMBOLIC

$(OBJ_DIR)\$(EXE_NAME): $(OBJS)
    %write $*.lnk name $@
    %write $*.lnk $(LINK_FLAGS)
    %write $*.lnk file {$(OBJS)}
    wlink @$*.lnk

.c.obj: .AUTODEPEND
!ifeq PLATFORM WIN
    set wcc386=$(CFLAGS)
!endif
    $(CC) -ad=$*.d $< -fo=$@ -fr=$*.err

clean: .SYMBOLIC
    @echo Cleaning $(PLATFORM) $(BUILD)..
!ifdef __NT__
    @if exist $(OBJ_DIR) rmdir /s /q $(OBJ_DIR)
!else
    @if exist $(OBJ_DIR) del $(OBJ_DIR)\*.*
    @if exist $(OBJ_DIR) rmdir $(OBJ_DIR)
!endif
    @if exist *.err del *.err
