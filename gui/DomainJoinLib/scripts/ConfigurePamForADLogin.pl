#
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

use strict;
use warnings;

use FindBin;
use lib "$FindBin::Bin";
use CenterisPam;

my $PAM_IDENTITY_MODULE_NAME = 'lwidentity';
my $PAM_PASSPOLICY_MODULE_NAME = 'lwipasspolicy';
my $PAM_IDENTITY_SHARED_LIBRARY = 'pam_'.$PAM_IDENTITY_MODULE_NAME.'.so';
my $PAM_PASSPOLICY_SHARED_LIBRARY = 'pam_'.$PAM_PASSPOLICY_MODULE_NAME.'.so';

my $OP_AFTER = 'after';
my $OP_BEFORE = 'before';
my $OP_DELETE = 'delete';

sub main();
exit(main());


sub GetAixConfigList($);
sub GetSolarisConfigList($);
sub GetRedhatConfigList($;$);
sub GetSuseOldConfigList($;$$$$);
sub GetSuseConfigList($);
sub GetDebianConfigList($);
sub GetDarwinConfigList($;$);
sub GetOldHpuxConfigList($);
sub GetHpuxConfigList($);

sub CreateConfigList($)
{
    my $enable = shift || 0;

    return
    {
     AIX =>
     {
      aix =>
      {
       '5.3' => GetAixConfigList( $enable ),
      },
     },

     SunOS =>
     {
      solaris =>
      {
       '5.8' => GetSolarisConfigList( $enable ),
       '5.9' => GetSolarisConfigList( $enable ),
       '5.10' => GetSolarisConfigList( $enable ),
      },
     },
     "HP-UX" =>
     {
	 hpux =>
	 {
	     '11.11' => GetOldHpuxConfigList( $enable ),
	     '11.23' => GetHpuxConfigList( $enable ),
	     '11.31' => GetHpuxConfigList( $enable ),
	 },
     },

     Linux =>
     {
      redhat =>
      {
       '7.2' => GetRedhatConfigList( $enable ),
       '7.3' => GetRedhatConfigList( $enable ),
       '8.0' => GetRedhatConfigList( $enable ),
       '9' => GetRedhatConfigList( $enable ),
      },

      rhel =>
      {
       2 => GetRedhatConfigList( $enable ),
       3 => GetRedhatConfigList( $enable ),
       4 => GetRedhatConfigList( $enable ),
       5 => GetRedhatConfigList( $enable ),
      },

      fedora =>
      {
       3 => GetRedhatConfigList( $enable ),
       4 => GetRedhatConfigList( $enable ),
       5 => GetRedhatConfigList( $enable ),
       6 => GetRedhatConfigList( $enable ),
       7 => GetRedhatConfigList( $enable ),
      },

      centos =>
      {
       3 => GetRedhatConfigList( $enable ),
       4 => GetRedhatConfigList( $enable ),
       5 => GetRedhatConfigList( $enable ),
      },

      suse =>
      {
       '8.0'  => GetSuseOldConfigList( $enable, 'pam_unix.so', 'pam_unix.so', 'pam_unix.so', 'pam_unix.so' ),       
       '8.1'  => GetSuseOldConfigList( $enable, 'pam_unix.so', 'pam_unix.so', 'pam_unix.so' ),
       '8.2'  => GetSuseOldConfigList( $enable, 'pam_unix2.so' ), # xdm does not have pam_pwcheck.so
       '9.0'  => GetSuseOldConfigList( $enable, 'pam_unix2.so' ), # xdm does not have pam_pwcheck.so
       '9.1'  => GetSuseOldConfigList( $enable ),
       '9.2'  => GetSuseOldConfigList( $enable ),
       '9.3'  => GetSuseConfigList( $enable ),
       '10.0' => GetSuseConfigList( $enable ),
       '10.1' => GetSuseConfigList( $enable ),
       '10.2' => GetSuseConfigList( $enable ),
      },

      opensuse =>
      {
       '10.2' => GetSuseConfigList( $enable ),
      },

      sles =>
      {
       9 => GetSuseOldConfigList( $enable ),
       10 => GetSuseConfigList( $enable ),
      },

      sled =>
      {
       10 => GetSuseConfigList( $enable ),
      },

      ubuntu =>
      {
       '6.06' => GetDebianConfigList( $enable ),
       '6.10' => GetDebianConfigList( $enable ),
       '7.04' => GetDebianConfigList( $enable ),
      },

      debian =>
      {
       '3.1' => GetDebianConfigList( $enable ),
       '4.0' => GetDebianConfigList( $enable ),
       'lenny/sid' => GetDebianConfigList ( $enable ),
      },
     },

     Darwin =>
     {
      darwin =>
      {
       '8.x.x' => GetDarwinConfigList( $enable, 1 ),
      },
     }
    };
}


