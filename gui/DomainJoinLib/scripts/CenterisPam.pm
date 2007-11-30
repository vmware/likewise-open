##
## Copyright (C) Centeris Corporation 2004-2007
## Copyright (C) Likewise Software 2007.  
## All rights reserved.
## 
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
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

package CenterisPam;

use strict;
use warnings;

use Centeris;

BEGIN {
    use Exporter ();
    our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);

    #
    # Version for version checking
    #

    $VERSION = 1.00;

    @ISA = qw(Exporter);
    @EXPORT = qw(&BuildPamdDeleteChange &BuildPamdControlChange BuildPamdSubOptionsChange &BuildPamdChange &BuildPamdChangeSequence &BuildPamUnix2Change &BuildPamUnix2ChangeSequence &ModifyPamFiles);
    %EXPORT_TAGS = ();

    #
    # Exported package globals and optionally exported functions
    #

    @EXPORT_OK = qw();
}
our @EXPORT_OK;


#
# There are 2 types of pam configuration files that we handle:
#
# 1) pam.d (/etc/pam.d) format.  Here, each file would contain non-comment,
#    non-whitespace lines of the format "type control module options".
#
# 2) pam_unix2.conf format -- Non-comment and non-whitespace lines are of the
#    form: "PamType: options".  (e.g., "auth: nullok").  This format is used
#    for old-school distributions.  call_modules=mod,mod,mod is the option
#    about which we care.
#
# For a given type of file we want to change, we express a list of changes
# that we want to make.
#
# For pam.d file type, each change directive has an "after" contraint that
# expresses the directive after which we want to insert the desired module
# directive.
#
# For pam_unix2.conf file type, we just express which modules we want called.
# For now, we will pre-prend to any existing list of modules, printing out a
# warning.  (Note: We probably should have option to die...)
#
# $PAMD_TYPE_CONFIG_ITEM_EXAMPLE and $PAM_UNIX2_TYPE_CONFIG_ITEM_EXAMPLE below
# are just examples of a "config item" that changes a set of files.  These
# "config items" are combined into "config lists" that describe the
# configuration necessary for a given OS distribution.
#

my $PAMD_TYPE_CONFIG_ITEM_EXAMPLE =
{
 files => [ '/etc/pam.d/somefile' ],
 changes =>
 {
  file_type => 'pam.d',
  list => [
           {
            op => 'after',
            type => 'account',
            control => 'sufficient',
            module => 'pam_winbind.so',
            options => '',
            op_module => 'pam_unix.so',
           },

           {
            op => 'after',
            type => 'auth',
            control => 'sufficient',
            module => 'pam_krb5.so',
            options => 'try_first_pass',
            op_module => 'pam_unix.so',
           },
           {
            op => 'after',
            type => 'auth',
            control => 'sufficient',
            module => 'pam_winbind.so',
            options => 'try_first_pass',
            op_module => 'pam_krb5.so',
           },

           {
            op => 'after',
            type => 'session',
            control => 'required',
            module => 'pam_mkhomedir.so',
            options => 'skel=/etc/skel/ umask=0022',
            op_module => 'pam_limits.so',
           },
           {
            op => 'after',
            type => 'session',
            control => 'optional',
            module => 'pam_krb5.so',
            options => '',
            op_module => 'pam_mkhomedir.so',
           },
          ],
 },
};

my $PAM_UNIX2_TYPE_CONFIG_ITEM_EXAMPLE =
{
 files => [ '/etc/security/pam_unix2.conf' ],
 changes =>
 {
  file_type => 'pam_unix2.conf',
  list => [
           {
            type => 'account',
            call_modules => [ 'winbind' ],
           },
           {
            type => 'auth',
            call_modules => [ 'pam_krb5', 'winbind' ],
           },
          ],
 },
};

my $PAMCONF_TYPE_CONFIG_ITEM_EXAMPLE =
{
 files => [ '/etc/pam.conf' ],
 changes =>
 {
  file_type => 'pam.conf',
  list => [
           {
            op => 'after',
            service => 'login',
            type => 'auth',
            control => 'sufficient',
            module => 'pam_winbind.so',
            options => '',
            op_module => 'pam_authtok_get.so.1',
           },
          ],
 },
};


