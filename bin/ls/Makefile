#	@(#)Makefile	8.1 (Berkeley) 6/2/93
# $FreeBSD$

.include <src.opts.mk>

PROG=	ls
SRCS=	cmp.c ls.c print.c util.c
CFLAGS+= ${CARGS}
LIBADD=	xo util

.if !defined(RELEASE_CRUNCH) && \
	${MK_LS_COLORS} != no
CFLAGS+= -DCOLORLS
LIBADD+=	termcapw
.endif

# sudo pkg install npm
# sudo npm install jsonlint -g
JSONLINT?=/usr/local/bin/jsonlint
LN?=/bin/ln
MKDIR?=/bin/mkdir
RM?=/bin/rm
TOUCH?=/usr/bin/touch
LS_JSON?=./ls --libxo json,pretty
json-test:
	@${RM} -rf test_dir
	@${MKDIR} -p test_dir/nested_1/nested_2
	@${TOUCH} test_dir/file_1.1
	@${TOUCH} test_dir/file_1.2
	@${TOUCH} test_dir/file_1.3
	@${TOUCH} test_dir/nested_1/file_2.1
	@${TOUCH} test_dir/nested_1/file_2.2
	@${LN} -s ../file_1.2 test_dir/nested_1/file_1.2
	@echo Testing no flags
	${LS_JSON} test_dir | ${JSONLINT}
	@echo Testing multi-column output
	${LS_JSON} -C test_dir | ${JSONLINT}
	@echo Testing trailing path type output
	${LS_JSON} -F test_dir | ${JSONLINT}
	@echo Testing recursive output
	${LS_JSON} -R test_dir | ${JSONLINT}
	@echo Testing recursive trailing path type output
	${LS_JSON} -FR test_dir | ${JSONLINT}
	@echo Testing long output
	${LS_JSON} -l test_dir | ${JSONLINT}
	@echo Testing long output with trailing path type
	${LS_JSON} -lF test_dir | ${JSONLINT}
	@echo Testing long recursive output
	${LS_JSON} -lR test_dir | ${JSONLINT}
	@echo Testing long recursive output with trailing path type
	${LS_JSON} -lFR test_dir | ${JSONLINT}
	@echo Testing colorized output
	${LS_JSON} -G test_dir | ${JSONLINT}
	@echo Testing sort-by-size output
	${LS_JSON} -S test_dir | ${JSONLINT}
	@echo Testing inode output
	${LS_JSON} -i test_dir | ${JSONLINT}
	@echo Testing reverse sort output
	${LS_JSON} -r test_dir | ${JSONLINT}
	@echo Testing custom date format output
	${LS_JSON} -l -D "%A %C" test_dir | ${JSONLINT}
	@echo Testing stream format output
	${LS_JSON} -m test_dir | ${JSONLINT}
	@echo Testing recursive stream format output
	${LS_JSON} -mR test_dir | ${JSONLINT}
	@${RM} -rf test_dir

.include <bsd.prog.mk>
