#Likewise Open on VMware Photon

##Source code

git clone ssh://git@github.com/vmware/likewise-open.git

We will be using the "lcifs" branch.

##How to build?

Please checkout the code and build on Photon (Full)

>cd [workspace-root]/release && ./photonbuild64.sh

A successful build will create the following RPMs under [workspace-root]/release/package/rpm/likewise-open

 1. likewise-open-6.2.0-0.x86_64.rpm
 2. likewise-open-devel-6.2.0-x86_64.rpm

##How to use the Likewise Open stack ?

The Likewise Open stack provides integration for non-windows systems to Microsoft Active Directory.

The integration includes authentication for identities defined in Microsoft Active Directory as well as File Services support as implemented through Common Internet File Systems (CIFS).

The stack makes it possible to introduce additional authentication providers to help integrate with identity stacks other than Microsoft Active Directory.

At the current time, Photon uses only the authentication feature.

 - Photon is an Operating System based on the Linux 3.x Kernel.
 - User/Group lookup through GLIBC APIs is controlled through /etc/nsswitch.conf
 - Authentication is based on Pluggable Authentication Modules (PAM)

###How to integrate the Likewise stack through nsswitch?

Likewise Open provides a plugin for nsswitch that can be configured using the following command.

>*/opt/likewise/bin/domainjoin-cli configure --enable nsswitch*

###How to integrate the Likewise stack through PAM?

Likewise Open provides a plugin for PAM that can be configured using the following command

>*/opt/likewise/bin/domainjoin-cli configure --enable pam*

###How to Join the local system to Active Directory?

>"/opt/likewise/bin/domainjoin-cli join domain-name Administrator"

Notes : 

 1. This command does not automatically enable the nsswitch and PAM integration at this time.
 2. Before running the join command, ensure that the DNS settings are configured correctly to be able to lookup the active directory domain.
 3. Run "/opt/likewise/bin/lw-get-dc-name [domain-name]" to make sure the Likewise system can locate the domain controllers for the given domain-name.

###How to make the local system disengage with the Active Directory integration?

>"/opt/likewise/bin/domainjoin-cli leave Administrator"

Note: This command does not automatically disable the nsswitch and PAM integration at this time.

###How do I know that my system has been integrated with Active Directory?

 - If nsswitch has been configured to recognize the Likewise stack, running "id fqdn-of-user-in-active-directory" should locate the user record.
 - If nsswitch has not been configured, one can use "/opt/likewise/bin/lw-find-user-by-name id-fqdn-of-user-in-active-directory". For instance, "/opt/likewise/bin/lw-find-user-by-name joe@mycompany.com"
 - On Photon, ssh has been configured to integrate with PAM. If the Likewise stack has been configured with PAM, it should be possible to login to the localhost using SSH as a user in Active Directory.
