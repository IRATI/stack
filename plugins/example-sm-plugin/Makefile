#
# Written by Vincenzo Maffione <v.maffione@nextworks.it>
#

# Set INSTALLDIR environment variable to the directory containing
# the IRATI installation

ifndef INSTALLDIR
$(error The INSTALLDIR environment variable is not set)
endif

PLUGIN_NAME = sm-example
SOURCES     =					\
	plugin.cc				\
	sm-example.cc

CXX        = g++
CXXFLAGS   = -Wall -Werror -fPIC
LDFLAGS    = -shared -L $(INSTALLDIR)/lib
LIBS       = -lrina

CXXFLAGS   += -I $(INSTALLDIR)/include/rinad/ipcp
CXXFLAGS   += -I $(INSTALLDIR)/include/rinad
CXXFLAGS   += -I $(INSTALLDIR)/include

OBJECTS    = $(SOURCES:.cc=.o)

all: $(PLUGIN_NAME).so

$(PLUGIN_NAME).so: $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

.cc.o:
	$(CXX) -c $(CXXFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(PLUGIN_NAME).so

install:
	install $(PLUGIN_NAME).so $(INSTALLDIR)/lib/rinad/ipcp
	install -m 0644 $(PLUGIN_NAME).manifest $(INSTALLDIR)/lib/rinad/ipcp

uninstall:
	-rm $(INSTALLDIR)/lib/rinad/ipcp/$(PLUGIN_NAME).so $(INSTALLDIR)/lib/rinad/ipcp/$(PLUGIN_NAME).manifest
