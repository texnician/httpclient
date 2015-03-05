CXX := /usr/local/gcc49/bin/g++49
CXXFLAGS += -std=c++11
GCC_LIBRARY_PATH=/usr/local/gcc49/lib64
LDFLAGS += -pthread

BINDIR := ./bin
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))
SINGLE_BINS := $(patsubst %.cpp,%,$(SRCS))
DEBUG_SYMBOLS := $(patsubst %.cpp,%.dSYM,$(SRCS))

FB_ROOT := $(HOME)/buildutil

ALL_LIB_DIRS := $(FB_ROOT)/lib
ALL_INCLUDE_DIRS := $(FB_ROOT)/include

CXXFLAGS += $(addprefix -I,$(ALL_INCLUDE_DIRS))
LDFLAGS += $(addprefix -L,$(ALL_LIB_DIRS))

DEP_DIR := .deps
$(shell test -e $(DEP_DIR) || mkdir $(DEP_DIR))
DEPS := $(addprefix $(DEP_DIR)/,$(subst .cpp,.Tpo,$(SRCS)))

FB_LIBS := proxygenlib thriftcpp2 thrift thriftz folly event ssl crypto double-conversion

DYNAMIC_LIBS := gflags glog rt dl z

LINK_STATIC_FLAGS := $(addprefix -l,$(FB_LIBS))
LINK_DYNAMIC_FLAGS := $(addprefix -l,$(DYNAMIC_LIBS))

$(shell test -d $(BINDIR) || mkdir $(BINDIR))

.PHONY: all install clean distclean

.SUFFIXES:
.SUFFIXES: .o .h
.PRECIOUS: %.c %.cpp %.o

TARGET := httpclient


all: $(TARGET)

-include $(DEPS)

install:
	cp -f $(SINGLE_BINS) $(BINDIR)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ -Wl,-Bstatic $(LINK_STATIC_FLAGS) -Wl,-Bdynamic $(LINK_DYNAMIC_FLAGS) -Wl,-rpath=$(GCC_LIBRARY_PATH)
#@which dsymutil 2&> 1 > /dev/null ; if [ $$? -eq 0 ] ; then dsymutil $@ ; fi
	@grep -xq "$@" .gitignore || echo $@ >> .gitignore

%: %.o
	$(CXX) $(CXXFLAGS) -MD -MP -MF .$(DEP_DIR)/$*.Tpo -o $@ $<
#@which dsymutil 2&>1 > /dev/null ; if [ $$? -eq 0 ] ; then dsymutil $@ ; fi
	@grep -xq "$@" .gitignore || echo $@ >> .gitignore


%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MD -MP -MF $(DEP_DIR)/$*.Tpo -D__MAIN__ -g -o $@ -c $<

clean:
	rm -f $(TARGET)
	rm -f $(OBJS)

distclean: clean
	rm -f $(BINDIR)/*
	rm -f $(DEPS)
