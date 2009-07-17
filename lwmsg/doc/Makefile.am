MKDIR_P = @MKDIR_P@

if HAVE_DOXYGEN

if HAVE_DOXYGEN_PDF
DOXYGEN_PDF_TARGET=doxygen-pdf
DOXYGEN_PDF_INSTALL_TARGET=doxygen-pdf-install
else
DOXYGEN_PDF_TARGET=
DOXYGEN_PDF_INSTALL_TARGET=
endif

DOXYGEN_HEADER_DEPS = \
	$(top_srcdir)/include/lwmsg/call.h \
	$(top_srcdir)/include/lwmsg/assoc.h \
	$(top_srcdir)/include/lwmsg/buffer.h \
	$(top_srcdir)/include/lwmsg/client.h \
	$(top_srcdir)/include/lwmsg/common.h \
	$(top_srcdir)/include/lwmsg/connection.h \
	$(top_srcdir)/include/lwmsg/context.h \
	$(top_srcdir)/include/lwmsg/data.h \
	$(top_srcdir)/include/lwmsg/lwmsg.h \
	$(top_srcdir)/include/lwmsg/protocol.h \
	$(top_srcdir)/include/lwmsg/security.h \
	$(top_srcdir)/include/lwmsg/server.h \
	$(top_srcdir)/include/lwmsg/session.h \
	$(top_srcdir)/include/lwmsg/status.h \
	$(top_srcdir)/include/lwmsg/time.h \
	$(top_srcdir)/include/lwmsg/type.h \
	$(top_srcdir)/include/lwmsg/archive.h \
	$(top_srcdir)/include/lwmsg/message.h

DOXYGEN_DEPS = \
	Doxyfile.in \
	main.doxy \
	arch.doxy \
	arch_rep.doxy \
	arch_sess.doxy \
	example.doxy \
	example_fserv.doxy \
	example_fserv_layout.doxy \
	example_fserv_source.doxy \
	$(DOXYGEN_HEADER_DEPS)

doxygen-html: $(DOXYGEN_DEPS)
	$(MKDIR_P) doxygen-html && \
	( cat Doxyfile; echo GENERATE_HTML=yes ) | $(DOXYGEN) - && \
	touch doxygen-html

doxygen-latex: $(DOXYGEN_DEPS)
	$(MKDIR_P) doxygen-latex && \
	( cat Doxyfile; echo GENERATE_LATEX=yes ) | $(DOXYGEN) - && \
	touch doxygen-latex

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

all-local: doxygen-html $(DOXYGEN_PDF_TARGET)

install-data-local: doxygen-html-install doxygen-pdf-install

.PHONY: doxygen-pdf doxygen-html-install doxygen-pdf-install

endif
