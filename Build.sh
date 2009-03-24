#!/bin/bash

COMPONENTS="\
	krb5 \
	openldap \
	libunistr \
	lwbase \
        lwmsg \
	centutils \
	sqlite \
	pstore \
	netlogon \
        lwio \
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

	
