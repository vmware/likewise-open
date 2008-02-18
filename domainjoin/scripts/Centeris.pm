##
## Copyright (C) Centeris Corporation 2004-2007
## Copyright (C) Likewise Software    2007-2008
## All rights reserved.
## 
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http:##www.gnu.org/licenses/>.
##

#
# Copyright Centeris Corporation.  All rights reserved.
#

package Centeris;

use strict;
use warnings;


BEGIN {
    use Exporter ();
    our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);

    #
    # Version for version checking
    #

    $VERSION = 1.00;

    @ISA = qw(Exporter);
    @EXPORT = qw(&GetOsType &GetDistroType &GetDistroVersion &ReadFile &WriteFile &ReplaceFile &ReplaceSymlink &GetFileParseMessage &QuickBaseName &CheckTestPrefix &CreateGlobalOpts &CompareLines);
    %EXPORT_TAGS = ( );

    #
    # Exported package globals and optionally exported functions
    #

    @EXPORT_OK = qw();
}
our @EXPORT_OK;


sub GetOsType(;$)
{
    my $testPrefix = shift || '';
    my $result = undef;

    if ( $testPrefix and ( -f "$testPrefix/ostype" ) )
    {
        my $lines = ReadFile("$testPrefix/ostype");
        $result = $lines->[0];
    }
    else
    {
        my @lines = `uname`;
        $result = $lines[0];
    }

    chomp($result);

    return $result;
}


