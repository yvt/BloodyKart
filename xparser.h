/*
 *  xparser.h
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/23.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#ifndef XPARSER_H_INCLUDED
#define XPARSER_H_INCLUDED

#define XPARSER_BUFFER_SIZE     256
#define XPARSER_STRING_SIZE        256

#define XPARSER_DEBUG(text)     fprintf(stderr, \
"xparser: %s\n", text)

struct _XFILE{
    FILE *f;
    char buffer[XPARSER_BUFFER_SIZE+1];
    int next;
    char id[XPARSER_STRING_SIZE];
    char name[XPARSER_STRING_SIZE];
};


typedef _XFILE *XFILE;
#define XNODETYPE       int

#define XNODE_DATA          ((XNODETYPE)(0x0001))
#define XNODE_MEMBER_VALUE  ((XNODETYPE)(0x0101))
#define XNODE_MEMBER_STRING ((XNODETYPE)(0x0102))
#define XNODE_UUID          ((XNODETYPE)(0x0201))
#define XNODE_EOD           ((XNODETYPE)(0x0002))
#define XNODE_EOF           ((XNODETYPE)(0x0000))

XFILE xparserOpen(const char *);
void xparserClose(XFILE);
void xparserMoveToContent(XFILE);
XNODETYPE xparserNodeType(XFILE);
void xparserReadUUID(char *, XFILE);
void xparserBeginData(XFILE);
void xparserReadID(char *, XFILE);
void xparserReadName(char *, XFILE);
void xparserEndData(XFILE);
double xparserReadValue(XFILE);
void xparserReadString(char *, XFILE);

#endif // XPARSER_H_INCLUDED



