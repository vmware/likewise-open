Building SrvSvc on Windows:
===================================================

--Use the latest Sapphire tree from SVN
--Open LMC.sln from Sapphire\src\windows\Quartz\LikewiseConsole
--Build and run the entire project


Building SrvSvc on Linux:
===================================================

--Verify that all Sapphire RPMs from the latest successful build (Currently, build 2634), except srvsvc, are installed.
--Use the latest Sapphire source tree from SVN
--Enter directory Sapphire/src/linux/srvsvc
--run export LD_LIBRARY_PATH=/usr/likewise/lib
--run ./autogen.sh
--run --enable-debug
--run make
--run sudo make install



Debugging SrvSvc on Linux:
===================================================

Same as steps for 'Building SrvSvc on Linux', except for configure:

--run ./configure --enable-debug instead of './configure'


Now, you should be able to use gdb to debug the server and client modules.

Please make sure your $HOME/.gdbinit has the following line

handle SIGXCPU SIG33 SIG35 SIGPWR nostop noprint




To run srvsvc server from the command line on linux:
===================================================

--enter /usr/likewise/sbin
--su to root
--run /sbin/service likewise.com-rpcd restart
--run export LD_LIBRARY_PATH=/usr/likewise/lib
--run ./likewise-srvsvcd


To start srvsvc server as a background service on linux:
===================================================
--su to root
--run /sbin/service likewise.com-rpcd restart
--run /sbin/service likewise.com-srvsvcd restart



To run srvsvc client from the command line on linux:
===================================================
--su to root
--run export LD_LIBRARY_PATH=/usr/likewise/lib
--run /usr/likewise/bin/lw-srvsvc-cli help to display help