sub GetPamdChanges($$$;$)
    # returns 'changes' part of a "config item"
{
    my $enable = shift || 0;
    my $op = shift || die;
    my $op_modules = shift || die;
    my $opt = shift || undef;

    die if ( not (ref($op_modules) eq 'HASH') );

    my $account = $op_modules->{account}; # 'pam_unix.so';
    my $auth = $op_modules->{auth}; # 'pam_unix.so';
    my $session = $op_modules->{session}; # 'pam_limits.so';
    my $password = $op_modules->{password};

    my $list = [];

    if ( $enable )
    {
        if ( $account )
        {
            my @lines;
            push(@lines, 'account sufficient '.$PAM_IDENTITY_SHARED_LIBRARY);
            push(@lines, 'account requisite '.$PAM_PASSPOLICY_SHARED_LIBRARY) if not $opt->{no_passpolicy};

            push(@$list, BuildPamdChangeSequence($op, $account, @lines));
        }

        if ( $auth )
        {
            my @lines;
            push(@lines, 'auth sufficient '.$PAM_IDENTITY_SHARED_LIBRARY);

            push(@$list, BuildPamdChangeSequence($op, $auth, @lines));
        }

        if ( $session )
        {
            my @lines;
            push(@lines, 'session optional '.$PAM_IDENTITY_SHARED_LIBRARY);

            push(@$list, BuildPamdChangeSequence($op, $session, @lines));
        }

        if ( $password )
        {
            my @lines;

	    if( $op eq $OP_BEFORE && ( $password eq 'pam_pwcheck.so' || $password eq 'pam_cracklib.so' ))
	    {
		push(@lines, 'password [success=1 new_authtok_reqd=ok ignore=ignore default=die] '.$PAM_PASSPOLICY_SHARED_LIBRARY) if not $opt->{no_passpolicy};
	    }
	    else
	    {
		push(@lines, 'password required '.$PAM_PASSPOLICY_SHARED_LIBRARY) if not $opt->{no_passpolicy};
	    }
            push(@lines, 'password sufficient '.$PAM_IDENTITY_SHARED_LIBRARY);

            push(@$list, BuildPamdChangeSequence($op, $password, @lines));
        }
    }
    else
    {
        if ( $account )
        {
            push(@$list, BuildPamdDeleteChange('account', $PAM_IDENTITY_SHARED_LIBRARY));
            push(@$list, BuildPamdDeleteChange('account', $PAM_PASSPOLICY_SHARED_LIBRARY));
        }

        if ( $auth )
        {
            push(@$list, BuildPamdDeleteChange('auth', $PAM_IDENTITY_SHARED_LIBRARY));
        }

        if ( $session )
        {
            push(@$list, BuildPamdDeleteChange('session', $PAM_IDENTITY_SHARED_LIBRARY));
        }

        if ( $password )
        {
            push(@$list, BuildPamdDeleteChange('password', $PAM_PASSPOLICY_SHARED_LIBRARY));
            push(@$list, BuildPamdDeleteChange('password', $PAM_IDENTITY_SHARED_LIBRARY));
        }
    }

    die join(',', sort keys %$op_modules) if not @$list;

    my $changes =
    {
     file_type => 'pam.d',
     list => $list,
    };

    return $changes;
}


sub GetPamdConfigItem($$$$;$)
    # returns a "config item" that goes into a "config list"
{
    my $enable = shift || 0;
    my $files = shift || die;
    my $op = shift || die;
    my $op_modules = shift || die;
    my $opt = shift || undef;

    die if not (ref($files) eq 'ARRAY');

    return {
            files => $files,
            changes => GetPamdChanges( $enable, $op, $op_modules, $opt ),
           };
}


sub AddFilter($$)
    # returns a "config item" that goes into a "config list"
{
    my $configItem = shift || die;
    my $filter = shift || die;

    die if not (ref($filter) eq 'CODE');
    die if not (ref($configItem) eq 'HASH');

    $configItem->{filter} = $filter;

    return $configItem;
}


sub CreateSkipNoExistsFilter(;$$)
{
    my $base = shift;
    my $subFilter = shift;

    return sub
    {
        my $file = shift || die;
        my $fileType = shift || die;
        my $distroType = shift || die;
        my $distroVersion = shift || die;
        my $testPrefix = shift || '';

        use Centeris;

        $file = $testPrefix.$file;

        my $useBase = $base || QuickBaseName($file);
        if ( QuickBaseName($file) eq $useBase )
        {
            if ( ! -e $file )
            {
                # Skip
                return 0;
            }
            else
            {
                if ( $subFilter )
                {
                    return &$subFilter($file);
                }
                else
                {
                    return 1;
                }
            }
        }
    };
}


