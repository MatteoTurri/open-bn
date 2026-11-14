Summary: GNU C Compiler
Name: gcc
%define version 2.95.2
Version: %{version}
Release: 3a
Copyright: GPL
Group: Development/Languages

Source: gcc-%{version}.tar.gz
Patch0: gcc-2.95.2-frankengcc-patches.patch
Patch1: gcc-2.95.2-single-float-const.patch
Patch2: gcc-ps2linux-1.0.0.patch

Buildroot: /var/tmp/gcc-%{version}-root
Requires: binutils >= 2.9EE
Obsoletes: gcc
Provides: gcc
Requires: cpp = %{version}
Prereq: /sbin/install-info

%description
GCC is a free software project that intends to further the development
of GNU compilers using an open development environment.  The gcc package
contains the gcc compiler, a compiler aimed at integrating all the
optimizations and features necessary for a high-performance and stable
development environment.

Install gcc if you'd like to use an experimental GNU compiler.

%package c++
Summary: C++ support for the gcc compiler.
Group: Development/Languages
Requires: gcc = %{version}
Obsoletes: gcc-c++
Obsoletes: libstdc++-devel
Obsoletes: libg++-devel
Requires: libstdc++ = %{version}
Requires: cpp = %{version}

%description c++
This package adds C++ support to the GNU C compiler. It includes support for
most of the current C++ specification, including templates and exception
handling. It does include the static standard C++ library and C++ header
files; the library for dynamically linking programs is available separately.

%package objc
Summary: Objective C support for the gcc compiler.
Group: Development/Languages
Requires: gcc = %{version}
Obsoletes: gcc-objc
Requires: cpp = %{version}

%description objc
Gcc-objc provides Objective C support for the GNU C compiler (gcc).
Mainly used on systems running NeXTSTEP, Objective C is an 
object-oriented derivative of the C language.

Install gcc-objc if you are going to do Objective C development and you
would like to use the gcc compiler.  You will also need to install the gcc
package.

%package g77
Summary: Fortran 77 support for the gcc compiler.
Group: Development/Languages
Requires: gcc = %{version}
Prereq: /sbin/install-info

%description g77
The gcc-g77 package provides support for compiling Fortran 77
programs with the GNU gcc compiler.

You should install gcc-g77 if you are going to do Fortran development and
you would like to use the gcc compiler.  You will also need to install the
gcc package.

%package -n libstdc++
Summary: GNU c++ library
Group: System Environment/Libraries
Obsoletes: libg++
Version: %{version}

%description -n libstdc++
GCC is a free software project that intends to further the development
of GNU compilers using an open development environment.  The gcc
package contains the gcc compiler, a compiler aimed at integrating all
the optimizations and features necessary for a high-performance and stable
development environment. GCC includes the shared libraries necessary for
running C++ appplications, along with additional GNU tools.

Install gcc if you'd like to use an experimental GNU compiler.

%package -n cpp
Summary: The C Preprocessor.
Group: Development/Languages
Prereq: /sbin/install-info

%description -n cpp
The C preprocessor is a 'macro processor' which is used automatically
by the C compiler to transform your program before actual
compilation. It is called a macro processor because it allows
you to define 'macros,' which are abbreviations for longer
constructs.

The C preprocessor provides four separate facilities that you can use as
you see fit:

* Inclusion of header files. These are files of declarations that can be
  substituted into your program.
* Macro expansion. You can define 'macros,' which are abbreviations for 
  arbitrary fragments of C code, and then the C preprocessor will replace
  the macros with their definitions throughout the program.
* Conditional compilation. Using special preprocessing directives,
  you can include or exclude parts of the program according to various
  conditions.
* Line control. If you use a program to combine or rearrange source files
  into an intermediate file which is then compiled, you can use line
  control to inform the compiler about where each source line originated.

You should install this package if you are a programmer who is searching for
such a macro processor.

%prep
%setup -q -n %{name}-%{version}
%patch0 -p1
%patch1 -p1
%patch2 -p1
# --- EE ---
# mkdir compat
# tar xzf %{SOURCE1} -C compat
# cd gcc ; autoconf ; cd ..

%build
rm -rf obj-$RPM_ARCH-linux
mkdir obj-$RPM_ARCH-linux
cd obj-$RPM_ARCH-linux
# --- EE ---
# ../configure  --prefix=/usr \
# 	--enable-shared --enable-threads \
#	$RPM_ARCH-redhat-linux
../configure  --prefix=/usr \
	--enable-shared \
	--target=mipsEEel-linux \
	--host=mipsEEel-linux \
	--build=mipsEEel-linux \
	--disable-nls

