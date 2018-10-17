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

EXE_NAME = rx.exe

CPP = wpp

INCLUDE_DOS = -I=TCPINC -I=TCPLIB
INCLUDE_WIN = -I=PICOTCP
INCLUDE = $(INCLUDE_$(PLATFORM))

CFLAGS_DOS = -bt=dos -mc -DCFG_H="rx.cfg"
CFLAGS_WIN = -bt=nt
CFLAGS_PLATFORM = $(CFLAGS_$(PLATFORM))
CFLAGS_RELEASE = -d$(P) $(CFLAGS_PLATFORM) $(INCLUDE) -oxtl+ -s
#-oh -ok -ot -s -oa -ei -zp2 -zpw -we -ob -ol+ -oi+
CFLAGS_DEBUG  = -d$(P) $(CFLAGS_PLATFORM) $(INCLUDE) -dDEBUG -we -0
CFLAGS = $(CFLAGS_$(BUILD))

CPPFLAGS = $(CPPFLAGS_$(BUILD))

LINK_FLAGS_DOS = system dos
LINK_FLAGS_WIN = system nt libpath PICOTCP libfile PICOTCPL.LIB
LINK_FLAGS_DEBUG = debug all
LINK_FLAGS_RELEASE =
LINK_FLAGS = $(LINK_FLAGS_$(BUILD))
LINK_FLAGS += $(LINK_FLAGS_$(PLATFORM))

OBJS = $(OBJ_DIR)\packet.obj $(OBJ_DIR)\arp.obj $(OBJ_DIR)\eth.obj $(OBJ_DIR)\ip.obj $(OBJ_DIR)\tcp.obj $(OBJ_DIR)\tcpsockm.obj $(OBJ_DIR)\udp.obj $(OBJ_DIR)\utils.obj $(OBJ_DIR)\dns.obj $(OBJ_DIR)\timer.obj $(OBJ_DIR)\ipasm.obj
OBJS += $(OBJ_DIR)\main.obj

# Additional search dirs for source files.
.cpp: TCPLIB
.asm: TCPLIB

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

.asm.obj:
    wasm -0 -mc $< -fo=$@ -fr=$*.err

.cpp.obj: .AUTODEPEND
    set wpp=$(CFLAGS)
    $(CPP) $< -fo=$@ -fr=$*.err

clean: .SYMBOLIC
    @echo Cleaning $(PLATFORM) $(BUILD)..
!ifdef __NT__
    @if exist $(OBJ_DIR) rmdir /s /q $(OBJ_DIR)
!else
    @if exist $(OBJ_DIR) del $(OBJ_DIR)\*.*
    @if exist $(OBJ_DIR) rmdir $(OBJ_DIR)
!endif
    @if exist *.err del *.err
