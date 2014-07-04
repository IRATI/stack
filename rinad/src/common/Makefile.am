#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

noinst_LTLIBRARIES = librinad.la

librinad_la_CFLAGS   =
librinad_la_CPPFLAGS = $(CPPFLAGS_EXTRA) $(LIBRINA_CFLAGS)
librinad_la_LIBADD   = $(LIBRINA_LIBS)

tclapSOURCES =					\
	tclap/ArgException.h			\
	tclap/Arg.h				\
	tclap/ArgTraits.h			\
	tclap/CmdLine.h				\
	tclap/CmdLineInterface.h		\
	tclap/CmdLineOutput.h			\
	tclap/Constraint.h			\
	tclap/DocBookOutput.h			\
	tclap/HelpVisitor.h			\
	tclap/IgnoreRestVisitor.h		\
	tclap/Makefile				\
	tclap/Makefile.am			\
	tclap/Makefile.in			\
	tclap/MultiArg.h			\
	tclap/MultiSwitchArg.h			\
	tclap/OptionalUnlabeledTracker.h	\
	tclap/StandardTraits.h			\
	tclap/StdOutput.h			\
	tclap/SwitchArg.h			\
	tclap/UnlabeledMultiArg.h		\
	tclap/UnlabeledValueArg.h		\
	tclap/ValueArg.h			\
	tclap/ValuesConstraint.h		\
	tclap/VersionVisitor.h			\
	tclap/Visitor.h				\
	tclap/XorHandler.h			\
	tclap/ZshCompletionOutput.h
	
encoderSOURCES =					\
	encoders/ApplicationProcessNamingInfoMessage.pb.cc \
	encoders/ApplicationRegistrationMessage.pb.cc  \
	encoders/CommonMessages.pb.cc

librinad_la_SOURCES  =					\
	$(encoderSOURCES)			\
	rina-configuration.cc rina-configuration.h	\
	event-loop.cc	      event-loop.h		\
	encoder.cc	      	  encoder.h		\
                          concurrency.h		\
	$(tclapSOURCES)
