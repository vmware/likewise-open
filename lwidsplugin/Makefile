OUT_DIR=build/Default/LWIDSPlugIn.dsplug
OUT_PROGRAM_DIR=$(OUT_DIR)/Contents/MacOS
OUT_INFO_DIR=$(OUT_DIR)/Contents

PROGRAM=LWIDSPlugIn

SOURCES=\
	LWAuthAdapter.cpp \
	LWIAttrLookup.cpp \
	LWIAttrValDataQuery.cpp \
	LWIBitVector.cpp \
	LWICRC.cpp \
	LWIDirNodeQuery.cpp \
	LWIGroup.cpp \
	LWIQuery.cpp \
	LWIRecTypeLookup.cpp \
	LWIRecordListQuery.cpp \
	LWIRecordQuery.cpp \
	LWISAuthAdapter.cpp \
	LWIUser.cpp \
	PlugInShell.cpp \
	ServerModule.c \
	ServerModuleLib.c \
	Utilities.cpp

FRAMEWORKS=CoreFoundation

FRAMEWORKS:=$(addprefix -framework ,$(FRAMEWORKS))

OBJS:=$(SOURCES)
OBJS:=$(OBJS:%.c=%.o)
OBJS:=$(OBJS:%.cpp=%.o)

OUT_PROGRAM=$(OUT_PROGRAM_DIR)/$(PROGRAM)
OUT_INFO=$(OUT_INFO_DIR)/Info.plist

all: $(OUT_PROGRAM) $(OUT_INFO)

clean:
	rm -f $(PROGRAM)

.SUFFIX: .c .cpp .o

LOCAL_CFLAGS=-arch i386 -fno-common -fpascal-strings -Os -Wmost -Wno-four-char-constants -Wno-unknown-pragmas
WARN_CFLAGS=-Werror -Wall

COMPILE_FLAGS=-I. $(LOCAL_CFLAGS) $(WARN_CFLAGS) $(_CPPFLAGS) $(_CFLAGS)

$(OUT_PROGRAM): $(OBJS)
	mkdir -p $(OUT_PROGRAM_DIR)
	g++-4.0 -bundle -o $@ $(FRAMEWORKS) $(_LDFLAGS) $(OBJS)

$(OUT_INFO): Info.plist
	mkdir -p $(OUT_INFO_DIR)
	cp Info.plist $@

%.o : %.c
	gcc $(COMPILE_FLAGS) -c -o $@ $<

%.o : %.cpp
	g++-4.0 $(COMPILE_FLAGS) -c -o $@ $<
