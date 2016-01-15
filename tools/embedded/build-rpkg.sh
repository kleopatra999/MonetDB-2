#!/bin/sh
#set -x

STAGEDIR=/tmp/monetdb-embedded-stage
RPKG=MonetDBLite_0.1.0.tar.gz

rm -rf $STAGEDIR

hg archive $STAGEDIR/sourcetree

# generate mal/sql scripts and sql_parser.tab.c/h to remove our dependency on bison.
rm -f ../../sql/server/sql_parser.tab.*
cd ../../sql/server/
make sql_parser.tab.c 
make sql_parser.tab.h
cp sql_parser.tab.c sql_parser.tab.h $STAGEDIR/sourcetree/sql/server/
cd $STAGEDIR/sourcetree

# patch build system to only include things we need
echo "SUBDIRS = embedded" > tools/Makefile.ag
echo "SUBDIRS = mapilib" > clients/Makefile.ag
echo "SUBDIRS = mal modules optimizer scheduler tools" > monetdb5/Makefile.ag
echo "SUBDIRS = buildtools common clients gdk monetdb5 sql tools\nEXTRA_DIST = bootstrap configure configure.ac configure.ag libversions rpm.mk.in\nheaders_config = {\nDIR = includedir/monetdb\nHEADERS = h\nSOURCES = monetdb_config.h\n}\n" > Makefile.ag
sed -i -e '/^SUBDIRS = .*$/d' sql/backends/monet5/Makefile.ag
sed -i -e 's/sql_parser\.y/sql_parser.tab.h sql_parser.tab.c/' sql/server/Makefile.ag

./bootstrap
# buildtools/conf/lt~obsolete.m4 file name violates CRAN policies, rename and replace references
mv "buildtools/conf/lt~obsolete.m4" buildtools/conf/lt-obsolete.m4
find . -name "Makefile.in" -exec sed -i -e "s/lt~obsolete\.m4/lt-obsolete\.m4/g" {} \;

cd ..
mv sourcetree/tools/embedded/rpackage .
rsync -av --exclude-from sourcetree/tools/embedded/pkg-excludes sourcetree/ rpackage/src
 
export R_INCLUDE_DIR=`R CMD config --cppflags | sed s/^-I//`

# # inline mal/sql scripts, we need R with the stringr package for that
# python sourcetree/tools/embedded/inline.py dummytarget/libs/monetdb5/ rpackage/src/monetdb5/mal/mal_init_inline.h

mkdir -p rpackage/src/monetdb5/extras/rapi
touch rpackage/src/monetdb5/extras/rapi/placeholder

R CMD build rpackage

echo scp $STAGEDIR/$RPKG release@dev.monetdb.org:/var/www/html/Assets/R/

echo
echo 'install.packages("MonetDBLite", repos="http://dev.monetdb.org/Assets/R/", type="source")'
echo

# OSX 10.10/10.11 needs pkg-config!
