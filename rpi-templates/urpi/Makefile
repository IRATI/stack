#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

POLICY_NAME = skeleton
SOURCES     =					\
	source1.cc				\
	source2.cc

PKG_CONFIG_PATH =

CXX        = g++
CXXFLAGS   = -Wall -Werror -fPIC
LDFLAGS    = -shared
LIBS       =
PKG_CONFIG = pkg-config

OBJECTS    = $(SOURCES:.cc=.o)

all: $(POLICY_NAME).so

$(POLICY_NAME).so: $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

.cc.o:
	$(CXX) -c $(CXXFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS)
