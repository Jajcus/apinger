
EXTRA_DIST = autogen.sh TODO BUGS

SUBDIRS = src doc

.PHONY: ChangeLog

ChangeLog: 
	test -d .git && make cl-stamp || :
	
cl-stamp: .git
	git log > ChangeLog
	touch cl-stamp