sub CreateSkipPatternSubFilter($)
{
    my $skipPattern = shift || die;
    return sub
    {
        my $file = shift || die;

        use Centeris;

        my $contents = ReadFile($file);

        foreach my $line (@$contents)
        {
            if ( $line =~ /$skipPattern/ )
            {
                # Skip
                return 0;
            }
        }

        return 1;
    };
}


sub XdmFilter($$$$;$)
{
    my $file = shift || die;
    my $fileType = shift || die;
    my $distroType = shift || die;
    my $distroVersion = shift || die;
    my $testPrefix = shift || '';

    use Centeris;

    $file = $testPrefix.$file;

    my $contents;

    if ( ( not -e $file ) and
         ( $fileType eq 'pam.d' ) and
         ( QuickBaseName($file) eq 'xdm' ) )
    {
        if ( ($distroType eq 'sles') and ($distroVersion eq '9') )
        {
            $contents = [ split(/\n/, <<DATA) ];
#%PAM-1.0
auth     required       pam_unix2.so    nullok #set_secrpc
account  required       pam_unix2.so
password required       pam_pwcheck.so  nullok
password required       pam_unix2.so    nullok use_first_pass use_authtok
session  required       pam_unix2.so    debug # trace or none
session  required       pam_devperm.so
session  required       pam_resmgr.so
DATA
            WriteFile( $file, $contents );
        }
        elsif ( ($distroType eq 'suse') and ($distroVersion eq '8.1') )
        {
            $contents = [ split(/\n/, <<DATA) ];
#%PAM-1.0
auth     required       pam_unix.so     nullok #set_secrpc
account  required       pam_unix.so
password required       pam_unix.so     #strict=false
session  required       pam_unix.so     debug # trace or none
session  required       pam_devperm.so
DATA
            WriteFile( $file, $contents );
        }
        elsif ( ($distroType eq 'suse') and ($distroVersion eq '8.2') )
        {
            $contents = [ split(/\n/, <<DATA) ];
#%PAM-1.0
auth     required       pam_unix2.so    nullok #set_secrpc
account  required       pam_unix2.so
password required       pam_unix2.so    #strict=false
session  required       pam_unix2.so    debug # trace or none
session  required       pam_devperm.so
session  required       pam_resmgr.so
DATA
            WriteFile( $file, $contents );
        }
        elsif ( ($distroType eq 'suse') and ($distroVersion eq '9.0') )
        {
            $contents = [ split(/\n/, <<DATA) ];
#%PAM-1.0
auth     required       pam_unix2.so    nullok #set_secrpc
account  required       pam_unix2.so
password required       pam_unix2.so    #strict=false
session  required       pam_unix2.so    debug # trace or none
session  required       pam_devperm.so
session  required       pam_resmgr.so
DATA
            WriteFile( $file, $contents );
        }
        elsif ( ($distroType eq 'suse') and ($distroVersion eq '9.1') )
        {
            $contents = [ split(/\n/, <<DATA) ];
#%PAM-1.0
auth     required       pam_unix2.so    nullok #set_secrpc
account  required       pam_unix2.so
password required       pam_pwcheck.so  nullok
password required       pam_unix2.so    nullok use_first_pass use_authtok
session  required       pam_unix2.so    debug # trace or none
session  required       pam_devperm.so
session  required       pam_resmgr.so
DATA
            WriteFile( $file, $contents );
        }
        elsif ( ($distroType eq 'suse') and ($distroVersion eq '9.2') )
        {
            $contents = [ split(/\n/, <<DATA) ];
#%PAM-1.0
auth     required       pam_unix2.so    nullok #set_secrpc
account  required       pam_unix2.so
password required       pam_pwcheck.so  nullok
password required       pam_unix2.so    nullok use_first_pass use_authtok
session  required       pam_unix2.so    debug # trace or none
session  required       pam_limits.so
session  required       pam_devperm.so
session  required       pam_resmgr.so
DATA
            WriteFile( $file, $contents );
        }
    }

    #
    # Continue is 1, skip is 0
    #

    return 1;
}


sub GetFindPamConfLinePattern($$)
{
    my $service = shift || die;
    my $type = shift || die;

    return "^\[ \\t\]*$service\[ \\t\]+$type\[ \\t\]+\[^\\n\]*\$";
}


