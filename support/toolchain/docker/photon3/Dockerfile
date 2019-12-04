FROM photon:3.0-20191101
MAINTAINER "Sriram Nambakam" <snambakam@vmware.com>
ENV container=docker

RUN tdnf update -qy  tdnf  && \
    tdnf erase -qy toybox && \
    tdnf install -y autoconf-2.69-8.ph3.noarch \
                    automake-1.16.1-1.ph3.noarch \
                    binutils-2.32-3.ph3.x86_64 \
                    bison-3.1-1.ph3.x86_64 \
                    e2fsprogs-devel-1.44.3-4.ph3.x86_64 \
                    gawk-4.2.1-1.ph3.x86_64 \
                    gcc-7.3.0-4.ph3.x86_64 \
                    glibc-devel-2.28-4.ph3.x86_64 \
                    krb5-1.17-1.ph3.x86_64 \
                    krb5-devel-1.17-1.ph3.x86_64 \
                    libnsl-1.2.0-1.ph3.x86_64 \
                    libtool-2.4.6-3.ph3.x86_64 \
                    libxml2-2.9.9-1.ph3.x86_64 \
                    libxml2-devel-2.9.9-1.ph3.x86_64 \
                    linux-api-headers-4.19.84-1.ph3.noarch \
                    Linux-PAM-1.3.0-1.ph3.x86_64\
                    Linux-PAM-devel-1.3.0-1.ph3.x86_64 \
                    make-4.2.1-3.ph3.x86_64 \
                    ncurses-6.1-1.ph3.x86_64 \
                    ncurses-devel-6.1-1.ph3.x86_64 \
                    openldap-2.4.48-1.ph3.x86_64 \
                    openldap-devel-2.4.48-1.ph3.x86_64 \
                    openssl-1.0.2t-1.ph3.x86_64 \
                    openssl-devel-1.0.2t-1.ph3.x86_64 \
                    popt-1.16-5.ph3.x86_64 \
                    popt-devel-1.16-5.ph3.x86_64 \
                    procps-ng-3.3.15-1.ph3.x86_64 \
                    rpm-4.14.2-5.ph3.x86_64 \
                    rpm-build-4.14.2-5.ph3.x86_64 \
                    rpm-devel-4.14.2-5.ph3.x86_64 \
                    sed-4.5-1.ph3.x86_64 \
                    shadow-4.6-4.ph3.x86_64 \
                    sqlite-3.27.2-5.ph3.x86_64 \
                    sqlite-devel-3.27.2-5.ph3.x86_64 \
                    tar-1.30-4.ph3.x86_64 \
                    util-linux-devel-2.32-1.ph3.x86_64 \
                    zlib-1.2.11-1.ph3.x86_64 \
                    zlib-devel-1.2.11-1.ph3.x86_64 && \
    echo 'ALL ALL=NOPASSWD: ALL' >>/etc/sudoers

