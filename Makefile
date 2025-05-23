ifneq ($(findstring singularity,$(HOST)),)
CC_COMPILER = g++
CC_OPTIONS = -Wall -Wold-style-cast -c -O3 -fPIC -std=c++17 -Weffc++
SOURCEDIR = /src
INCLUDEDIR = /usr/include/myincludes
POSTGRESQLINCLUDEDIR = /usr/include/postgresql
LIBDIR = /usr/lib64/mylibs
LIBVERSION =
#
F77_COMPILER = ifort
F77_OPTIONS = -c -fPIC
#
ACFTSUBS = $(SOURCEDIR)/libacft/datsavaircraft.cpp $(SOURCEDIR)/libacft/tdf57aircraft.cpp
ACFTOBJS = datsavaircraft.o tdf57aircraft.o
#
ifneq ($(wildcard $(SOURCEDIR)/libbitmap/*.cpp),)
BITMAPOBJS = $(shell ls -1 $(SOURCEDIR)/libbitmap/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libbitmap/libbitmap\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libbufr/*.cpp),)
BUFROBJS = $(shell ls -1 $(SOURCEDIR)/libbufr/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libbufr/libbufr\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libcitation/*.cpp),)
CITATIONOBJS = $(shell ls -1 $(SOURCEDIR)/libcitation/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libcitation/libcitation\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libcitation_pg/*.cpp),)
CITATION_PGOBJS = $(shell ls -1 $(SOURCEDIR)/libcitation_pg/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libcitation_pg/libcitation_pg\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libcyclone/*.cpp),)
CYCLONEOBJS = $(shell ls -1 $(SOURCEDIR)/libcyclone/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libcyclone/libcyclone\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libdatetime/*.cpp),)
DATETIMEOBJS = $(shell ls -1 $(SOURCEDIR)/libdatetime/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libdatetime/libdatetime\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/liberror/*.cpp),)
ERROROBJS = $(shell ls -1 $(SOURCEDIR)/liberror/*.cpp |sed "s/\.cpp/\.o/" |sed "s/liberror/liberror\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libgrids/*.cpp),)
GRIDOBJS = $(shell ls -1 $(SOURCEDIR)/libgrids/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libgrids/libgrids\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libgridutils/*.cpp),)
GRIDUTILOBJS = $(shell ls -1 $(SOURCEDIR)/libgridutils/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libgridutils/libgridutils\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libhdf/*.cpp),)
HDFOBJS = $(shell ls -1 $(SOURCEDIR)/libhdf/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libhdf/libhdf\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libio/*.cpp),)
IOOBJS = $(shell ls -1 $(SOURCEDIR)/libio/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libio/libio\/singularity/")
endif
#
IOSUBS_F77 = $(SOURCEDIR)/crayopen.f $(SOURCEDIR)/rptopen.f
IOOBJS_F77 = crayopen.o rptopen.o
#
IOMETASUBS = $(SOURCEDIR)/libiometadata/iometautils.cpp
IOMETAOBJS = iometautils.o
#
#JSONOBJS = $(shell ls -1 $(SOURCEDIR)/libjson/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libjson/libjson\/singularity/")
#
#LDAPOBJS = $(shell ls -1 $(SOURCEDIR)/libmyldap/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libmyldap/libmyldap\/singularity/")
#
ifneq ($(wildcard $(SOURCEDIR)/libmetaexport/*.cpp),)
METAEXPORTOBJS = $(shell ls -1 $(SOURCEDIR)/libmetaexport/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libmetaexport/libmetaexport\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libmetaexport_pg/*.cpp),)
METAEXPORT_PGOBJS = $(shell ls -1 $(SOURCEDIR)/libmetaexport_pg/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libmetaexport_pg/libmetaexport_pg\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libmetaexporthelpers/*.cpp),)
METAEXPORTHELPERSOBJS = $(shell ls -1 $(SOURCEDIR)/libmetaexporthelpers/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libmetaexporthelpers/libmetaexporthelpers\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libmetaexporthelpers_pg/*.cpp),)
METAEXPORTHELPERS_PGOBJS = $(shell ls -1 $(SOURCEDIR)/libmetaexporthelpers_pg/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libmetaexporthelpers_pg/libmetaexporthelpers_pg\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libmetahelpers/*.cpp),)
METAHELPERSOBJS = $(shell ls -1 $(SOURCEDIR)/libmetahelpers/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libmetahelpers/libmetahelpers\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libmetautils/*.cpp),)
METAUTILSOBJS = $(shell ls -1 $(SOURCEDIR)/libmetautils/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libmetautils/libmetautils\/singularity/")
endif
#
#MYCURLOBJS = $(shell ls -1 $(SOURCEDIR)/libmycurl/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libmycurl/libmycurl\/singularity/")
#
ifneq ($(wildcard $(SOURCEDIR)/libmyssl/*.cpp),)
MYSSLOBJS = $(shell ls -1 $(SOURCEDIR)/libmyssl/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libmyssl/libmyssl\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libobs/*.cpp),)
OBSOBJS = $(shell ls -1 $(SOURCEDIR)/libobs/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libobs/libobs\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libpostgresql/*.cpp),)
POSTGRESQLOBJS = $(shell ls -1 $(SOURCEDIR)/libpostgresql/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libpostgresql/libpostgresql\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libs3/*.cpp),)
S3OBJS = $(shell ls -1 $(SOURCEDIR)/libs3/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libs3/libs3\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libsearch/*.cpp),)
SEARCHOBJS = $(shell ls -1 $(SOURCEDIR)/libsearch/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libsearch/libsearch\/singularity/")
endif
#
#SOCKOBJS = $(shell ls -1 $(SOURCEDIR)/libsock/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libsock/libsock\/singularity/")
#
SPELLCHECKERSUBS = $(SOURCEDIR)/libspellchecker/spellchecker.cpp
SPELLCHECKEROBJS = spellchecker.o
#
ifneq ($(wildcard $(SOURCEDIR)/libutils/*.cpp),)
UTILOBJS = $(shell ls -1 $(SOURCEDIR)/libutils/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libutils/libutils\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libutilsthread/*.cpp),)
UTILSTHREADOBJS = $(shell ls -1 $(SOURCEDIR)/libutilsthread/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libutilsthread/libutilsthread\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libweb/*.cpp),)
WEBOBJS = $(shell ls -1 $(SOURCEDIR)/libweb/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libweb/libweb\/singularity/")
endif
#
ifneq ($(wildcard $(SOURCEDIR)/libxml/*.cpp),)
XMLOBJS = $(shell ls -1 $(SOURCEDIR)/libxml/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libxml/libxml\/singularity/")
endif
#
#YAMLOBJS = $(shell ls -1 $(SOURCEDIR)/libyaml/*.cpp |sed "s/\.cpp/\.o/" |sed "s/libyaml/libyaml\/singularity/")
#
all: libbitmap.so libbufr.so libcitation.so libcitation_pg.so libcyclone.so liberror.so libgrids.so libgridutils.so libhdf.so obj_iometadata libiometadata.so libio.so libjson.so libmetaexport.so libmetaexport_pg.so libmetaexporthelpers.so libmetaexporthelpers_pg.so libmetahelpers.so libmetautils.so libmycurl.so libobs.so libpostgresql.so libsearch.so libsock.so obj_spellchecker libspellchecker.so libutils.so libutilsthread.so libweb.so libxml.so libyaml.so
#
obj_acft: $(ACFTSUBS)
ifneq ($(GCCVERSION),$(EXPECTEDGCCVERSION))
	$(error obj_acft: can\'t compile with g++ version $(GCCVERSION))
else
	$(CC_COMPILER) $(CC_OPTIONS) $(ACFTSUBS) -I$(INCLUDEDIR)
endif
libacft.so: $(ACFTOBJS)
ifeq ($(strip $(LIBVERSION)),)
	$(error libacft.so: no version number given)
else
ifneq ($(GCCVERSION),$(EXPECTEDGCCVERSION))
	$(error libacft.so: can\'t compile with g++ version $(GCCVERSION))
else
	$(CC_COMPILER) -shared -o $(LIBDIR)/libacft.so.$(LIBVERSION) -Wl,-soname,libacft.so.$(LIBVERSION) $(ACFTOBJS)
	rm -f libacft.so
	ln -s libacft.so.$(LIBVERSION) libacft.so
	sudo -u rdadata rm -f $(DSSLIBDIR)/libacft.so
	sudo -u rdadata cp $(LIBDIR)/libacft.so.$(LIBVERSION) $(DSSLIBDIR)
	sudo -u rdadata ln -s $(DSSLIBDIR)/libacft.so.$(LIBVERSION) $(DSSLIBDIR)/libacft.so
endif
endif
#
$(SOURCEDIR)/libbitmap/singularity/%.o: $(SOURCEDIR)/libbitmap/%.cpp $(INCLUDEDIR)/bitmap.hpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libbitmap.so: $(BITMAPOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libbitmap.so -Wl,-soname,libbitmap.so $(BITMAPOBJS)
#
$(SOURCEDIR)/libbufr/singularity/%.o: $(SOURCEDIR)/libbufr/%.cpp $(INCLUDEDIR)/bufr.hpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libbufr.so: $(BUFROBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libbufr.so -Wl,-soname,libbufr.so $(BUFROBJS)
#
$(SOURCEDIR)/libcitation_pg/singularity/%.o: $(SOURCEDIR)/libcitation_pg/%.cpp $(INCLUDEDIR)/citation_pg.hpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -I$(POSTGRESQLINCLUDEDIR) -o $@
libcitation_pg.so: $(CITATION_PGOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libcitation_pg.so -Wl,-soname,libcitation_pg.so $(CITATION_PGOBJS)
#
$(SOURCEDIR)/libcyclone/singularity/%.o: $(SOURCEDIR)/libcyclone/%.cpp $(INCLUDEDIR)/cyclone.hpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libcyclone.so: $(CYCLONEOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libcyclone.so -Wl,-soname,libcyclone.so $(CYCLONEOBJS)
#
$(SOURCEDIR)/libdatetime/singularity/%.o: $(SOURCEDIR)/libdatetime/%.cpp $(INCLUDEDIR)/datetime.hpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libdatetime.so: $(DATETIMEOBJS) $(INCLUDEDIR)/datetime.hpp
	$(CC_COMPILER) -shared -o $(LIBDIR)/libdatetime.so -Wl,-soname,libdatetime.so $(DATETIMEOBJS)
#
$(SOURCEDIR)/liberror/singularity/%.o: $(SOURCEDIR)/liberror/%.cpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
liberror.so: $(ERROROBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/liberror.so -Wl,-soname,liberror.so $(ERROROBJS)
#
$(SOURCEDIR)/libgrids/singularity/%.o: $(SOURCEDIR)/libgrids/%.cpp $(INCLUDEDIR)/grid.hpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -D__WITH_JASPER -o $@
libgrids.so: $(GRIDOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libgrids.so -Wl,-soname,libgrids.so $(GRIDOBJS)
#
$(SOURCEDIR)/libgridutils/singularity/%.o: $(SOURCEDIR)/libgridutils/%.cpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libgridutils.so: $(GRIDUTILOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libgridutils.so -Wl,-soname,libgridutils.so $(GRIDUTILOBJS)
#
$(SOURCEDIR)/libhdf/singularity/%.o: $(SOURCEDIR)/libhdf/%.cpp $(INCLUDEDIR)/hdf.hpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libhdf.so: $(HDFOBJS) $(INCLUDEDIR)/hdf.hpp
	$(CC_COMPILER) -shared -o $(LIBDIR)/libhdf.so -Wl,-soname,libhdf.so $(HDFOBJS)
#
$(SOURCEDIR)/libio/singularity/%.o: $(SOURCEDIR)/libio/%.cpp $(INCLUDEDIR)/iodstream.hpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libio.so: $(IOOBJS) $(INCLUDEDIR)/utils.hpp
	$(CC_COMPILER) -shared -o $(LIBDIR)/libio.so -Wl,-soname,libio.so $(IOOBJS)
#
libiof.so: $(IOSUBS_F77) $(SOURCEDIR)/craywrap.cpp $(SOURCEDIR)/rptinwrap.cpp
	$(CC_COMPILER) $(CC_OPTIONS) $(SOURCEDIR)/myerror_f77.cpp $(SOURCEDIR)/craywrap.cpp $(SOURCEDIR)/rptinwrap.cpp -I$(INCLUDEDIR)
	$(F77_COMPILER) $(F77_OPTIONS) $(IOSUBS_F77)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libiof.so -Wl,-soname,libiof.so myerror_f77.o craywrap.o rptinwrap.o $(IOOBJS_F77)
	cp libiof.so $(DSSLIBDIR)
#
$(SOURCEDIR)/libjson/singularity/%.o: $(SOURCEDIR)/libjson/%.cpp $(INCLUDEDIR)/json.hpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libjson.so: $(JSONOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libjson.so -Wl,-soname,libjson.so $(JSONOBJS)
#
$(SOURCEDIR)/libmyldap/singularity/%.o: $(SOURCEDIR)/libmyldap/%.cpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libmyldap.so: $(LDAPOBJS) $(INCLUDEDIR)/MyLDAP.hpp
	$(CC_COMPILER) -shared -o $(LIBDIR)/libmyldap.so -Wl,-soname,libmyldap.so $(LDAPOBJS)
#
$(SOURCEDIR)/libmetaexport_pg/singularity/%.o: $(SOURCEDIR)/libmetaexport_pg/%.cpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -I$(POSTGRESQLINCLUDEDIR) -o $@
libmetaexport_pg.so: $(METAEXPORT_PGOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libmetaexport_pg.so -Wl,-soname,libmetaexport_pg.so $(METAEXPORT_PGOBJS)
#
$(SOURCEDIR)/libmetaexporthelpers_pg/singularity/%.o: $(SOURCEDIR)/libmetaexporthelpers_pg/%.cpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -I$(POSTGRESQLINCLUDEDIR) -o $@
libmetaexporthelpers_pg.so: $(METAEXPORTHELPERS_PGOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libmetaexporthelpers_pg.so -Wl,-soname,libmetaexporthelpers_pg.so $(METAEXPORTHELPERS_PGOBJS)
#
$(SOURCEDIR)/libmetahelpers/singularity/%.o: $(SOURCEDIR)/libmetahelpers/%.cpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -I$(POSTGRESQLINCLUDEDIR) -o $@
libmetahelpers.so: $(METAHELPERSOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libmetahelpers.so -Wl,-soname,libmetahelpers.so $(METAHELPERSOBJS)
#
$(SOURCEDIR)/libmetautils/singularity/%.o: $(SOURCEDIR)/libmetautils/%.cpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -I$(POSTGRESQLINCLUDEDIR) -o $@
libmetautils.so: $(METAUTILSOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libmetautils.so -Wl,-soname,libmetautils.so $(METAUTILSOBJS)
#
$(SOURCEDIR)/libmycurl/singularity/%.o: $(SOURCEDIR)/libmycurl/%.cpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libmycurl.so: $(MYCURLOBJS) $(INCLUDEDIR)/mycurl.hpp
	$(CC_COMPILER) -shared -o $(LIBDIR)/libmycurl.so -Wl,-soname,libmycurl.so $(MYCURLOBJS)
#
$(SOURCEDIR)/libmyssl/singularity/%.o: $(SOURCEDIR)/libmyssl/%.cpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libmyssl.so: $(MYSSLOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libmyssl.so -Wl,-soname,libmyssl.so $(MYSSLOBJS)
#
$(SOURCEDIR)/libobs/singularity/%.o: $(SOURCEDIR)/libobs/%.cpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libobs.so: $(OBSOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libobs.so -Wl,-soname,libobs.so $(OBSOBJS)
#
$(SOURCEDIR)/libs3/singularity/%.o: $(SOURCEDIR)/libs3/%.cpp $(INCLUDEDIR)/s3.hpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libs3.so: $(S3OBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libs3.so -Wl,-soname,libs3.so $(S3OBJS)
#
$(SOURCEDIR)/libpostgresql/singularity/%.o: $(SOURCEDIR)/libpostgresql/%.cpp $(INCLUDEDIR)/PostgreSQL.hpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -I$(POSTGRESQLINCLUDEDIR) -o $@
libpostgresql.so: $(POSTGRESQLOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libpostgresql.so -Wl,-soname,libpostgresql.so $(POSTGRESQLOBJS)
#
$(SOURCEDIR)/libsearch/singularity/%.o: $(SOURCEDIR)/libsearch/%.cpp $(INCLUDEDIR)/search.hpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -I$(POSTGRESQLINCLUDEDIR) -o $@
libsearch.so: $(SEARCHOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libsearch.so -Wl,-soname,libsearch.so $(SEARCHOBJS)
#
$(SOURCEDIR)/libsock/singularity/%.o: $(SOURCEDIR)/libsock/%.cpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libsock.so: $(SOCKOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libsock.so -Wl,-soname,libsock.so $(SOCKOBJS)
#
$(SOURCEDIR)/libutils/singularity/%.o: $(SOURCEDIR)/libutils/%.cpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libutils.so: $(UTILOBJS) $(INCLUDEDIR)/utils.hpp $(INCLUDEDIR)/strutils.hpp
	$(CC_COMPILER) -shared -o $(LIBDIR)/libutils.so -Wl,-soname,libutils.so $(UTILOBJS)
#
$(SOURCEDIR)/libutilsthread/singularity/%.o: $(SOURCEDIR)/libutilsthread/%.cpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libutilsthread.so: $(UTILSTHREADOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libutilsthread.so -Wl,-soname,libutilsthread.so $(UTILSTHREADOBJS)
#
$(SOURCEDIR)/libweb/singularity/%.o: $(SOURCEDIR)/libweb/%.cpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -I$(POSTGRESQLINCLUDEDIR) -o $@
libweb.so: $(WEBOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libweb.so -Wl,-soname,libweb.so $(WEBOBJS)
#
$(SOURCEDIR)/libxml/singularity/%.o: $(SOURCEDIR)/libxml/%.cpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libxml.so: $(XMLOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libxml.so -Wl,-soname,libxml.so $(XMLOBJS)
#
$(SOURCEDIR)/libyaml/singularity/%.o: $(SOURCEDIR)/libyaml/%.cpp
	$(CC_COMPILER) $(CC_OPTIONS) $< -I$(INCLUDEDIR) -o $@
libyaml.so: $(YAMLOBJS)
	$(CC_COMPILER) -shared -o $(LIBDIR)/libyaml.so -Wl,-soname,libyaml.so $(YAMLOBJS)
else
%::
	$(error singularity libraries must be built with HOST=singularity)
endif
