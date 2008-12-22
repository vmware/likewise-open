SUBDIRS = \
	lsaclient \
	test_getgrgid \
	test_getpwuid \
	test_LsaGetNamesBySidList \
	test_getgrnam \
	test_LsaAllocSecurityIdentifierFromBinary \
	test_LsaGetSecurityIdentifierHashedRid \
	test_LsaAllocSecurityIdentifierFromString \
	test_LsaParseConfigFile \
	test_getpwent \
	test_getpwnam \
        test_stress_ad_provider \
        test_bitvector \
        test_perf \
	test_memcache \
	test_authenticate \
	test_validate \
	test_changepasswd \
	test_session \
	test_checkuserinlist

if !PLATFORM_IS_HPUX
    SUBDIRS += test_openldap
endif