my $OP_DELETE = 'delete';
my $OP_CONTROL = 'control';
my $OP_AFTER = 'after';
my $OP_BEFORE = 'before';
my $OP_SUB_OPTIONS = 'sub_options';


sub UnparsePamdLine($$$$$)
{
    my $service = shift || '';
    my $type = shift || die;
    my $control = shift || die;
    my $module = shift || die;
    my $options = shift;

    my $line = "$type\t$control\t$module";
    if ( $service )
    {
        $line = "$service\t$line";
    }
    if ( defined($options) )
    {
        $line .= "\t$options";
    }

    return $line;
}


sub ParsePamdLineNoComments($;$)
{
    my $line = shift;
    my $isConfMode = shift || 0;

    if ( $isConfMode )
    {
        if ( $line =~ /^\s*(\S+)\s+(\S+)\s+(\S+)\s+(\S+)(\s+(.*))?\s*$/ )
        {
            return {
                    service => $1,
                    type => $2,
                    control => $3,
                    module => $4,
                    options => $5 ? ( $6 ? $6 : undef ) : undef,
                   };
        }
    }
    else
    {
	if ( $line =~ /^\s*(\S+)\s+\[(.*)\]\s+(\S+)(\s+(.*))?\s*$/ )
	{
	    if ( $2 =~ /[\[\]]/ )
	    {
		die "ERROR: Nested square braces in expression: $line";
	    }
            return {
                    type => $1,
                    control => '['.$2.']',
                    module => $3,
                    options => $4 ? ( $5 ? $5 : undef ) : undef,
                   };
        }
        elsif ( $line =~ /^\s*(\S+)\s+(\S+)\s+(\S+)(\s+(.*))?\s*$/ )
        {
            return {
                    type => $1,
                    control => $2,
                    module => $3,
                    options => $4 ? ( $5 ? $5 : undef ) : undef,
                   };
        }
    }

    return 0;
}


sub ParsePamUnix2Options($;$$$)
{
    my $options = shift;
    my $file = shift;
    my $lineNumber = shift;
    my $line = shift;

    #
    # Remove leading and trailing whitespace
    #

    $options =~ s/^\s+//;
    $options =~ s/\s+$//;

    my @optionsList = split(/\s+/, $options);
    my $parsedOptions = { value => {}, list => [] };

    foreach my $option (@optionsList)
    {
        die if not $option;

        if ( $option =~ /^(call_modules)=(.+)$/ )
        {
            if ( $2 )
            {
                print GetFileParseMessage("WARNING: Multiple call_modules options", $file, $lineNumber, $line) if $parsedOptions->{hash}->{$1} and $file;

                $parsedOptions->{value}->{$1} = [ split(/,/, $2) ];
                push(@{$parsedOptions->{list}}, $1);
            }
            else
            {
                print GetFileParseMessage("WARNING: Empty call_modules option", $file, $lineNumber, $line) if $file;
            }
        }
        else
        {
            $parsedOptions->{value}->{$option} = $option;
            push(@{$parsedOptions->{list}}, $option);
        }
    }

    return $parsedOptions;
}


sub UnparsePamUnix2Options($)
{
    my $parsedOptions = shift;

    my $result = '';

    foreach my $option (@{$parsedOptions->{list}})
    {
        if ( ref($parsedOptions->{value}->{$option}) eq 'ARRAY' )
        {
            $result .= " $option=".join(',', @{$parsedOptions->{value}->{$option}} );
        }
        else
        {
            $result .= " $option";
        }
    }

    return $result;
}


sub BuildPamdDeleteChange($$)
{
    my $type = shift || die;
    my $module = shift || die;

    return {
            op => $OP_DELETE,
            type => $type,
            module => $module,
           };
}


sub BuildPamdControlChange($$$)
{
    my $type = shift || die;
    my $control = shift || die;
    my $module = shift || die;

    return {
            op => $OP_CONTROL,
            type => $type,
            control => $control,
            module => $module,
           };
}


