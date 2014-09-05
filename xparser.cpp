/*
 *  xparser.cpp
 *  BloodyKart
 *
 *  Created by tcpp on 09/08/23.
 *  Copyright 2009 tcpp. All rights reserved.
 *
 */

#include "global.h"

#include "xparser.h"


char _xparserPeekChar(XFILE f){
    return f->buffer[f->next];
}
char _xparserReadChar(XFILE f){
    char c;
    c=f->buffer[f->next];
    if(f->next==XPARSER_BUFFER_SIZE-1){
        f->next=0;
        fread(f->buffer, 1, XPARSER_BUFFER_SIZE, f->f);
    }else{
        f->next++;
    }
   // fprintf(stderr,"%c",c);
    return c;
}
void _xparserSkip(XFILE f){
    if(f->next==XPARSER_BUFFER_SIZE-1){
        f->next=0;
        fread(f->buffer, 1, XPARSER_BUFFER_SIZE, f->f);
    }else{
        f->next++;
    }
}


XFILE xparserOpen(const char *filename){
    FILE *f;
    XFILE x;
    char header[16];
    long n;

    f=fopen(filename, "rb");

    if(f==NULL)
        return NULL;
    if(fread(header, 1, 16, f)<16){
        fclose(f);
        return NULL;
    }
    x=(XFILE)malloc(sizeof(struct _XFILE));
    x->f=f;
    for(n=0;n<XPARSER_BUFFER_SIZE;n++)
        x->buffer[n]=0;
    fread(x->buffer, 1, XPARSER_BUFFER_SIZE, f);
    x->next=0;
    return x;
}
void xparserClose(XFILE f){
    if(f){
        fclose(f->f);
        free(f);
    }
}
void xparserMoveToContent(XFILE f){
    while(_xparserPeekChar(f)==' ' || _xparserPeekChar(f)==13 ||
		  _xparserPeekChar(f)==10 || _xparserPeekChar(f)==',' || _xparserPeekChar(f)==';'){
        _xparserSkip(f);
    }
}
XNODETYPE xparserNodeType(XFILE f){
    if(f==NULL)
        XPARSER_DEBUG("f==NULL");
    xparserMoveToContent(f);
    //printf("pc: %c\n", _xparserPeekChar(f));
    switch(_xparserPeekChar(f)){
        case 0:
            return XNODE_EOF;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '-':
        case '.':
            return XNODE_MEMBER_VALUE;
        case '"':
            return XNODE_MEMBER_STRING;
        case '<':
            return XNODE_UUID;
        case '}':
            return XNODE_EOD;
        default:
            return XNODE_DATA;
    }
}
void xparserReadUUID(char *buf, XFILE f){
    long len;
    if(xparserNodeType(f)!=XNODE_UUID)
        XPARSER_DEBUG("xparserNodeType(f)!=XNODE_UUID");
    len=0;
    _xparserSkip(f);
    while(_xparserPeekChar(f)!='>'){
        buf[len++]=_xparserReadChar(f);
    }
    buf[len]=0;
    _xparserSkip(f);
}

void xparserBeginData(XFILE f){
    char txt[XPARSER_STRING_SIZE*2];
    long len, len2, n;
    int x;
    if(xparserNodeType(f)!=XNODE_DATA)
        XPARSER_DEBUG("xparserNodeType(f)!=XNODE_DATA");
    len=0;
    while(_xparserPeekChar(f)!='{' && _xparserPeekChar(f)!='}'){
        txt[len++]=_xparserReadChar(f);
    }
    txt[len]=0;
    //printf("120: txt=%s\n", txt);
    len--;
    while(txt[len]==' ' || txt[len]==13 || txt[len]==10){
        txt[len]=0;
        len--;
        if(len==-1)
            break;
    }
	if(_xparserPeekChar(f)=='{')
		_xparserSkip(f);
    //printf("129: txt=%s\n", txt);
    len=0;
    len2=0;
    n=0;
    x=0;
    for(n=0;n<strlen(txt);n++){
        if(txt[n]==' '){
            x=1;
        }else if(x==0){
            f->id[len++]=txt[n];
        }else if(x==1){
            f->name[len2++]=txt[n];
        }
    }
    f->id[len]=0;
    f->name[len2]=0;
    //printf("145: ID=%s\n", f->id);
    //printf("146: Name=%s\n", f->name);
}
void xparserReadID(char *buf, XFILE f){
    strcpy(buf, f->id);
}
void xparserReadName(char *buf, XFILE f){
    strcpy(buf, f->name);
}
void xparserEndData(XFILE f){
    long lv;
    lv=0;
    //puts("\n *** EndData ***\n");
    while(1){
        //printf("%c",_xparserPeekChar(f));
        switch(_xparserReadChar(f)){
            case '{':
                lv++;
                break;
            case '}':
                if(lv==0)
                    return;
                lv--;
                break;
        }
    }
}

double xparserReadValue(XFILE f){
    char buf[XPARSER_STRING_SIZE];
    long len;
    char c;
    if(xparserNodeType(f)!=XNODE_MEMBER_VALUE)
        XPARSER_DEBUG("xparserNodeType(f)!=XNODE_MEMBER_VALUE");
    len=0;
    while(1){
        c=_xparserPeekChar(f);
        switch(c){
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case 'e':
            case '+':
            case '-':
            case '.':
                buf[len++]=c;
                _xparserSkip(f);
                break;
            default:
                goto ok;                //GOTO OK
        }
    }
ok: buf[len]=0;                         //LABEL OK
    return atof(buf);
}
void xparserReadString(char *buf, XFILE f){
    long len;
    char c;
    if(xparserNodeType(f)!=XNODE_MEMBER_STRING)
        XPARSER_DEBUG("xparserNodeType(f)!=XNODE_MEMBER_STRING");
    len=0;
    _xparserSkip(f);
    while((c=_xparserReadChar(f))!='"'){
        buf[len++]=c;
    }
    buf[len]=0;
}


