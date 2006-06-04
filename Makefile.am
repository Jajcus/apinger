# $Id$

EXTRA_DIST = autogen.sh TODO BUGS

SUBDIRS = src doc

.PHONY: ChangeLog

ChangeLog: 
	test -f .svn/entries && make cl-stamp || :
	
cl-stamp: .svn/entries
	TZ=UTC svn log -v --xml \
		| aux/svn2log.py -p '/(branches/[^/]+|trunk)' -x ChangeLog -u aux/users -F
	touch cl-stamp