sub BuildPamdSubOptionsChange($$$)
{
    my $type = shift || die;
    my $module = shift || die;
    my $sub = shift || die;

    die if not ref($sub);
    die if not ( 'CODE' eq ref($sub) );

    return {
            op => $OP_SUB_OPTIONS,
            type => $type,
            module => $module,
            options => $sub,
           };
}


sub BuildPamdChange($$$)
{
    my $op = shift || die;
    my $op_module = shift || die;
    my $line = shift || die;

    die if not ( $op eq $OP_AFTER or $op eq $OP_BEFORE );

    my $change = ParsePamdLineNoComments($line);

    die if not $change;

    $change->{op} = $op;
    $change->{op_module} = $op_module;

    return $change;
}


sub BuildPamdChangeSequence($$@)
{
    my $op = shift || die;
    my $op_module = shift || die;
    my @lines = @_;

    die if not @lines;

    my @changes;

    foreach my $line (@lines)
    {
        my $change = BuildPamdChange($op, $op_module, $line);
        die if not $change;
        push(@changes, $change);
        # Make sure we just have the basename
        $op_module = QuickBaseName($change->{module});
    }

    return @changes;
}


sub BuildPamUnix2Change($)
{
    my $line = shift || die;

    if ( $line =~ /^\s*([^:]+):(.*)?$/ )
    {
        my $type = $1;
        my $options = $2 ? $2 : '';

        die if not $options;

        my $parsedOptions = ParsePamUnix2Options($options);
        die if $#{@{$parsedOptions->{list}}} != 0;
        die if not $parsedOptions->{value}->{call_modules};
        die if $#{@{$parsedOptions->{value}->{call_modules}}} < 0;

        return {
                type => $type,
                call_modules => [ @{$parsedOptions->{value}->{call_modules}} ],
               };
    }
    else
    {
        die;
    }
}


sub BuildPamUnix2ChangeSequence($@)
{
    my @lines = @_;

    die if not @lines;

    my @changes;

    foreach my $line (@lines)
    {
        my $change = BuildPamUnix2Change($line);
        push(@changes, $change);
    }

    return @changes;
}


