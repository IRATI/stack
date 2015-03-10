#
# Written by Francesco Salvestrini <f.salvestrini@nextworks.it>
#

POLICY_NAME = skeleton
SOURCES     =					\
	source1.cc				\
	source2.cc

CXX        = g++
CXXFLAGS   = -Wall -Werror -fPIC
LDFLAGS    = -shared
LIBS       =
PKG_CONFIG_PATH =
PKG_CONFIG      = pkg-config

OBJECTS    = $(SOURCES:.cc=.o)

all: $(POLICY_NAME).so

$(POLICY_NAME).so: $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

.cc.o:
	$(CXX) -c $(CXXFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS)

install:
	@echo "This target has not been implemented yet"
	@exit 1

uninstall:
	@echo "This target has not been implemented yet"
	@exit 1