sub CreateAixAddServiceFilter($$$;$)
{
    my $service = shift || die;
    my $control = shift || die;
    my $module = shift || die;
    my $subFilter = shift;

    return sub
    {
        my $file = shift || die;
        my $fileType = shift || die;
        my $distroType = shift || die;
        my $distroVersion = shift || die;
        my $testPrefix = shift || '';

        my $origFile = $file;
        $file = $testPrefix.$file;

        use Centeris;

        my $lines = ReadFile($file);
        my $text = join('', @$lines);

        my $modified = 0;
        foreach my $type (qw(auth account session password))
        {
            my $pattern = GetFindPamConfLinePattern($service, $type);
            if ( not $text =~ /$pattern/m )
            {
                $pattern = GetFindPamConfLinePattern('OTHER', $type);
                my $add = "$service\t$type\t$control\t$module";
                if ( $text =~ s/$pattern/$add\n$&/m )
                {
                    print "INFO: Adding: $add\n";
                    $modified++;
                }
                else
                {
                    print "INFO: Did not find: OTHER $type\n";
                }
            }
        }

        if ( $modified )
        {
            my @lines = split('\n', $text);
            ReplaceFile( $file, \@lines );
        }

        if ( $subFilter )
        {
            return &$subFilter($origFile, $fileType, $distroType, $distroVersion, $testPrefix);
        }

        #
        # Always continue
        #

        return 1;
    };
}


sub SubOptionsTryFirstPassOnce($$)
{
    my $options = shift;
    my $refSubRef = shift || die;

    die if not ref($refSubRef);
    die if not ( 'REF' eq ref($refSubRef) );
    die if not ( 'CODE' eq ref($$refSubRef) );

    $$refSubRef = undef;

    if (defined $options)
    {
        if ( $options =~ /try_first_pass/ )
        {
            return $options;
        }

        $options =~ s/use_first_pass//;
        $options = 'try_first_pass '.$options;
    }
    else
    {
        $options = 'try_first_pass';
    }

    return $options;
}

sub SubOptionsUseAuthtokPassOnce($$)
{
    my $options = shift;
    my $refSubRef = shift || die;

    die if not ref($refSubRef);
    die if not ( 'REF' eq ref($refSubRef) );
    die if not ( 'CODE' eq ref($$refSubRef) );

    $$refSubRef = undef;

    if (defined $options)
    {
        if ( $options =~ /use_authtok/ )
        {
            return $options;
        }

        $options =~ s/use_authtok//;
        $options = 'use_authtok '.$options;
    }
    else
    {
        $options = 'use_authtok';
    }

    return $options;
}

sub SubOptionsUseFirstPassAll($$)
{
    my $options = shift;
    my $refSubRef = shift || die;

    die if not ref($refSubRef);
    die if not ( 'REF' eq ref($refSubRef) );
    die if not ( 'CODE' eq ref($$refSubRef) );

    if (defined $options)
    {
        if ( $options =~ /use_first_pass/ )
        {
            return $options;
        }

        $options =~ s/try_first_pass//;
        $options = 'use_first_pass '.$options;
    }
    else
    {
        $options = 'use_first_pass';
    }

    return $options;
}


sub SubOptionsNoUseFirstPassAll($$)
{
    my $options = shift;
    my $refSubRef = shift || die;

    die if not ref($refSubRef);
    die if not ( 'REF' eq ref($refSubRef) );
    die if not ( 'CODE' eq ref($$refSubRef) );

    if (defined $options)
    {
        $options =~ s/use_first_pass//;
    }

    return $options;
}


sub GetSuseOldConfigList($;$$$$)
    # returns a "config list" for an old-style SUSE system (w/o common includes)
{
    my $main_module = 'pam_unix2.so';
    my $main_password_module = 'pam_pwcheck.so';

    my $enable = shift || 0;
    my $xdm_password_module = shift || $main_password_module;
    my $alt_main_module = shift || $main_module;
    my $alt_password_module = shift || $main_password_module;
    my $login_main_module = shift || $main_module;

    return [
            # These must be present:
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/login' ],
                               $OP_BEFORE,
                               { auth => $login_main_module, account => $login_main_module, session => $login_main_module, password => $main_password_module } ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/sshd', '/etc/pam.d/passwd', '/etc/pam.d/other' ],
                               $OP_BEFORE,
                               { auth => $main_module, account => $main_module, session => $main_module, password => $main_password_module } ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/su' ],
                               $OP_BEFORE,
                               { auth => $alt_main_module, account => $alt_main_module, session => $alt_main_module, password => $alt_password_module } ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/sudo' ],
                               $OP_BEFORE,
                               { auth => $alt_main_module } ),
            # xdm file gets created if not present:
            AddFilter( GetPamdConfigItem( $enable,
                                          [ '/etc/pam.d/xdm' ],
                                          $OP_BEFORE,
                                          { auth => $alt_main_module, account => $alt_main_module, session => $alt_main_module, password => $xdm_password_module } ),
                       \&XdmFilter ),
            # These need not be present:
            # NOTE: gdm files always use the main module for password -- never pwcheck...
            AddFilter( GetPamdConfigItem( $enable,
                                          [ '/etc/pam.d/gdm', '/etc/pam.d/gdm-autologin' ],
                                          $OP_BEFORE,
                                          { auth => $main_module, account => $main_module, session => $main_module, password => $main_module } ),
                       CreateSkipNoExistsFilter() ),
            AddFilter( GetPamdConfigItem( $enable,
                                          [ '/etc/pam.d/xlock', '/etc/pam.d/xscreensaver' ],
                                          $OP_BEFORE,
                                          { auth => $alt_main_module } ),
                       CreateSkipNoExistsFilter() ),
           ];
}