sub ProcessPamdFilePass($$$$$)
{
    my $lines = shift || die;
    my $fileType = shift || die;
    my $item = shift || die;
    my $file = shift || die;
    my $isInsertPass = shift;

    my $op = $item->{op} || die;
    my $op_module = $item->{op_module} || '';
    my $service = $item->{service} || '';
    my $type = $item->{type} || die;
    my $control = $item->{control} || '';
    my $module_path = $item->{module} || die;
    my $options = $item->{options} || undef;

    my $module = QuickBaseName($module_path) || die;

    if ( $isInsertPass )
    {
        die if not $op_module;
        die if not $control;
    }

    my $lineNumber = 0;

    my $newLines = [];

    my $changeCount = 0;
    my $insertCount = 0;
    my $removeCount = 0;
    my $removedLines = [];

    # XXX - print "NEED: \"$type\", \"$op\", \"$op_module\"\n";

    foreach my $line (@$lines)
    {
        chomp($line);
        $lineNumber++;

        my $originalLine = $line;

        if ( $line =~ /^([^#]*)#.*$/ )
        {
            $line = $1 ? $1 : '';
        }

        $line =~ s/^\s*//;
        $line =~ s/\s*$//;

        if ( not $isInsertPass )
        {
            if ( $line =~ /^(.*)\\$/ )
            {
                #
                # Line extension character.
                #
                # This should at least get logged.  Do not die for backwards
                # compatibility.  A proper parser would be ideal.
                #
                print GetFileParseMessage( "WARNING: Line continuation found",
                                           $file, $lineNumber, $originalLine );
            }

            if ( $line =~ /\[/ )
            {
                #
                # Token quoting character.
                #
                # This should at least get logged.  Do not die for backwards
                # compatibility.  A proper parser would be ideal.
                #
                print GetFileParseMessage( "WARNING: Token quoting character found",
                                           $file, $lineNumber, $originalLine );
            }
        }

        my $parsedLine = ParsePamdLineNoComments($line, $fileType eq 'pam.conf');
        if ( $parsedLine )
        {
            my $got_service = $parsedLine->{service} || '';
            my $got_type = $parsedLine->{type} || die;
            my $got_control = $parsedLine->{control} || die;
            my $got_module = $parsedLine->{module} || die;
            my $got_options = $parsedLine->{options};

            my $original_got_module = $got_module;
            $got_module = QuickBaseName($got_module);

            # XXX - print "GOT: \"$got_type\", \"$got_module\"\n";

            if ( ( ($got_service eq $service) or (not $service) ) and
                 ($got_type eq $type) and ($got_module eq $module) )
            {
                if ( $op eq $OP_CONTROL )
                {
                    if ( $got_control eq $control )
                    {
                        push(@$newLines, $originalLine);
                    }
                    else
                    {
                        my $newLine = $originalLine;
                        $newLine =~ s/$got_control/$control/;

                        $changeCount++;
                        push(@$newLines, $newLine);
                    }
                }
                elsif ( $op eq $OP_SUB_OPTIONS )
                {
                    if ( not defined $options )
                    {
                        push(@$newLines, $originalLine);
                    }
                    else
                    {
                        my $newOptions = &$options($got_options, \$options);
                        if ( ( defined $newOptions != defined $got_options ) or
                             ( ( defined $newOptions and defined $got_options ) and
                               not ( $newOptions eq $got_options ) ) )
                        {
                            my $newLine = UnparsePamdLine($got_service, $got_type, $got_control, $original_got_module, $newOptions);
                            $changeCount++;
                            push(@$newLines, $newLine);
                        }
                        else
                        {
                            push(@$newLines, $originalLine);
                        }
                    }
                }
                else
                {
                    #
                    # The line we are replacing should go away (modulo any options
                    # or control options we wish to carry forward).  If the line
                    # does not match exactly, we should probably log that we are
                    # replacing the line.
                    #
                    # TODO
                    #
                    print GetFileParseMessage( "INFO: Removing line for $module",
                                               $file, $lineNumber, $originalLine );

                    #
                    # The line should have been removed before the insert pass
                    #

                    die if $isInsertPass;

                    #
                    # TODO-Ideally, die here.
                    #

                    print GetFileParseMessage( "ERROR: Multiple remove points found for $module",
                                               $file, $lineNumber, $originalLine ) if $removeCount;

                    $removeCount++;
                    push(@$removedLines, { line => $originalLine,
                                           service => $got_service,
                                           type => $got_type,
                                           control => $got_control,
                                           module => $original_got_module,
                                           options => $got_options } );
                }
            }
            else
            {
                my $func = sub
                {
                    my $op_module = shift || die;

                    if ( $isInsertPass and ($got_service eq $service) and ($got_type eq $type) and ($got_module eq $op_module) )
                    {
                        die GetFileParseMessage( "ERROR: Multiple insertion points found for $module",
                                                 $file, $lineNumber, $originalLine ) if $insertCount;

                        $insertCount++;
                        push(@$newLines, UnparsePamdLine($service, $type, $control, $module_path, $options));
                    }
                };

                if ( $op eq $OP_BEFORE )
                {
                    &$func($op_module);
                }
                push(@$newLines, $originalLine);
                if ( $op eq $OP_AFTER )
                {
                    &$func($op_module);
                }
            }
        }
        else
        {
            push(@$newLines, $originalLine);

            print GetFileParseMessage( "WARNING: Could not parse line",
                                       $file, $lineNumber, $originalLine ) if not $line =~ /^$/;
        }
    }

    return {
            lines => $newLines,
            removedLines => ( $removeCount > 0 ) ? $removedLines : undef,
            removeCount => $removeCount,
            insertCount => $insertCount,
            changeCount => $changeCount,
            modifyCount => $removeCount + $insertCount + $changeCount,
           };
}


sub DumpFileLines($$$)
{
    my $prefix = shift;
    my $file = shift || die;
    my $lines = shift || die;

    print "$prefix " if $prefix;
    print "Contents for $file:\n";
    print '-' x 40 . "\n";
    foreach my $line (@$lines)
    {
        chomp($line);
        print "$line\n";
    }
    print '-' x 40 . "\n";
}

sub ProcessPamdFile($$$;$)
{
    my $file = shift || die;
    my $fileType = shift || die;
    my $changeList = shift || die;
    my $opt = shift || {};

    my $lines = ReadFile($file);
    my $modifyCount = 0;

    my $originalLines = [];
    @$originalLines = @$lines;

    foreach my $item (@$changeList)
    {
        my $op = $item->{op} || die;
        my $op_module = $item->{op_module} || '';
        my $before = $item->{before} || '';
        my $type = $item->{type} || die;
        my $control = $item->{control} || '';
        my $module = $item->{module} || die;
        my $options = $item->{options} || '';
        my $service = $item->{service} || '';

        my $isInsert = 0;

        if ( $op eq $OP_AFTER || $op eq $OP_BEFORE )
        {
            die if not $op_module;
            die if not $control;
            $isInsert = 1;
        }
        elsif ( $op eq $OP_CONTROL )
        {
            die if not $control;
        }
        elsif ( $op eq $OP_DELETE )
        {
            # nothing
        }
        elsif ( $op eq $OP_SUB_OPTIONS )
        {
            die if not $options;
            die if not ref($options);
            die if not ('CODE' eq ref($options));
        }
        else
        {
            die "Bad op: $op";
        }

        my $result;

        #
        # We will do 2 passes.  The first pass will remove an existing line
        # and capture any info we may want to merge.  The second pass will
        # do the insertion.
        #

        #DumpFileLines( '', $file, $lines ) if $opt->{debug};

        $result = ProcessPamdFilePass( $lines, $fileType, $item, $file, 0 );
        die if $result->{insertCount};

        if ( $result->{modifyCount} )
        {
            $modifyCount += $result->{modifyCount};
        }

        if ( $isInsert )
        {
            if ( $result->{removedLines} )
            {
                #
                # TODO-Merge options and/or control for anything that was removed
                #
            }

            @$lines = @{$result->{lines}};
            $result = ProcessPamdFilePass( $lines, $fileType, $item, $file, 1 );

            #DumpFileLines( '', $file, $lines ) if $opt->{debug};

            die "ERROR: Could not find insertion point $op $op_module for $module in $file ($service $type)\n" if not $result->{insertCount};

            if ( $result->{modifyCount} )
            {
                $modifyCount += $result->{modifyCount};
            }
        }

        @$lines = @{$result->{lines}};
    }

    if ( $modifyCount )
    {
        my $isDifferent = CompareLines( $originalLines, $lines );
        if ( not $isDifferent )
        {
            $modifyCount = 0;
        }
    }

    if ( $modifyCount )
    {
        DumpFileLines( 'New', $file, $lines ) if $opt->{debug};
        print "Modified $file\n";
        ReplaceFile( $file, $lines ) if not $opt->{test};
    }
    else
    {
        print "No need to modify $file\n";
    }
}


sub IsEqualListText($$)
{
    my $list1 = shift;
    my $list2 = shift;

    return 0 if $#{@{$list1}} != $#{@{$list2}};

    for (0..$#{@{$list1}})
    {
        return 0 if not ($list1->[$_] eq $list2->[$_]);
    }

    return 1;
}


sub GetCountHashFromList($)
{
    my $list = shift;

    my $result = {};
    map { $result->{$_}++; } @$list;
    return $result;
}


sub ProcessPamUnix2File($$;$)
{
    my $file = shift || die;
    my $changeList = shift || die;
    my $opt = shift || {};

    my $lines = ReadFile($file);
    my $modifyCount = 0;

    my $originalLines = [];
    @$originalLines = @$lines;

    foreach my $item (@$changeList)
    {
        my $type = $item->{type} || die;
        my $modules = $item->{call_modules} || die;

        my $newLines = [];
        my $lineNumber = 0;

        my $found = 0;

        foreach my $line (@$lines)
        {
            chomp($line);
            $lineNumber++;

            my $originalLine = $line;

            my $modify = 0;
            my $parsedOptions;

            if ( $line =~ /^([^#])*#.*$/ )
            {
                $line = $1 ? $1 : '';
            }

            $line =~ s/^\s*//;
            $line =~ s/\s*$//;

            #
            # We will only ever modify one line, and it will
            # be the first candidate.  However, we will warn
            # on other matching lines for good measure.
            #

            if ( $line =~ /^$type:(.*)$/ )
            {
                if ( $found )
                {
                    print GetFileParseMessage( "WARNING: Multiple lines of type $type", $file, $lineNumber, $originalLine );
                }
                else
                {
                    my $options = $1 ? $1 : '';
                    my $useDefault;

                    $found = 1;

                    if ( $options )
                    {
                        $parsedOptions = ParsePamUnix2Options($options, $file, $lineNumber, $originalLine);

                        if ( $parsedOptions->{value}->{call_modules} )
                        {
                            if ( not IsEqualListText($modules, $parsedOptions->{value}->{call_modules}) )
                            {
                                $parsedOptions->{value}->{call_modules} = [ @$modules ];
                                $modify = 1;
                            }
                        }
                        else
                        {
                            $useDefault = 1;
                        }
                    }
                    else
                    {
                        $useDefault = 1;
                    }

                    if ( $useDefault )
                    {
                        unshift(@{$parsedOptions->{list}}, 'call_modules');
                        $parsedOptions->{value}->{call_modules} = [ @$modules ];
                        $modify = 1;
                    }
                }
            }

            if ( $modify )
            {
                die if not $parsedOptions;
                push(@$newLines, "$type: ".UnparsePamUnix2Options($parsedOptions));
                $modifyCount++;
            }
            else
            {
                push(@$newLines, $originalLine);
            }
        }

        @$lines = @$newLines;
    }

    if ( $modifyCount )
    {
        my $isDifferent = CompareLines( $originalLines, $lines );
        if ( not $isDifferent )
        {
            $modifyCount = 0;
        }
    }

    if ( $modifyCount )
    {
        DumpFileLines( 'New', $file, $lines ) if $opt->{debug};
        print "Modified $file\n";
        ReplaceFile( $file, $lines ) if not $opt->{test};
    }
    else
    {
        print "No need to modify $file\n";
    }
}


sub ModifyPamFiles($;$$)
{
    my $configListForDistros = shift || die;
    my $testPrefix = shift || '';
    my $opt = shift || {};

    my $osType = GetOsType($testPrefix) || die;
    my $distroType = GetDistroType($testPrefix) || die;
    my $distroVersion = GetDistroVersion($testPrefix) || die;

    my $configList = $configListForDistros->{$osType}->{$distroType}->{$distroVersion} || die "invalid (OS,distro,version): (\"$osType\", \"$distroType\", \"$distroVersion\")";

    foreach my $configItem (@$configList)
    {
        my $files = $configItem->{files};
        my $changes = $configItem->{changes};
        my $filter = $configItem->{filter};

        die if not (ref($files) eq 'ARRAY');
        die if not (ref($changes) eq 'HASH');
        die if $filter and ( not (ref($filter) eq 'CODE') );

        my $fileType = $changes->{file_type};
        my $changeList = $changes->{list};

        die if not (ref($changeList) eq 'ARRAY');

        foreach my $file (@$files)
        {
            if ( $filter )
            {
                my $continue = &$filter( $file, $fileType, $distroType, $distroVersion, $testPrefix );
                next if not $continue;
            }

            $file = $testPrefix.$file;

            if ( $fileType eq 'pam.d' or $fileType eq 'pam.conf' )
            {
                ProcessPamdFile( $file, $fileType, $changeList, $opt );
            }
            elsif ( $fileType eq 'pam_unix2.conf' )
            {
                ProcessPamUnix2File( $file, $changeList, $opt );
            }
            else
            {
                die "Invalid type \"$fileType\"";
            }
        }
    }

    return 0; # success
}


1;
