#	@(#)Makefile	8.1 (Berkeley) 6/12/93
# $FreeBSD$

.include <src.opts.mk>

PROG=	netstat
SRCS=	if.c inet.c main.c mbuf.c mroute.c netisr.c route.c \
	unix.c mroute6.c ipsec.c bpf.c pfkey.c sctp.c \
	flowtable.c

WARNS?=	3
CFLAGS+=${CARGS}
CFLAGS+=-fno-strict-aliasing

CFLAGS+=-DIPSEC
CFLAGS+=-DSCTP

.if ${MK_INET_SUPPORT} != "no"
CFLAGS+=-DINET
.endif

.if ${MK_INET6_SUPPORT} != "no"
SRCS+=	inet6.c
CFLAGS+=-DINET6
.endif

.if ${MK_OFED} != "no"
CFLAGS+=-DSDP
.endif

.if ${MK_PF} != "no"
CFLAGS+=-DPF
.endif

BINGRP=	kmem
BINMODE=2555
LIBADD=	kvm memstat util

.if ${MK_NETGRAPH_SUPPORT} != "no"
SRCS+=	netgraph.c
LIBADD+=	netgraph
CFLAGS+=-DNETGRAPH
.endif

LIBADD+= xo

# sudo pkg install npm
# sudo npm install jsonlint -g
JSONLINT?=/usr/local/bin/jsonlint
NETSAT_JSON?=netstat --libxo json,pretty
json-test:
	@echo Testing no flags
	${NETSAT_JSON} | ${JSONLINT}
	@echo Testing all
	${NETSAT_JSON} -nA | ${JSONLINT}
	@echo Testing interfaces
	${NETSAT_JSON} -i | ${JSONLINT}
	@echo Testing repeat
	${NETSAT_JSON} -w 1 -q 3 | ${JSONLINT}
	@echo Testing mbufs 
	${NETSAT_JSON} -m | ${JSONLINT}
	@ echo Testing bpf
	${NETSAT_JSON} -B | ${JSONLINT}
	@echo Testing routes
	${NETSAT_JSON} -nr | ${JSONLINT}
	@echo Testing multicast
	${NETSAT_JSON} -g | ${JSONLINT}
	@echo Testing multicase routes
	${NETSAT_JSON} -gs | ${JSONLINT}
	@echo Testing netisr 
	${NETSAT_JSON} -Q | ${JSONLINT}

.include <bsd.prog.mk>
