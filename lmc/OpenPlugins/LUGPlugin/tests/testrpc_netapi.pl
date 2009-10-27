#!/usr/bin/perl

if(@ARGV ne 1) {

    print "Usage: testrpc_netapi.pl <hostname>\n";
    exit;
}

$hostname = $ARGV[0];

print "hostname = " . $hostname . "\n";

$basecmd = "/usr/centeris/bin/testrpc -h " . $hostname;

@tests = (
    "NETAPI-USER-ADD", 
    "NETAPI-USER-DEL", 
    "NETAPI-USER-GETINFO", 
    "NETAPI-USER-SETINFO",
    "NETAPI-JOIN-DOMAIN",
    "NETAPI-USER-CHANGE-PASSWORD"
);

$cmd = "ping -c 3 " . $hostname;
system($cmd);

foreach $test (@tests) {
    
    print "Running test " . $test;
    print "\n===================================\n";
	
    $cmd = $basecmd . " " . $test;
    #print $cmd . "\n";
    system($cmd);

    print "\n###################################";
    print "\n###################################\n\n";
}

