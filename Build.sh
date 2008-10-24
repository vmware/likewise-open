#!/bin/bash

COMPONENTS="\
	krb5 \
	openldap \
	centutils \
	sqlite \
	libunistr \
	pstore \
	netlogon \
	libsmbclient \
	libkeytab \
	npcmuxer \
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

	
