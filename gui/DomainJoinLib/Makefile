# 01-tooling/04-integrate
MCS = gmcs

ifdef DEBUG
MCSFLAGS = -debug
endif

.PHONY: clean all

SRC=AuthDaemonManager.cs \
  ComputerName.cs \
  Config.cs \
  DaemonManager.cs \
  DomainJoinLib.cs \
  Exceptions.cs \
  ExecUtils.cs \
  FileUtils.cs \
  IniPreserve.cs \
  IniSectionAlreadyExists.cs \
  IniSection.cs \
  IniSectionNotFound.cs \
  ParseHosts.cs \
  SupportClass.cs \
  SysConfigNetworkConfigFixer.cs \
  Test.cs \
  TimeManager.cs


all: DomainJoinLib.dll

#%.exe: %.cs
#	$(MCS) $(MCSFLAGS) -target:exe -out:$@ $(filter %.cs,$^) \
#	$(foreach dl,$(filter %.dll,$^),$(addprefix -r:,$(dl)))

%.dll: %.cs
	$(MCS) $(MCSFLAGS) -target:library -out:$@ $(filter %.cs,$^) \
	$(foreach dl,$(filter %.dll,$^),$(addprefix -r:,$(dl)))

DomainJoinLib.dll: $(SRC)
	$(MCS) $(MCSFLAGS) -target:library -out:$@ $(SRC)

clean:
	rm -f *.exe *.dll
install:
	install -m755 DomainJoinLib.dll $(DESTDIR)/$(PREFIX)/bin/DomainJoinLib.dll