sub GetDebianConfigList($)
# returns a "config list" for an Ubuntu (6.06 or later) or Debian (3.1 or later) system
{
    my $enable = shift || 0;

    return [
	    {
             files => [ '/etc/pam.d/common-password' ],
             changes =>
             {
              file_type => 'pam.d',
              list =>
              [
               BuildPamdChange($OP_BEFORE, 'pam_unix.so', 'password required'.' pam_cracklib.so retry=3 minlen=6 difok=3'),
              ],
             },
            },
	    {
             files => [ '/etc/pam.d/common-password' ],
             changes =>
             {
              file_type => 'pam.d',
              list =>
              [
               BuildPamdChange($OP_AFTER, 'pam_cracklib.so', 'password required'.' pam_unix.so use_authtok nullok md5'),
              ],
             },
            },
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/common-account' ],
                               $OP_BEFORE,
                               { account => 'pam_unix.so' } ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/common-auth' ],
                               $OP_BEFORE,
                               { auth => 'pam_unix.so' } ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/common-session' ],
                               $OP_BEFORE,
                               { session => 'pam_unix.so' } ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/common-password' ],
                               $OP_BEFORE,
                               { password => 'pam_cracklib.so' } ),
            {
             files => [ '/etc/pam.d/common-auth' ],
             changes =>
             {
              file_type => 'pam.d',
              list =>
              [
               BuildPamdSubOptionsChange('auth', 'pam_unix.so', \&SubOptionsTryFirstPassOnce ),
              ],
             },
            },
            {
             files => [ '/etc/pam.d/common-password' ],
             changes =>
             {
              file_type => 'pam.d',
              list =>
              [
               BuildPamdSubOptionsChange('password', 'pam_unix.so', \&SubOptionsUseAuthtokPassOnce ),
              ],
             },
            },
           ];
}

sub GetSuseConfigList($)
    # returns a "config list" for a new-style SUSE system
{
    my $enable = shift || 0;

    return [
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/common-account' ],
                               $OP_AFTER,
                               { account => 'pam_unix2.so' } ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/common-auth' ],
                               $OP_AFTER,
                               { auth => 'pam_env.so' } ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/common-session' ],
                               $OP_AFTER,
                               { session => 'pam_unix2.so' } ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/common-password' ],
                               $OP_BEFORE,
                               { password => 'pam_pwcheck.so' } ),
            AddFilter( GetPamdConfigItem( $enable,
                                          [ '/etc/pam.d/gnome-passwd' ],
                                          $OP_BEFORE,
                                          { password => 'pam_pwcheck.so' } ),
                       CreateSkipNoExistsFilter('gnome-passwd', CreateSkipPatternSubFilter('^\s*password\s+include\s+common-password\b') ) ),
           ];
}


sub GetRedhatConfigList($;$)
    # returns a "config list" suitable for RH-based/lineage systems
{
    my $enable = shift || 0;
    my $no_passpolicy = shift || 0;

    return [
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/system-auth' ],
                               $OP_AFTER,
                               { account => 'pam_unix.so',
                                 session => 'pam_limits.so' } ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/system-auth' ],
                               $OP_BEFORE,
                               { auth => 'pam_unix.so' } ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/system-auth' ],
                               $OP_BEFORE,
                               { password => 'pam_cracklib.so' },
                               $no_passpolicy ? { no_passpolicy => 1 } : undef ),
            {
             files => [ '/etc/pam.d/system-auth' ],
             changes =>
             {
              file_type => 'pam.d',
              list =>
              [
               BuildPamdSubOptionsChange('auth', 'pam_unix.so', \&SubOptionsTryFirstPassOnce ),
              ],
             },
            },
           ];
}


