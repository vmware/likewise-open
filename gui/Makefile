DIRS=DomainJoinLib DomainJoin

all:
	for d in $(DIRS); do (cd $$d && make); done

clean:
	for d in $(DIRS); do (cd $$d && make clean); done

install:
	for d in $(DIRS); do \
		(cd $$d && \
		 make DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) install); \
	done
