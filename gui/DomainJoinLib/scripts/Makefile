#
# Arguments
#

PREFIX_DIR=/usr/centeris
LIB_DIR_NAME=lib

ASSEMBLY_DIR=$(PREFIX_DIR)/bin
LIB_DIR=$(PREFIX_DIR)/$(LIB_DIR_NAME)
MONO_DIR=$(PREFIX_DIR)/mono
ID_PROG=id

TEMPLATE=mono-invoke.template

CLI_NAME=domainjoin-cli
GUI_NAME=domainjoin-gui
CLI_NAME_SETKEY=setkey-cli
GUI_NAME_SETKEY=setkey-gui

$(CLI_NAME): ASSEMBLY_NAME=Centeris.DomainJoin.Console.exe
$(GUI_NAME): ASSEMBLY_NAME=DomainJoin.exe
$(CLI_NAME_SETKEY): ASSEMBLY_NAME=SetKeyCli.exe
$(GUI_NAME_SETKEY): ASSEMBLY_NAME=SetKey.exe

SCRIPTS=$(CLI_NAME) $(GUI_NAME) $(CLI_NAME_SETKEY) $(GUI_NAME_SETKEY)

help:
	@echo "Define PREFIX_DIR and ID_PROG to do OS-specific overrides."
	@echo "For Solaris, you will need PREFIX_DIR=/opt/centeris ID_PROG=/usr/xpg4/bin/id"
	@exit 13

all: $(SCRIPTS)

$(SCRIPTS): $(TEMPLATE) Makefile
	sed -e \
	    's/__SCRIPT_NAME__/$@/' \
	    -e 's,__ASSEMBLY_DIR__,$(ASSEMBLY_DIR),' \
	    -e 's,__ASSEMBLY_NAME__,$(ASSEMBLY_NAME),' \
	    -e 's,__ID_PROG__,$(ID_PROG),' \
	    -e 's,__MONO_DIR__,$(MONO_DIR),' \
	    -e 's,__LIB_DIR__,$(LIB_DIR),' \
	    < $(TEMPLATE) > $@

clean:
	rm -f $(SCRIPTS)
