MKDIR_P = @MKDIR_P@

if HAVE_DOXYGEN
DOXYGEN_DEPS = \
	Doxyfile.in \
	main.doxy \
	arch.doxy \
	arch_rep.doxy \
	arch_sess.doxy \
	example.doxy \
	example_fserv.doxy \
	example_fserv_layout.doxy \
	example_fserv_source.doxy

doxygen-html: $(DOXYGEN_DEPS)
	$(MKDIR_P) doxygen-html
	( cat Doxyfile; echo GENERATE_HTML=yes ) | $(DOXYGEN) -

doxygen-latex: $(DOXYGEN_DEPS)
	$(MKDIR_P) doxygen-latex
	( cat Doxyfile; echo GENERATE_LATEX=yes ) | $(DOXYGEN) -

lwmsg.pdf: doxygen-latex
	$(MAKE) -C doxygen-latex pdf
	cp doxygen-latex/refman.pdf lwmsg.pdf

doxygen-pdf: lwmsg.pdf

doxygen-html-install: doxygen-html
	$(MKDIR_P) -m 755 $(DESTDIR)$(docdir)/html
	$(INSTALL_DATA) $</* $(DESTDIR)$(docdir)/html

doxygen-pdf-install: lwmsg.pdf
	$(MKDIR_P) -m 755 $(DESTDIR)$(docdir)/pdf
	$(INSTALL_DATA) $< $(DESTDIR)$(docdir)/pdf

all-local: doxygen-html doxygen-pdf

install-data-local: doxygen-html-install doxygen-pdf-install

.PHONY: doxygen-html doxygen-pdf doxygen-html-install

endif