sub GetDistroInfo(;$)
{
    my $testPrefix = shift || '';
    my $osType = GetOsType($testPrefix);

    my $distro = undef;
    my $version = undef;

    #
    # If using osdistro/osver, both must be present
    #

    my $distroFile = "$testPrefix/osdistro";
    my $verFile = "$testPrefix/osver";

    if ( $testPrefix and ( ( -f $distroFile ) or ( -f $verFile ) ) )
    {
        my $lines;

        $lines = ReadFile($distroFile);
        $distro = $lines->[0];
        chomp($distro);

        $lines = ReadFile($verFile);
        $version = $lines->[0];
        chomp($version);
    }
    elsif ( $osType eq 'AIX' )
    {
	$distro = 'aix';

        my @lines;
        @lines = `uname -v`;
        my $versionPart = $lines[0];
        chomp($versionPart);
        @lines = `uname -r`;
        my $releasePart = $lines[0];
        chomp($releasePart);
        $version = "$versionPart.$releasePart";
    }
    elsif ( $osType eq 'SunOS' )
    {
	$distro = 'solaris';

        my @lines = `uname -r`;
        $version = $lines[0];
        chomp($version);
    }
    elsif ( $osType eq 'Darwin' )
    {
        $distro = 'darwin';

        my @lines = `uname -r`;
        $version = $lines[0];
        chomp($version);
        $version =~ s/^8.(\d+).(\d+)$/8.x.x/;
    }
    elsif ( $osType eq 'HP-UX' )
    {
        $distro = 'hpux';

        my @lines = `uname -r`;
        $version = $lines[0];
        chomp($version);
	$version =~ s/(\D+)//;
    }
    elsif ( $osType eq 'Linux' )
    {
        my $RELEASE_INFO =
        {
         order => [ qw( rhel redhat fedora centos suse opensuse sles sled ubuntu debian ) ],
         files =>
         {
          rhel => '/etc/redhat-release',
          redhat => '/etc/redhat-release',
          fedora => '/etc/redhat-release',
          centos => '/etc/redhat-release',
          suse => '/etc/SuSE-release',
          opensuse => '/etc/SuSE-release',
          sles => '/etc/SuSE-release',
          sled => '/etc/SuSE-release',
          ubuntu => '/etc/lsb-release',
          debian => '/etc/debian_version',
         },
        };

	foreach my $distroType (@{$RELEASE_INFO->{order}})
	{
	    my $relFile = $testPrefix.$RELEASE_INFO->{files}->{$distroType};
	    next if not -r $relFile;

            my $text = join('', @{ReadFile($relFile)});

            if ($distroType eq "rhel")
            {
                # The format of the line is something like:
                #   Red Hat Enterprise Linux ES release 4 (Nahant Update 1)
                #   Red Hat Advanced Server release 2.1AS (Pensacola)
                # In addition, Oracle Linux reports itself as:
                #   Enterprise Linux Enterprise Linux AS release 4 (October Update 5)
                if ($text =~ /^\s*((Red Hat)|(Enterprise Linux)) ((Enterprise Linux \S+)|(Linux (Advanced|Enterprise) Server)) release (\S+)/)
                {
                    $distro = $distroType;
                    $version = $8;
                    $version =~ s/(AS)|(ES)$//;
                    $version =~ s/^(\d+)\.\d+$/$1/;
                    last;
                }
            }
            elsif ($distroType eq "redhat")
            {
                # The format of the line is something like:
                #   Red Hat Linux release 7.3 (Valhala)
                if ($text =~ /^\s*Red Hat Linux release (\d+(\.\d+)?)/)
                {
                    $distro = $distroType;
                    $version = $1;
                    last;
                }
            }
            elsif ($distroType eq "fedora")
            {
                # The format of the line is something like:
                #   Fedora Core release 4 (Stentz)
                if ($text =~ /^\s*Fedora (Core )?release (\S+)/)
                {
                    $distro = $distroType;
                    $version = $2;
                    last;
                }
            }
            elsif ($distroType eq "centos")
            {
                # The format of the line is something like:
                #   CentOS release 4.x (Final)
                if ($text =~ /^\s*CentOS release (\S+)/)
                {
                    $distro = $distroType;
                    $version = $1;
                    $version =~ s/^(\d+)\.\d+$/$1/;
                    last;
                }
            }
            elsif ($distroType eq "suse")
            {
                if ($text =~ /^\s*SUSE LINUX (\d+\.\d+)\s+/i)
                {
                    $distro = $distroType;
                    $version = $1;
                    last;
                }
            }
            elsif ($distroType eq "opensuse")
            {
                if ($text =~ /^\s*openSUSE (\d+\.\d+)\s+/i)
                {
                    $distro = $distroType;
                    $version = $1;
                    last;
                }
            }
            elsif  ($distroType eq "sles")
            {
                if ($text =~ /^\s*SUSE LINUX Enterprise Server (\d+)\s+/i)
                {
                    $distro = $distroType;
                    $version = $1;
                    last;
                }
            }
            elsif  ($distroType eq "sled")
            {
                if ($text =~ /^\s*SUSE LINUX Enterprise Desktop (\d+)\s+/i)
                {
                    $distro = $distroType;
                    $version = $1;
                    last;
                }
            }
            elsif ($distroType eq "ubuntu")
            {
                # The file will have lines that include:
                #   DISTRIB_ID=Ubuntu
                #   DISTRIB_RELEASE=6.06
                if ($text=~ /^\s*DISTRIB_ID\s*=\s*Ubuntu\s*$/m)
                {
                    $distro = $distroType;
                }
                if ($text=~ /^\s*DISTRIB_RELEASE\s*=\s*(\S+)\s*$/m)
                {
                    $version = $1;
                }
                last if (defined($distro) || defined($version));
            }
            elsif ($distroType eq "debian")
            {
                my $lsb_release = $RELEASE_INFO->{files}->{ubuntu};
                #
                # Debian and Ubuntu both have /etc/debian_version,
                # but only Ubuntu has an /etc/lsb-release
                #
                if ( -r $lsb_release )
                {
                    die "Unexpetected file: $lsb_release"
                }

                $distro = $distroType;

                # The format of the entire file is a single line like:
                # 3.1
                # and nothing else, so that is the version
                $version = $text;
                chomp($version);
                last;
            }
            if (defined($distro) || defined($version))
            {
                die "Should have bailed: [$distro, $version]\n";
            }
	}
        if (not defined($distro))
        {
            # Support a catch-all for Linux distros:
            $distro = "other";
            $version = "0";
        }
    }

    if (not defined($distro))
    {
	die "Unable to determine distribution type for OS \"$osType\"";
    }
    if (not defined($version))
    {
	die "Unable to determine version for OS \"$osType\" and distro \"$distro\"";
    }

    return { os => $osType, distro => $distro, version => $version };
}


