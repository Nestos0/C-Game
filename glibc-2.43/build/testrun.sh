#!/bin/bash
builddir=`dirname "$0"`
GCONV_PATH="${builddir}/iconvdata"

usage () {
cat << EOF
Usage: $0 [OPTIONS] <program> [ARGUMENTS...]

  --tool=TOOL  Run with the specified TOOL. It can be strace, rpctrace,
               valgrind or container. The container will run within
               support/test-container.  For strace and valgrind,
               additional arguments can be passed after the tool name.
EOF

  exit 1
}

toolname=default
while test $# -gt 0 ; do
  case "$1" in
    --tool=*)
      toolname="${1:7}"
      shift
      ;;
    --*)
      usage
      ;;
    *)
      break
      ;;
  esac
done

if test $# -eq 0 ; then
  usage
fi

case "$toolname" in
  default)
    exec   env GCONV_PATH="${builddir}"/iconvdata LOCPATH="${builddir}"/localedata LC_ALL=C  "${builddir}"/elf/ld-linux-x86-64.so.2 --library-path "${builddir}":"${builddir}"/math:"${builddir}"/elf:"${builddir}"/dlfcn:"${builddir}"/nss:"${builddir}"/nis:"${builddir}"/rt:"${builddir}"/resolv:"${builddir}"/mathvec:"${builddir}"/support:"${builddir}"/misc:"${builddir}"/debug:"${builddir}"/nptl:/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/../../../../lib/:/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/../../../../lib/ ${1+"$@"}
    ;;
  strace*)
    exec $toolname  -EGCONV_PATH=/home/nestos/test/glibc-2.43/build/iconvdata  -ELOCPATH=/home/nestos/test/glibc-2.43/build/localedata  -ELC_ALL=C  /home/nestos/test/glibc-2.43/build/elf/ld-linux-x86-64.so.2 --library-path /home/nestos/test/glibc-2.43/build:/home/nestos/test/glibc-2.43/build/math:/home/nestos/test/glibc-2.43/build/elf:/home/nestos/test/glibc-2.43/build/dlfcn:/home/nestos/test/glibc-2.43/build/nss:/home/nestos/test/glibc-2.43/build/nis:/home/nestos/test/glibc-2.43/build/rt:/home/nestos/test/glibc-2.43/build/resolv:/home/nestos/test/glibc-2.43/build/mathvec:/home/nestos/test/glibc-2.43/build/support:/home/nestos/test/glibc-2.43/build/misc:/home/nestos/test/glibc-2.43/build/debug:/home/nestos/test/glibc-2.43/build/nptl:/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/../../../../lib/:/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/../../../../lib/ ${1+"$@"}
    ;;
  rpctrace)
    exec rpctrace  -EGCONV_PATH=/home/nestos/test/glibc-2.43/build/iconvdata  -ELOCPATH=/home/nestos/test/glibc-2.43/build/localedata  -ELC_ALL=C  /home/nestos/test/glibc-2.43/build/elf/ld-linux-x86-64.so.2 --library-path /home/nestos/test/glibc-2.43/build:/home/nestos/test/glibc-2.43/build/math:/home/nestos/test/glibc-2.43/build/elf:/home/nestos/test/glibc-2.43/build/dlfcn:/home/nestos/test/glibc-2.43/build/nss:/home/nestos/test/glibc-2.43/build/nis:/home/nestos/test/glibc-2.43/build/rt:/home/nestos/test/glibc-2.43/build/resolv:/home/nestos/test/glibc-2.43/build/mathvec:/home/nestos/test/glibc-2.43/build/support:/home/nestos/test/glibc-2.43/build/misc:/home/nestos/test/glibc-2.43/build/debug:/home/nestos/test/glibc-2.43/build/nptl:/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/../../../../lib/:/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/../../../../lib/ ${1+"$@"}
    ;;
  valgrind*)
    exec env GCONV_PATH=/home/nestos/test/glibc-2.43/build/iconvdata LOCPATH=/home/nestos/test/glibc-2.43/build/localedata LC_ALL=C $toolname  /home/nestos/test/glibc-2.43/build/elf/ld-linux-x86-64.so.2 --library-path /home/nestos/test/glibc-2.43/build:/home/nestos/test/glibc-2.43/build/math:/home/nestos/test/glibc-2.43/build/elf:/home/nestos/test/glibc-2.43/build/dlfcn:/home/nestos/test/glibc-2.43/build/nss:/home/nestos/test/glibc-2.43/build/nis:/home/nestos/test/glibc-2.43/build/rt:/home/nestos/test/glibc-2.43/build/resolv:/home/nestos/test/glibc-2.43/build/mathvec:/home/nestos/test/glibc-2.43/build/support:/home/nestos/test/glibc-2.43/build/misc:/home/nestos/test/glibc-2.43/build/debug:/home/nestos/test/glibc-2.43/build/nptl:/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/../../../../lib/:/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/../../../../lib/ ${1+"$@"}
    ;;
  container)
    exec env GCONV_PATH=/home/nestos/test/glibc-2.43/build/iconvdata LOCPATH=/home/nestos/test/glibc-2.43/build/localedata LC_ALL=C  /home/nestos/test/glibc-2.43/build/elf/ld-linux-x86-64.so.2 --library-path /home/nestos/test/glibc-2.43/build:/home/nestos/test/glibc-2.43/build/math:/home/nestos/test/glibc-2.43/build/elf:/home/nestos/test/glibc-2.43/build/dlfcn:/home/nestos/test/glibc-2.43/build/nss:/home/nestos/test/glibc-2.43/build/nis:/home/nestos/test/glibc-2.43/build/rt:/home/nestos/test/glibc-2.43/build/resolv:/home/nestos/test/glibc-2.43/build/mathvec:/home/nestos/test/glibc-2.43/build/support:/home/nestos/test/glibc-2.43/build/misc:/home/nestos/test/glibc-2.43/build/debug:/home/nestos/test/glibc-2.43/build/nptl:/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/../../../../lib/:/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/../../../../lib/ /home/nestos/test/glibc-2.43/build/support/test-container env GCONV_PATH=/home/nestos/test/glibc-2.43/build/iconvdata LOCPATH=/home/nestos/test/glibc-2.43/build/localedata LC_ALL=C  /home/nestos/test/glibc-2.43/build/elf/ld-linux-x86-64.so.2 --library-path /home/nestos/test/glibc-2.43/build:/home/nestos/test/glibc-2.43/build/math:/home/nestos/test/glibc-2.43/build/elf:/home/nestos/test/glibc-2.43/build/dlfcn:/home/nestos/test/glibc-2.43/build/nss:/home/nestos/test/glibc-2.43/build/nis:/home/nestos/test/glibc-2.43/build/rt:/home/nestos/test/glibc-2.43/build/resolv:/home/nestos/test/glibc-2.43/build/mathvec:/home/nestos/test/glibc-2.43/build/support:/home/nestos/test/glibc-2.43/build/misc:/home/nestos/test/glibc-2.43/build/debug:/home/nestos/test/glibc-2.43/build/nptl:/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/../../../../lib/:/usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/../../../../lib/ ${1+"$@"}
    ;;
  *)
    usage
    ;;
esac
