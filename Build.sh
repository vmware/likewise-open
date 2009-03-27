#!/bin/bash

COMPONENTS="\
	krb5 \
	openldap \
	libunistr \
	lwbase \
	lwmapsecurity \
        lwmsg \
	centutils \
	sqlite \
	libtdb \
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

	
