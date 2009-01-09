#!/bin/bash

COMPONENTS="\
	krb5 \
	openldap \
        lwmsg \
	centutils \
	sqlite \
	libunistr \
	pstore \
	netlogon \
        lsmb \
	libkeytab \
	libgss \
	dcerpc \
	librpc \
	eventlog \
	lsass \
	lwdns \
	domainjoin \
	"

for comp in ${COMPONENTS}; do
	build/mkcomponent $comp || exit 1
done

	
