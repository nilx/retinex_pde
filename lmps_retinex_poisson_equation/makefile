# Copyright 2009, 2010 IPOL Image Processing On Line http://www.ipol.im/
# Author: Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
#
# Copying and distribution of this file, with or without
# modification, are permitted in any medium without royalty provided
# the copyright notice and this notice are preserved.  This file is
# offered as-is, without any warranty.

SRCDIR	= src
DEMODIR	= demo
WIKIDIR	= wiki
SRCDOCDIR	= srcdoc
DOXYCFG	= "\n\
	PROJECT_NAME           = \"retinex pde\"\n\
	EXTRACT_STATIC         = YES\n\
	INPUT                  = $(SRCDIR)\n\
	SOURCE_BROWSER         = YES\n\
	EXAMPLE_PATH           = $(SRCDIR)\n\
	HTML_OUTPUT            = $(SRCDOCDIR)\n\
	GENERATE_LATEX         = NO\n\
	HAVE_DOT               = YES\n\
	CALL_GRAPH             = YES\n\
	CALLER_GRAPH           = YES"


default: src srcdoc

.PHONY	: src srcdoc
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

.PHONY: update updatedemo updatewiki
update	: updatedemo updatewiki
updatedemo	:
	cp script/retinex_pde.sh $(DEMODIR)
	$(MAKE) -C $(SRCDIR) CC="ccache cc"
	cp $(SRCDIR)/retinex_pde $(DEMODIR)
	echo "update the demo" #$(MAKE) -C $(DEMODIR)
updatewiki	:
	$(MAKE) src srcdoc
	cp -f src.tgz src.zip srcdoc.tgz srcdoc.zip $(WIKIDIR)
	$(RM) -r $(WIKIDIR)/srcdoc
	tar xvzf $(WIKIDIR)/srcdoc.tgz -C $(WIKIDIR)
	cd $(WIKIDIR) && git add * && git commit -a && git push

.PHONY: distclean
distclean	:
	$(MAKE) -C $(SRCDIR) distclean
	$(RM) src.tgz src.zip
	$(RM) srcdoc.tgz srcdoc.zip