PATH=$PATH:/sbin:/usr/sbin
# gperf is most likely broken on alpha and sparc 
#touch  ../gcc/c-gperf.h
make  MAKEINFO="makeinfo --no-split"  bootstrap

%install
rm -rf $RPM_BUILD_ROOT
cd obj-$RPM_ARCH-linux
PATH=$PATH:/sbin:/usr/sbin
make install \
	prefix=$RPM_BUILD_ROOT/usr \
	mandir=$RPM_BUILD_ROOT/usr/man \
	MAKEINFO="makeinfo --no-split" 
FULLVER=$($RPM_BUILD_ROOT/usr/bin/gcc --version)
FULLPATH=$(dirname $RPM_BUILD_ROOT/usr/lib/gcc-lib/mipsEEel-linux/$FULLVER/cc1)

file $RPM_BUILD_ROOT/usr/bin/* | grep ELF | cut -d':' -f1 | xargs strip || :
strip $FULLPATH/{cc1,cc1obj,cc1plus,cpp,f771,collect2}

# fix some things
ln -f $RPM_BUILD_ROOT/usr/bin/mipsEEel-linux-gcc $RPM_BUILD_ROOT/usr/bin/gcc
ln -sf gcc $RPM_BUILD_ROOT/usr/bin/cc
# mv $RPM_BUILD_ROOT/usr/include/g++-3 $RPM_BUILD_ROOT/usr/include/g++

rm -f $RPM_BUILD_ROOT/usr/info/dir
gzip -n -9f $RPM_BUILD_ROOT/usr/info/*.info* 

ln -sf g77.1 $RPM_BUILD_ROOT/usr/man/man1/f77.1
ln -sf g77 $RPM_BUILD_ROOT/usr/bin/f77

mkdir -p $RPM_BUILD_ROOT/lib
ln -sf ../usr/bin/cpp $RPM_BUILD_ROOT/lib/cpp

ln -sf cccp.1 $RPM_BUILD_ROOT/usr/man/man1/cpp.1

#install the compatibility libstdc++ library
# --- EE ---
# [ -d ../compat/$RPM_ARCH ] && install -m 755 ../compat/$RPM_ARCH/* $RPM_BUILD_ROOT/usr/lib/

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/install-info --info-dir=/usr/info /usr/info/gcc.info.gz

%post -n cpp
/sbin/install-info --info-dir=/usr/info /usr/info/cpp.info.gz

%post g77
/sbin/install-info --info-dir=/usr/info /usr/info/g77.info.gz

%preun
if [ $1 = 0 ]; then
   /sbin/install-info --delete --info-dir=/usr/info /usr/info/gcc.info.gz
fi

%preun -n cpp
if [ $1 = 0 ]; then
   /sbin/install-info --delete --info-dir=/usr/info /usr/info/cpp.info.gz
fi

%preun g77
if [ $1 = 0 ]; then
   /sbin/install-info --delete --info-dir=/usr/info /usr/info/g77.info.gz
fi

%post -n libstdc++ -p /sbin/ldconfig

%postun -n libstdc++ -p /sbin/ldconfig

%files
%doc README* COPYING COPYING.LIB
%dir /usr/lib/gcc-lib
%dir /usr/lib/gcc-lib/mipsEEel-linux
%dir /usr/lib/gcc-lib/mipsEEel-linux/*
%dir /usr/lib/gcc-lib/mipsEEel-linux/*/include
/usr/bin/mipsEEel-linux-gcc
# /usr/bin/egcs
/usr/bin/gcov
/usr/bin/protoize
/usr/bin/unprotoize
/usr/bin/gcc
/usr/bin/cc
/usr/info/gcc*

# /usr/man/man1/egcs.1
/usr/man/man1/gcc.1

/usr/lib/gcc-lib/mipsEEel-linux/*/SYSCALLS.c.X
/usr/lib/gcc-lib/mipsEEel-linux/*/cc1
/usr/lib/gcc-lib/mipsEEel-linux/*/collect2
/usr/lib/gcc-lib/mipsEEel-linux/*/libgcc.a
# /usr/lib/gcc-lib/mipsEEel-linux/*/libgcc.map
/usr/lib/gcc-lib/mipsEEel-linux/*/specs
/usr/lib/gcc-lib/mipsEEel-linux/*/soft-float/libgcc.a

