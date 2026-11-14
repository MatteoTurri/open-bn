#!/bin/sh
# Regenerates the generated source files of SQLite 2.7.2 in ref/sqlite-2.7.2/gen/
# (sqlite.h, opcodes.[ch], parse.[ch]), following the recipes of main.mk.
# lemon needs a patch to build with modern toolchains (ErrorMsg K&R varargs).
set -e
TOP=ref/sqlite-2.7.2
GEN=$TOP/gen
mkdir -p "$GEN"
cd "$GEN"

sed -e "s/--VERS--/$(cat ../VERSION)/" -e 's/--ENCODING--/iso8859/' ../src/sqlite.h.in > sqlite.h

echo '/* Automatically generated file.  Do not edit */' > opcodes.h
grep '^case OP_' ../src/vdbe.c | sed -e 's/://' | \
    awk '{printf "#define %-30s %3d\n", $2, ++cnt}' >> opcodes.h

echo '/* Automatically generated file.  Do not edit */' > opcodes.c
echo 'char *sqliteOpcodeNames[] = { "???", ' >> opcodes.c
grep '^case OP_' ../src/vdbe.c | sed -e 's/^.*OP_/  "/' -e 's/:.*$/", /' >> opcodes.c
echo '};' >> opcodes.c

# lemon: fix ErrorMsg (K&R varargs) and its prototype for modern compilers
sed -e 's/#include <varargs.h>/#include <stdarg.h>/' ../tool/lemon.c > lemon.c
python3 - <<'PY'
s = open('lemon.c').read()
s = s.replace('void ErrorMsg( /* char *, int, char *, ... */ );',
              'void ErrorMsg(char *, int, char *, ...);')
s = s.replace('''void ErrorMsg(va_alist)
va_dcl
{
  char *filename;
  int lineno;
  char *format;''', 'void ErrorMsg(char *filename, int lineno, char *format, ...)\n{')
s = s.replace('''  va_start(ap);
  filename = va_arg(ap,char*);
  lineno = va_arg(ap,int);
  format = va_arg(ap,char*);''', '  va_start(ap, format);')
open('lemon.c', 'w').write(s)
PY
cc -std=gnu89 -Wno-implicit-function-declaration -Wno-implicit-int -Wno-return-type -o lemon lemon.c
cp ../src/parse.y . && cp ../tool/lempar.c .
./lemon parse.y
echo "OK: generated in $GEN (sqlite.h, opcodes.[ch], parse.[ch])"
