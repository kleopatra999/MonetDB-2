#!/usr/bin/env python

# The contents of this file are subject to the MonetDB Public
# License Version 1.0 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at
# http://monetdb.cwi.nl/Legal/MonetDBLicense-1.0.html
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Monet Database System.
#
# The Initial Developer of the Original Code is CWI.
# Portions created by CWI are Copyright (C) 1997-2004 CWI.
# All Rights Reserved.

import sys
import fileinput
import os

if sys.argv[1] == '-DHAVE_JAVA':
    del sys.argv[1]
    have_java_false = '#'
else:
    have_java_false = ''

build=os.path.abspath(sys.argv[1]);
prefix=os.path.abspath(sys.argv[2]);

source=os.path.abspath(os.path.join(build,os.pardir))

subs = [
    ('@exec_prefix@',            prefix),
    ('@sysconfdir@',             r'${prefix}\etc'),
    ('@localstatedir@',          r'${prefix}\var'),
    ('@libdir@',                 r'${prefix}\lib'),
    ('@bindir@',                 r'${prefix}\bin'),
    ('@mandir@',                 r'${prefix}\man'),
    ('@includedir@',             r'${prefix}\include'),
    ('@datadir@',                r'${prefix}\share'),
    ('@infodir@',                r'${prefix}\info'),
    ('@libexecdir@',             r'${prefix}\libexec'),
    ('@PACKAGE@',                r'MonetDB'),
    ('@VERSION@',                r'4.4.0'),
    ('@DIRSEP@',                 '\\'),
    ('@prefix@',                 prefix),
    ('@MONETDB_BUILD@',          build),
    ('@MONETDB_SOURCE@',         source),
    ('@NO_X_CFLAGS@',            ''),
    # conditionals for Mtest.py (see comment over there)
    ('@CROSS_COMPILING_FALSE@',  '#'),
    ('@DOCTOOLS_FALSE@',         '#'),
    ('@HAVE_JAVA_FALSE@',        have_java_false),
    ('@HAVE_MONET5_FALSE@',      ''),
    ('@HAVE_MONET_FALSE@',       '#'),
    ('@HAVE_PERL_FALSE@',        ''),
    ('@HAVE_PERL_SWIG_FALSE@',   ''),
    ('@HAVE_PHP_FALSE@',         ''),
    ('@HAVE_PYTHON_FALSE@',      '#'),
    ('@HAVE_PYTHON_SWIG_FALSE@', ''),
    ('@HAVE_SWIG_FALSE@',        ''),
    ('@LINK_STATIC_FALSE@',      ''),
    ('@NOT_WIN32_FALSE@',        ''),
    ('@PROFILING_FALSE@',        ''),
    # SQL:
    ('@MONET4_FALSE@',           '#'),
    ('@MONET5_FALSE@',           ''),
    ('@NATIVE_WIN32_FALSE@',     '#'),
]

for key, val in subs[:]:
    subs.insert(0, ('@Q'+key[1:], val.replace('\\', r'\\')))


def substitute(line):
    for (p,v) in subs:
        line = line.replace(p, v);
    return line

for line in fileinput.input(sys.argv[3:]):
    sys.stdout.write(substitute(line))
