# $Id$
#
# Makefile:  Makefile.test_genomic_collections_svc.app
#
# This file was originally generated by shell script "new_project" (r179222)
# Mon Dec 28 14:49:23 EST 2009
#

###  BASIC PROJECT SETTINGS
APP = test_genomic_collections_svc
SRC = test_genomic_collections_svc
# OBJ =

LIB_ = gencoll_client genome_collection $(SEQ_LIBS) pub medline biblio \
	   general xser xconnect xutil xncbi


LIB = $(LIB_:%=%$(STATIC))

###  EXAMPLES OF OTHER SETTINGS THAT MIGHT BE OF INTEREST
# CPPFLAGS = $(ORIG_CPPFLAGS) $(NCBI_C_INCLUDE)
# CFLAGS   = $(FAST_CFLAGS)
# CXXFLAGS = $(FAST_CXXFLAGS)
#
# LIB_OR_DLL = dll

CPPFLAGS = $(ORIG_CPPFLAGS)
LIBS = $(NETWORK_LIBS) $(ORIG_LIBS)

CHECK_CMD = test_genomic_collections_svc -request get-assembly -acc GCF_000001405.27
CHECK_CMD = test_genomic_collections_svc -request get-best-assembly -acc NC_002008.4
CHECK_CMD = test_genomic_collections_svc -request get-chrtype-valid -type eChromosome -loc eMacronuclear

WATCHERS = akimchi zherikov