sub GetDarwinConfigList($;$)
    # returns a "config list" for the Mac OS X Darwin system
{
    my $enable = shift || 0;
    my $no_passpolicy = shift || 0;

    return [
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/chkpasswd' ],
                               $OP_BEFORE,
                               { auth    => 'pam_unix.so', account => 'pam_unix.so' },
                               $no_passpolicy ? { no_passpolicy => 1 } : undef ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/cups' ],
                               $OP_BEFORE,
                               { auth => 'pam_securityserver.so', password => 'pam_deny.so', account => 'pam_permit.so', session => 'pam_permit.so' },
                               $no_passpolicy ? { no_passpolicy => 1 } : undef ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/ftpd' ],
                               $OP_BEFORE,
                               { auth => 'pam_securityserver.so', account => 'pam_permit.so', password => 'pam_deny.so', session => 'pam_permit.so' },
                               $no_passpolicy ? { no_passpolicy => 1 } : undef ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/login' ],
                               $OP_BEFORE,
                               { auth => 'pam_securityserver.so', account => 'pam_permit.so', password => 'pam_deny.so', session => 'pam_uwtmp.so' },
                               $no_passpolicy ? { no_passpolicy => 1 } : undef ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/passwd' ],
                               $OP_BEFORE,
                               { auth => 'pam_securityserver.so', account => 'pam_unix.so', password => 'pam_netinfo.so' },
                               $no_passpolicy ? { no_passpolicy => 1 } : undef ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/sshd' ],
                               $OP_BEFORE,
                               { auth => 'pam_securityserver.so', password => 'pam_deny.so', account => 'pam_permit.so', session => 'pam_afpmount.so' },
                               $no_passpolicy ? { no_passpolicy => 1 } : undef ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/su' ],
                               $OP_BEFORE,
                               { auth => 'pam_securityserver.so', account => 'pam_permit.so', password => 'pam_netinfo.so', session => 'pam_permit.so' },
                               $no_passpolicy ? { no_passpolicy => 1 } : undef ),
            GetPamdConfigItem( $enable,
                               [ '/etc/pam.d/sudo' ],
                               $OP_BEFORE,
                               { auth => 'pam_securityserver.so', account => 'pam_permit.so', password => 'pam_deny.so', session => 'pam_permit.so' },
                               $no_passpolicy ? { no_passpolicy => 1 } : undef ),
           ];
}


sub GetSolarisConfigList($)
{
    my $enable = shift || 0;

    my $module = $PAM_IDENTITY_SHARED_LIBRARY.'.1';
    my $module_pass = $PAM_PASSPOLICY_SHARED_LIBRARY.'.1';

    if ( $enable )
    {
        return [
                {
                 files => [ '/etc/pam.conf' ],
                 changes =>
                 {
                  file_type => 'pam.conf',
                  list =>
                  [
                   # auth
                   {
                    op => $OP_AFTER,
                    service => 'login',
                    type => 'auth',
                    control => 'sufficient',
                    module => $module,
                    options => 'use_first_pass',
                    op_module => 'pam_authtok_get.so.1',
                   },
                   {
                    op => $OP_AFTER,
                    service => 'other',
                    type => 'auth',
                    control => 'sufficient',
                    module => $module,
                    options => 'use_first_pass',
                    op_module => 'pam_authtok_get.so.1',
                   },
                   # account
                   {
                    op => $OP_BEFORE,
                    service => 'other',
                    type => 'account',
                    control => 'sufficient',
                    module => $module,
                    options => '',
                    op_module => 'pam_unix_account.so.1',
                   },
                   # session
                   {
                    op => $OP_AFTER,
                    service => 'other',
                    type => 'session',
                    control => 'optional',
                    module => $module,
                    options => '',
                    op_module => 'pam_unix_session.so.1',
                   },
                   # password
                   {
                    op => $OP_AFTER,
                    service => 'other',
                    type => 'password',
                    control => 'sufficient',
                    module => $module,
                    options => '',
                    op_module => 'pam_authtok_store.so.1',
                   },
#
# Disabling registration of centeris-passpolicy.so for 3.1 release of Identity for Solaris
#
#                   {
#                    op => $OP_AFTER,
#                    service => 'other',
#                    type => 'password',
#                    control => 'required',
#                    module => $module_pass,
#                    options => '',
#                    op_module => 'pam_authtok_store.so.1',
#                   },
                  ],
                 },
                },
               ];
    }
    else
    {
        return [
                {
                 files => [ '/etc/pam.conf' ],
                 changes =>
                 {
                  file_type => 'pam.conf',
                  list =>
                  [
                   # auth
                   {
                    op => $OP_DELETE,
                    service => 'login',
                    type => 'auth',
                    module => $module,
                   },
                   {
                    op => $OP_DELETE,
                    service => 'other',
                    type => 'auth',
                    module => $module,
                   },
                   # account
                   {
                    op => $OP_DELETE,
                    service => 'other',
                    type => 'account',
                    module => $module,
                   },
                   # session
                   {
                    op => $OP_DELETE,
                    service => 'other',
                    type => 'session',
                    module => $module,
                   },
                   # password
                   {
                    op => $OP_DELETE,
                    service => 'other',
                    type => 'password',
                    module => $module,
                   },
#
# Disabling registration of centeris-passpolicy.so for 3.1 release of Identity for Solaris
#
#                   {
#                    op => $OP_DELETE,
#                    service => 'other',
#                    type => 'password',
#                    module => $module_pass,
#                   },
                  ],
                 },
                },
               ];
    }
}