sub GetDistroType(;$)
{
    my $testPrefix = shift || '';
    return GetDistroInfo($testPrefix)->{distro};
}


sub GetDistroVersion(;$)
{
    my $testPrefix = shift || '';
    return GetDistroInfo($testPrefix)->{version};
}


sub ReadFile($)
{
    my $file = shift || die;

    my $lines = [];
    my $line;

    open(FILE, "<$file") || die "Unable to open $file";
    while ( $line = <FILE>)
    {
        push(@$lines, $line);
    }
    close(FILE);

    die "Empty file: $file" if not $lines or not @$lines;

    return $lines;
}


sub WriteFile($$)
{
    my $file = shift || die;
    my $lines = shift || die;

    open(FILE, ">$file") || die "Unable to open $file";
    foreach my $line (@$lines)
    {
        chomp($line);
        print FILE "$line\n";
    }
    close(FILE);
}


sub ReplaceFileInternal($$)
{
    my $file = shift || die;
    my $newfile = shift || die;

    my $orig = "$file.lwidentity.orig";
    my $backup = "$file.lwidentity.bak";

    if ( ! -e $orig )
    {
        $backup = $orig;
    }

    if ( -e $file or -l $file )
    {
	rename($file, $backup) || die "Unable to rename $file to $backup\n";
    }
    rename($newfile, $file) || die "Unable to rename $newfile to $file\n";
}


sub ReplaceFile($$)
{
    my $file = shift || die;
    my $lines = shift || die;

    #
    # TODO
    #
    # - Perhaps write out new file to /tmp.
    # - What if backup file is already there?
    #

    my $newfile = "$file.new";

    WriteFile( $newfile, $lines );
    ReplaceFileInternal($file, $newfile);
}


sub ReplaceSymlink($$)
{
    my $link = shift || die;
    my $target = shift || die;

    if ( -e $link and -l $link )
    {
        my $currentTarget = readlink($link);
        if ( $currentTarget eq $target )
        {
            return 0;
        }
    }

    my $newlink = "$link.new";

    symlink($target, $newlink) || die "Unable to create symlink $newlink -> $target\n";
    ReplaceFileInternal($link, $newlink);
    return 1;
}


sub GetFileParseMessage($;$$$)
{
    my $message = shift || die;
    my $file = shift;
    my $lineNumber = shift;
    my $line = shift;

    if ( $file )
    {
	$message .= " in file \"$file\"";
    }

    if ( $lineNumber )
    {
        $message .= " at line $lineNumber";
    }

    $message .= "\n";

    if ( defined($line) )
    {
        chomp($line);
        $message .= "    line contains \"$line\"\n";
    }

    return $message;
}


sub QuickBaseName($)
    # this is just so we do not have to depend on File::basename
{
    my $path = shift || die;
    return ( reverse(split('/', $path)) )[0];
}


sub CheckTestPrefix($)
{
    my $prefix = shift || '';

    if ( $prefix )
    {
	if ( not -d $prefix )
	{
	    die "Invalid test mode directory specified: \"$prefix\"\n";
	}
	print "Using test mode with prefix=\"$prefix\"\n";
    }

    return $prefix;
}


sub CreateGlobalOpts($)
{
    my $opts = shift || die;

    return { debug => $opts->{debug} ? 1 : 0,
             test => $opts->{test} ? 1 : 0 };
}


sub CompareLines($$)
{
    my $lines1 = shift || die;
    my $lines2 = shift || die;

    if ( $#{@$lines1} != $#{@$lines2} )
    {
        return 1;
    }

    my $last_index = $#{@$lines1};
    for (my $i = 0; $i <= $last_index; $i++)
    {
        my $line1 = $lines1->[$i];
        my $line2 = $lines2->[$i];
        chomp($line1);
        chomp($line2);

        if ( not ( $line1 eq $line2 ) )
        {
            return 1;
        }
    }

    return 0;
}


1;