/usr/lib/gcc-lib/mipsEEel-linux/*/*.o

#%ifnarch alpha
#/usr/lib/gcc-lib/mipsEEel-linux/*.o
#%endif



# /usr/mipsEEel-linux

/usr/lib/gcc-lib/mipsEEel-linux/*/include/README
/usr/lib/gcc-lib/mipsEEel-linux/*/include/float.h
/usr/lib/gcc-lib/mipsEEel-linux/*/include/iso646.h
/usr/lib/gcc-lib/mipsEEel-linux/*/include/limits.h
/usr/lib/gcc-lib/mipsEEel-linux/*/include/proto.h
/usr/lib/gcc-lib/mipsEEel-linux/*/include/stdarg.h
/usr/lib/gcc-lib/mipsEEel-linux/*/include/stdbool.h
/usr/lib/gcc-lib/mipsEEel-linux/*/include/stddef.h
/usr/lib/gcc-lib/mipsEEel-linux/*/include/syslimits.h
/usr/lib/gcc-lib/mipsEEel-linux/*/include/va-*.h
/usr/lib/gcc-lib/mipsEEel-linux/*/include/varargs.h
%ifarch ppc
/usr/lib/gcc-lib/mipsEEel-linux/include/*-asm.h
%endif

%files -n cpp
/lib/cpp
/usr/bin/cpp
/usr/man/man1/cpp.1
/usr/man/man1/cccp.1
/usr/info/cpp.info*.gz
/usr/lib/gcc-lib/mipsEEel-linux/*/cpp

%files c++
/usr/man/man1/g++.1
/usr/bin/g++
/usr/bin/c++
/usr/bin/c++filt
/usr/lib/gcc-lib/mipsEEel-linux/*/cc1plus
/usr/lib/gcc-lib/mipsEEel-linux/*/libstdc*
/usr/lib/gcc-lib/mipsEEel-linux/*/include/exception
/usr/lib/gcc-lib/mipsEEel-linux/*/include/new
/usr/lib/gcc-lib/mipsEEel-linux/*/include/typeinfo
/usr/lib/gcc-lib/mipsEEel-linux/*/include/new.h
/usr/include/g+*
/usr/lib/libstdc++-*-libc*-*.a
/usr/lib/libstdc++-*-libc*-*.so
/usr/lib/libstdc++-libc*.a.*
%ifarch ppc
/usr/lib/nof
%endif
/usr/lib/soft-float/libstdc++-*-libc6*-*.so
/usr/lib/soft-float/libstdc++-*-libc6*-*.a
/usr/lib/soft-float/libstdc++-libc6*.a.*
/usr/lib/soft-float/libstdc++-libc6*.so.*
/usr/lib/gcc-lib/mipsEEel-linux/*/soft-float/libstdc++.a
/usr/lib/gcc-lib/mipsEEel-linux/*/soft-float/libstdc++.so


%files objc
/usr/lib/gcc-lib/mipsEEel-linux/*/cc1obj
/usr/lib/gcc-lib/mipsEEel-linux/*/libobjc.a
/usr/lib/gcc-lib/mipsEEel-linux/*/include/objc

%files g77
/usr/bin/g77
/usr/bin/f77
/usr/info/g77*
/usr/lib/gcc-lib/mipsEEel-linux/*/f771
/usr/lib/gcc-lib/mipsEEel-linux/*/libg2c.a
/usr/man/man1/g77.1
/usr/man/man1/f77.1
/usr/lib/gcc-lib/mipsEEel-linux/*/include/g2c.h
/usr/lib/gcc-lib/mipsEEel-linux/*/soft-float/libg2c.a

%files -n libstdc++
/usr/lib/libstdc++*
%ifarch i386 sparc alpha
/usr/lib/libstdc++.so.2.8.0
%endif
%ifarch i386 alpha
/usr/lib/libstdc++.so.2.7.2.8
/usr/lib/libg++.so.2.7.2.8
%endif

%changelog
* Thu Mar 29 2001 Hiroyuki Machida <machida@sm.sony.co.jp>
- added changes for PS2 linux 

* Mon May 17 1999 Matt Wilson <msw@redhat.com>
- added patch from Dave Miller

* Sun Mar 21 1999 Cristian Gafton <gafton@redhat.com> 
- auto rebuild in the new build environment (release 12)

* Wed Mar 17 1999 Cristian Gafton <gafton@redhat.com>
- version 1.1.2
- added patch from davem

* Fri Mar 12 1999 Cristian Gafton <gafton@redhat.com>
- version 1.1.2pre3

