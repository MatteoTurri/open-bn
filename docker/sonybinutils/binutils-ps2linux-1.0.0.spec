Summary: GNU Binary Utility Development Utilities
Name: binutils
%define version 2.9EE
Version: %{version}
Release: 3a
Copyright: GPL
Group: Development/Tools
Source: binutils-%{version}.tar.gz
Patch1: binutils-ps2linux-1.0.0.patch
Buildroot: /var/tmp/binutils-root

%description
Binutils is a collection of utilities necessary for compiling programs. It
includes the assembler and linker, as well as a number of other
miscellaneous programs for dealing with executable formats.

%prep
%setup -q

%patch1 -p1

%build
mkdir -p build-tmp
cd build-tmp 
../configure --target=mipsEEel-linux --host=mipsEEel-linux --build=mipsEEel-linux -enable-shared
#./configure --prefix=/usr --enable-shared
make tooldir=/usr all info

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr
cd build-tmp
make prefix=$RPM_BUILD_ROOT/usr tooldir=$RPM_BUILD_ROOT/usr/ install install-info
cd ..
strip $RPM_BUILD_ROOT/usr/bin/*
gzip -q9f $RPM_BUILD_ROOT/usr/info/*.info*

# remove /usr/info/dir.info.gz
rm -f $RPM_BUILD_ROOT/usr/info/dir.info.gz

# remove conflit binary
rm -f $RPM_BUILD_ROOT/usr/bin/c++filt

## Get rid of that arch-specific directory
#rm -rf $RPM_BUILD_ROOT/usr/lib/bin

#install -m 644 libiberty/libiberty.a $RPM_BUILD_ROOT/usr/lib
install -m 644 include/libiberty.h $RPM_BUILD_ROOT/usr/include

chmod +x $RPM_BUILD_ROOT/usr/lib/lib*.so*

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig
/sbin/install-info --info-dir=/usr/info /usr/info/as.info.gz
/sbin/install-info --info-dir=/usr/info /usr/info/bfd.info.gz
/sbin/install-info --info-dir=/usr/info /usr/info/binutils.info.gz
/sbin/install-info --info-dir=/usr/info /usr/info/gasp.info.gz
/sbin/install-info --info-dir=/usr/info /usr/info/gprof.info.gz
/sbin/install-info --info-dir=/usr/info /usr/info/ld.info.gz
/sbin/install-info --info-dir=/usr/info /usr/info/standards.info.gz

%preun
if [ $1 = 0 ] ;then
  /sbin/install-info --delete --info-dir=/usr/info /usr/info/as.info.gz
  /sbin/install-info --delete --info-dir=/usr/info /usr/info/bfd.info.gz
  /sbin/install-info --delete --info-dir=/usr/info /usr/info/binutils.info.gz
  /sbin/install-info --delete --info-dir=/usr/info /usr/info/gasp.info.gz
  /sbin/install-info --delete --info-dir=/usr/info /usr/info/gprof.info.gz
  /sbin/install-info --delete --info-dir=/usr/info /usr/info/ld.info.gz
  /sbin/install-info --delete --info-dir=/usr/info /usr/info/standards.info.gz
fi

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc README
/usr/bin/*
/usr/man/man1/*
/usr/include/*
/usr/lib/*
/usr/info/*info*