sub GetOldHpuxConfigList($)
{
    my $enable = shift || 0;

    my $module = $PAM_IDENTITY_SHARED_LIBRARY;
    my $module_pass = $PAM_PASSPOLICY_SHARED_LIBRARY;

    my @lines = `uname -m`;
    my $arch = $lines[0];
    chomp($arch);

    if ( $enable )
    {
        my $dirPrefix = '/usr/lib/security/';

        $module = $dirPrefix.$module;
        $module_pass = $dirPrefix.$module;
    }

    my $list = [];

    my $cfg =
    {
     auth => [ qw(login su dtlogin dtaction ftp sshd sudo OTHER) ],
     account => [ qw(login su dtlogin dtaction ftp sshd sudo OTHER) ],
     session => [ qw(login dtlogin dtaction sshd sudo OTHER) ],
     password => [ qw(login passwd dtlogin dtaction sshd sudo OTHER) ],
    };

    foreach my $type (keys %$cfg)
    {
        my $control = 'sufficient';
        foreach my $service (@{$cfg->{$type}})
        {
            if ( $enable )
            {
                push(@$list,
                     {
                      op => $OP_BEFORE,
                      service => $service,
                      type => $type,
                      control => $control,
                      module => $module,
                      options => '',
                      op_module => 'libpam_unix.1',
                     });
            }
            else
            {
                push(@$list,
                     {
                      op => $OP_DELETE,
                      service => $service,
                      type => $type,
                      module => $module,
                     });
            }
        }
    }

    if ( $enable )
    {
        push(@$list, BuildPamdSubOptionsChange('auth', '/usr/lib/security/libpam_unix.1', \&SubOptionsUseFirstPassAll));
    }
    else
    {
        push(@$list, BuildPamdSubOptionsChange('auth', '/usr/lib/security/libpam_unix.1', \&SubOptionsNoUseFirstPassAll));
    }

    return [
            {
             files => [ '/etc/pam.conf' ],
             filter => CreateAixAddServiceFilter('sshd', 'required', '/usr/lib/security/libpam_unix.1',
                                                 CreateAixAddServiceFilter('sudo', 'required', '/usr/lib/security/libpam_unix.1')),
             changes =>
             {
              file_type => 'pam.conf',
              list => $list,
             },
            },
           ];
}

sub GetHpuxConfigList($)
{
    my $enable = shift || 0;

    my $module = $PAM_IDENTITY_SHARED_LIBRARY;
    my $module_pass = $PAM_PASSPOLICY_SHARED_LIBRARY;

    my @lines = `uname -m`;
    my $arch = $lines[0];
    chomp($arch);

    my $list = [];

    my $cfg =
    {
     auth => [ qw(login su dtlogin dtaction ftp sshd sudo OTHER) ],
     account => [ qw(login su dtlogin dtaction ftp sshd sudo OTHER) ],
     session => [ qw(login dtlogin sshd sudo OTHER) ],
     password => [ qw(login passwd dtlogin sshd sudo OTHER) ],
    };

    foreach my $type (keys %$cfg)
    {
        my $control = 'sufficient';
        foreach my $service (@{$cfg->{$type}})
        {
            if ( $enable )
            {
                push(@$list,
                     {
                      op => $OP_BEFORE,
                      service => $service,
                      type => $type,
                      control => $control,
                      module => $module,
                      options => '',
                      op_module => 'libpam_unix.so.1',
                     });
                push(@$list,
                     {
                      op => $OP_DELETE,
                      service => $service,
                      type => $type,
                      module => 'libpam_hpsec.so.1',
                     });
            }
            else
            {
                push(@$list,
                     {
                      op => $OP_DELETE,
                      service => $service,
                      type => $type,
                      module => $module,
                     });
                push(@$list,
                     {
                      op => $OP_BEFORE,
                      service => $service,
                      type => $type,
                      control => 'required',
                      module => 'libpam_hpsec.so.1',
                      options => '',
                      op_module => 'libpam_unix.so.1',
                     });
            }
        }
    }

    if ( $enable )
    {
        push(@$list, BuildPamdSubOptionsChange('auth', 'libpam_unix.so.1', \&SubOptionsUseFirstPassAll));
    }
    else
    {
        push(@$list, BuildPamdSubOptionsChange('auth', 'libpam_unix.so.1', \&SubOptionsNoUseFirstPassAll));
    }

    return [
            {
             files => [ '/etc/pam.conf' ],
             filter => CreateAixAddServiceFilter('sshd', 'required', 'libpam_unix.so.1',
                                                 CreateAixAddServiceFilter('sudo', 'required', 'libpam_unix.so.1')),
             changes =>
             {
              file_type => 'pam.conf',
              list => $list,
             },
            },
           ];
}

