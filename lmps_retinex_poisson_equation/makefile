# Copyright 2009, 2010 IPOL Image Processing On Line http://www.ipol.im/
# Author: Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
#
# Copying and distribution of this file, with or without
# modification, are permitted in any medium without royalty provided
# the copyright notice and this notice are preserved.  This file is
# offered as-is, without any warranty.

PKGNAME	= retinex_pde
SRCDIR	= ./src
SRCDOCDIR	= ./srcdoc
DOXYCFG	= "\n\
	PROJECT_NAME           = $(PKGNAME)\n\
	EXTRACT_STATIC         = YES\n\
	INPUT                  = $(SRCDIR)\n\
	FILE_PATTERNS          = *.c *.h\n\
	RECURSIVE              = YES\n\
	SOURCE_BROWSER         = YES\n\
	HTML_OUTPUT            = $(SRCDOCDIR)\n\
	GENERATE_LATEX         = NO\n\
	HAVE_DOT               = YES\n\
	CALL_GRAPH             = YES\n\
	CALLER_GRAPH           = YES"


default: src srcdoc
.PHONY	: src srcdoc distclean

src	:
	$(MAKE) -C $(SRCDIR) distclean
	/bin/tar cvzf src.tgz $(SRCDIR)
	zip -qr9 src.zip $(SRCDIR)

srcdoc	:
	doxygen -g - > doxygen.conf
	echo $(DOXYCFG) >> doxygen.conf
	/usr/bin/doxygen doxygen.conf
	$(RM) doxygen.conf
	$(RM) $(SRCDOCDIR)/*.md5
	/bin/tar cvzf srcdoc.tgz $(SRCDOCDIR)
	zip -qr9 srcdoc.zip $(SRCDOCDIR)
	$(RM) -r $(SRCDOCDIR)

distclean	:
	$(RM) src.tgz src.zip
	$(RM) srcdoc.tgz srcdoc.zip

