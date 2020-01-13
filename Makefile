STD ?= c++14
ARCH := haswell

ifeq ($(DEBUG), 1)
OPT  := -Og
else
CPPFLAGS += -DNDEBUG
OPT := -O2
endif

CPPFLAGS += -MMD -Wall -Wextra -Werror $(OPT) -g -march=$(ARCH) -Wno-unused-parameter $(CPPEXTRA) \
    -Wno-error=unused-variable \
	-Wno-unknown-pragmas \
#	-funroll-loops

CXXFLAGS += -std=$(STD) -DENABLE_TIMER=1

# uncomment to use the fast gold linker
# LDFLAGS = -use-ld=gold


TARGETS := bench test voltmon
MAINOS  := main.o main-test.o voltmon.o

TESTSRCS:= $(wildcard *-test.c *-test.cpp)
TESTOBJS:= $(patsubst %.c,%.o,$(TESTSRCS))
TESTOBJS:= $(patsubst %.cpp,%.o,$(TESTOBJS))

SRCS    := $(wildcard *.c *.cpp xxHash/xxhash.c)
OBJECTS := $(patsubst %.c,%.o,$(SRCS))
OBJECTS := $(patsubst %.cpp,%.o,$(OBJECTS))
ALLOBJS := $(OBJECTS)
OBJECTS := $(filter-out $(MAINOS) $(TESTOBJS), $(OBJECTS))

DEPS    := $(ALLOBJS:%.o=%.d)

# $(info $$DEPS is [${DEPS}])
# $(info $$TESTOBJS is [${TESTOBJS}])
# $(info $$OBJECTS is [${OBJECTS}])

JE_LIB := jevents/libjevents.a
JE_SRC := $(wildcard jevents/*.c jevents/*.h)

LDFLAGS := -lm

ifneq ($(LD), ld)
LDFLAGS += $(if $(LD),-fuse-ld=$(LD))
endif

ifeq ($(ASAN), 1)
CPPFLAGS += -fsanitize=address -fno-omit-frame-pointer
LDFLAGS  += -lasan
endif

default: $(TARGETS)

-include $(DEPS)

bench : $(OBJECTS) main.o

voltmon : voltmon.o msr-access.o

test  : $(OBJECTS) $(TESTOBJS)

$(TARGETS) : $(JE_LIB)
	$(CXX) $(CFLAGS) $(CPPFLAGS) $^ $(LDFLAGS) -o $@ $(JE_LIB)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CXXEXTRA) -c -o $@ $<

$(JE_LIB): $(JE_SRC)
	cd jevents && $(MAKE) MAKEFLAGS=

clean:
	rm -f $(TARGETS)
	rm -f *.o *.d
