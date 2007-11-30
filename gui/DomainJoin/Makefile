# 01-tooling/04-integrate
MCS = gmcs
MCSFLAGS = -debug

.PHONY: clean all

SRC=ComputerNameChangesDialog.cs CredsDialog.cs DetailForm.cs \
  DomainJoinManager.cs DomainManagerAPI.cs JoinProgressForm.cs \
  Prompt.cs UserInfo.cs WelcomeForm.cs

RESOURCES = CredsDialog.resources DomainJoinManagerForm.resources Prompt.resources \
  ComputerNameChangesDialog.resources DetailForm.resources JoinProgressForm.resources \
  WelcomeForm.resources

LIBS = ../DomainJoinLib/DomainJoinLib.dll System.Windows.Forms.dll System.Drawing.dll


all: DomainJoin.exe

#%.exe: %.cs
#	$(MCS) $(MCSFLAGS) -target:exe -out:$@ $(filter %.cs,$^) \
#	$(foreach dl,$(filter %.dll,$^),$(addprefix -r:,$(dl)))

%.dll: %.cs
	$(MCS) $(MCSFLAGS) -target:library -out:$@ $(filter %.cs,$^) \
	$(foreach dl,$(filter %.dll,$^),$(addprefix -r:,$(dl)))

%.resources: %.resx
	echo "Compiling resource file $@..."
	@resgen2 /compile `echo $@ | sed 's/resources/resx/'`
	@/bin/mv $@ Centeris.DomainJoin.$@

DomainJoin.exe: $(SRC) $(RESOURCES) 
	$(MCS) $(MCSFLAGS) -target:exe -out:$@ \
  $(foreach res,$(RESOURCES), $(addprefix -resource:Centeris.DomainJoin.,$(res))) \
  $(foreach dl,$(LIBS),$(addprefix -r:,$(dl))) $(SRC)

clean:
	rm -f *.exe *resources

install:
	install -m755 DomainJoin.exe $(DESTDIR)/$(PREFIX)/bin/DomainJoin.exe