sub GetAixConfigList($)
{
    my $enable = shift || 0;

    my $module = $PAM_IDENTITY_SHARED_LIBRARY;
    my $module_pass = $PAM_PASSPOLICY_SHARED_LIBRARY;

    if ( $enable )
    {
        my $dirPrefix = '/usr/lib/security/';

        $module = $dirPrefix.$module;
        $module_pass = $dirPrefix.$module;
    }

    my $list = [];

    my $cfg =
    {
     auth => [ qw(ftp imap login rexec rlogin snapp su telnet sshd sudo) ],
     account => [ qw(ftp login rexec rlogin rsh su telnet sshd sudo) ],
     session => [ qw(ftp imap login rexec rlogin rsh snapp su telnet sshd sudo) ],
     password => [ qw(login passwd rlogin su telnet sshd sudo) ],
    };

    foreach my $type (keys %$cfg)
    {
        my $control = ($type eq 'session') ? 'optional' : 'sufficient';
        foreach my $service (@{$cfg->{$type}})
        {
            if ( $enable )
            {
                push(@$list,
                     {
                      op => $OP_BEFORE,
                      service => $service,
                      type => $type,
                      control => $control,
                      module => $module,
                      options => '',
                      op_module => 'pam_aix',
                     });
            }
            else
            {
                push(@$list,
                     {
                      op => $OP_DELETE,
                      service => $service,
                      type => $type,
                      module => $module,
                     });
            }
        }
    }

    if ( $enable )
    {
        push(@$list,
             {
              op => $OP_BEFORE,
              service => 'rsh',
              type => 'auth',
              control => 'sufficient',
              module => $module,
              options => '',
              op_module => 'pam_rhosts_auth',
             });
        push(@$list, BuildPamdSubOptionsChange('auth', '/usr/lib/security/pam_aix', \&SubOptionsUseFirstPassAll));
    }
    else
    {
        push(@$list,
             {
              op => $OP_DELETE,
              service => 'rsh',
              type => 'auth',
              module => $module,
             });
        push(@$list, BuildPamdSubOptionsChange('auth', '/usr/lib/security/pam_aix', \&SubOptionsNoUseFirstPassAll));
    }

    return [
            {
             files => [ '/etc/pam.conf' ],
             filter => CreateAixAddServiceFilter('sshd', 'required', '/usr/lib/security/pam_aix',
                                                 CreateAixAddServiceFilter('sudo', 'required', '/usr/lib/security/pam_aix')),
             changes =>
             {
              file_type => 'pam.conf',
              list => $list,
             },
            },
           ];
}

sub UpdateLwidentityPamConfig()
{
    my $distro = Centeris::GetDistroType();
    my $version = Centeris::GetDistroVersion();

    if ($distro eq 'rhel' && ($version >= 2 && $version <= 4))
    {
        my ($file, $contents, $modified, $length);

        $file = "/etc/security/pam_lwidentity.conf";

        $contents = Centeris::ReadFile($file);

        @$modified = @$contents;

        $length = $#{@$contents};

        foreach my $index (0..$length)
        {
            $modified->[$index] =~ s/^\s*try_first_pass\s*=/\# try_first_pass =/;
        }

        if (Centeris::CompareLines($contents, $modified))
        {
            Centeris::ReplaceFile($file, $modified);
        }
    }
}


sub usage()
{
    use Centeris;

    $0 = QuickBaseName($0);
    return <<DATA;
usage: $0 [options] --enable
   or: $0 [options] --disable

  options (case-sensitive):

    --testprefix <prefix>   Prefix for test directory
DATA
}

sub main()
{
    use Centeris;
    use Getopt::Long;

    Getopt::Long::Configure('no_ignore_case', 'no_auto_abbrev') || die;

    my $opt = {};
    my $ok = GetOptions($opt,
			            'help|h|?',
                        'testprefix=s',
                        'test',
                        'debug',
                        'enable',
                        'disable',
                        'module=s',
                       );

    if ( not $ok or
         $opt->{help} or
         not ( $opt->{enable} xor $opt->{disable} ) or
         ( $#ARGV != -1 ) )
    {
	die usage();
    }

    my $prefix = $opt->{testprefix} || '';
    CheckTestPrefix($prefix);

    if ( defined($opt->{module}) ) {
	$PAM_IDENTITY_SHARED_LIBRARY = 'pam_'.$opt->{module}.'.so';
    }

    UpdateLwidentityPamConfig();

    my $configList = CreateConfigList( $opt->{enable} ? 1 : 0 );
    return ModifyPamFiles( $configList, $prefix, CreateGlobalOpts( $opt ) );
}
