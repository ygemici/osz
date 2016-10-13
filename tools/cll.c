/*
 * cll.c
 * 
 * Copyright 2016 GPLv3 bztsrc@github
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * @brief CL Dynamic Linkage section generator
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* SOURCE FORMAT
   //@ class <namespace>
   //@ [public|private|protected|static|extern] <type> <property name>
   //@ [public|private|protected|static|extern] [<return type>|void] <method name> ( [<type> <name>,]* ) <elf symbol>
*/

/* BINARY FORMAT
      0     4 magic, 'CLBF'
      4     4 size
      8     records
      x     0xFF 0xFF stringtable 0x0
    Record
      0     2 flags (public/private,prop/method,protected,static,extern)
      2     2 pointer to class name relative to 'CLBF'
      4     2 pointer to return value
      6     2 pointer to method name
      8     8 c function reference (address)
     16     2 reserved
     18     2 pointer/scalar mask for arguments
                bit 0: first arg's class, 0=scalar, 1=pointer
                bit 1: 2nd arg's class
                 ...
                bit 11: 12th arg's class
                 ...
                bit 15: return's class
     20   2+2 first argument class+name
     24   2+2 2nd argument class+name
     28   2+2 3rd argument class+name
     32   2+2 4th argument class+name
     36   2+2 5th argument class+name
     40   2+2 6th argument class+name
     44   2+2 7th argument class+name
     48   2+2 8th argument class+name
     52   2+2 9th argument class+name
     56   2+2 10th argument class+name
     60   2+2 11th argument class+name
    Last record closed with 2 0xFF characters. After that
    cames the stringtable, zero terminated utf-8 strings, terminated
    by another zero. So the block ends with 2 zero characters.
*/
typedef struct {
    int     type;
    int     name;
    int     isptr;
} LDParam;

typedef struct {
    int     flags;
    int     classname;
    char*   functionname;
    int     mask;
    LDParam returnclass;
    LDParam param[12];
} LDRec;

//char *types[]={"None","Class32","Class64","ClassNUM","Class"};
long int read_size;

int num_str=0;
char **strs=NULL;
int *idx=NULL;
int offs=0;

int num_method=0;
LDRec *methods=NULL;


//reads a file into memory
char* readfileall(char *file)
{
    char *data=NULL;
    FILE *f;
    read_size=0;
    f=fopen(file,"r");
    if(f){
        fseek(f,0L,SEEK_END);
        read_size=ftell(f);
        fseek(f,0L,SEEK_SET);
        data=(char*)malloc(read_size+1);
        if(data==NULL) { printf("cll: Unable to allocate %ld memory\n",read_size+1); exit(1); }
        memset(data,0,read_size+1);
        fread(data,read_size,1,f);
        fclose(f);
        data[read_size]=0;
    }
    return data;
}

void addmethod(LDRec *method)
{
    methods=realloc(methods,(num_method+1)*sizeof(LDRec));
    memcpy(methods+num_method*sizeof(LDRec),method,sizeof(LDRec));
    num_method++;
}

int addstr(char *start, int size)
{
    int i;
    for(i=0;i<num_str;i++){
        if(strlen(strs[i])==size && !strncmp(strs[i],start,size))
            return i;
    }
    strs=realloc(strs,(num_str+1)*sizeof(char*));
    idx=realloc(idx,(num_str+1)*sizeof(int));
    strs[num_str]=malloc(size+1);
    memcpy(strs[num_str],start,size);
    strs[num_str][size]=0;
    idx[num_str]=offs;
    offs+=size+1;
    num_str++;
    return num_str-1;
}

char *readclassname(char *str,LDParam *param,int mode)
{
    char *start=str;
    while(*str==' '||*str=='\t'||*str=='\r'||*str=='\n') {
        if(str>start+512) return start;
        str++;
    }
    if(*str=='*') { str++; if(param) param->isptr=1; }
    start=str;
    while(str<start+128 && *str!=0 && *str!='[' && *str!='(' && *str!=' ' && *str!='*' && *str!='\r' && *str!='\n' && *str!=',') str++;
    if(param && (*str=='*'||*str=='[')) param->isptr=1;
    if(start[0]=='.') {
        param->type=addstr("__VA_ARGS__",11);
        param->name=addstr("",0);
        return start+1;
    }
    if(mode==1)
        param->type=addstr(start,str-start);
    if(mode==2)
        param->name=addstr(start,str-start);
    if(*str=='*') str++;
    return str;
}

void parsefile(char *file)
{
    char *data=readfileall(file);
    char *ptr=data;
    LDParam cn;
    int cnflags;
    if(data==NULL) return;
    while(ptr[0]!=0){
        if(!strncmp(ptr,"//@ ",4)){
            LDRec method;
            char a,*str=ptr+4;
            memset(&method,0,sizeof(LDRec));
            method.classname=cn.name;
            while(ptr[0]!=0&&ptr[0]!='\n') ptr++;
            a=*ptr;*ptr=0;printf("CL: %s\n",str);*ptr=a;
            // modifiers
modifiers:
            if(!strncmp(str,"public ",7)){
                str+=7;
                method.flags|=2;
                method.flags&=~4;
                goto modifiers;
            }
            if(!strncmp(str,"private ",8)){
                str+=8;
                method.flags&=~2;
                method.flags&=~4;
                goto modifiers;
            }
            if(!strncmp(str,"protected ",10)){
                str+=10;
                method.flags&=~2;
                method.flags|=4;
                goto modifiers;
            }
            if(!strncmp(str,"static ",7)){
                str+=7;
                method.flags|=8;
                goto modifiers;
            }
            if(!strncmp(str,"extern ",7)){
                str+=7;
                method.flags|=16;
                goto modifiers;
            }
            if(!strncmp(str,"class ",6)){
                // class
                str+=6;
                str=readclassname(str,&cn,2);
                cnflags=method.flags;
                continue;
            } else {
                char *start;
                str=readclassname(str,&method.returnclass,1);    //returns
                str=readclassname(str,&method.returnclass,2);    //method/property name
                if(method.returnclass.isptr)
                    method.mask|=(1<<15);
                while(*str=='['){
                    start=str;
                    while(*str!=']'&&*str!=0) {
                        if(str>start+128) continue;
                        str++;
                    }
                }
                start=str;
                while(*str==' '||*str=='\t'||*str=='\r'||*str=='\n') {
                    if(str>start+128) continue;
                    str++;
                }
                if(*str=='('){
                    int i=0;
                    // method
                    method.flags&=~1;
                    str++;
                    start=str;
                    while(*str!=')'){
                        str=readclassname(str,&method.param[i],1);    //argument type
                        str=readclassname(str,&method.param[i],2);    //argument name
                        if(method.returnclass.isptr)
                            method.mask|=(1<<i);
                        while(*str==' '||*str=='\t'||*str==',') {
                            if(str>start+512) continue;
                            str++;
                        }
                         if(*str==')') break;
                        if(str>start+512||i>=10) {
                            while(*str!=')'||*str!=0)str++;
                            break;
                        }
                        str++; i++;
                    }
                    start=str;
                    while(*str==' '||*str=='\t'||*str==')') {
                        if(str>start+128) continue;
                        str++;
                    }
                    start=str;
                    while(*str!='\n'&&*str!=0) {
                        if(str>start+128) continue;
                        str++;
                    }
                    method.functionname=malloc(str-start+1);
                    memcpy(method.functionname,start,str-start);
                    method.functionname[str-start]=0;
                } else {
                    // property
                    method.flags|=1;
                }
            }
            //for static class, turn on static flag on every member
            if(cnflags|=8)
                method.flags|=8;
            //extern likewise
            if(cnflags|=16)
                method.flags|=16;

            addmethod(&method);
            continue;
        }
        ptr++;
    }
    free(data);
}

int main(int argc, char**argv)
{
    int i,j;
    FILE *f;
    if(argc<3) {
        printf("CL Dynamic Linkage section generator\n\nUsage:\n  %s (namespace) FILES...\n\nScans for CL definitions in given files and creates cl.S and cl.o with linkage info.\nLink that when creating shared objects and CL will use your .so.\n\nExample:\n  %s com.acme.libpng *.c *.h *.cpp *.java\n  gcc -shared cl.o main.o png.o -o com.acme.libpng.so\n",argv[0],argv[0]);
        exit(1);
    }
    addstr("",0);
    for(i=1;i<argc;i++) {
        parsefile(argv[i]);
    }

    // save information
    f=fopen("cl.S","w");
    if(f){
        // relocate strings
        j=8+num_method*64+2; for(i=0;i<num_str;i++) idx[i]+=j;
        // print out assembly
        fprintf(f,"# Coder's Language Dynamic Linkage information\n\n");
        for(i=0;i<num_method;i++){
            if(methods[i].functionname!=NULL)
                fprintf(f,".extern %s\n", methods[i].functionname);
        }
        j+=offs;
        fprintf(f,"\n# Header\n.section .cl\n.byte 'C','L','D','L'\n.hword %d, %d\n\n",j%65536,j/65536);
        for(i=0;i<num_method;i++){
            fprintf(f,".hword 0x%04x\t\t# %s%s %s\n.hword %d, %d, %d\t# %s%s %s.%s\n.long ",
                methods[i].flags,
                methods[i].flags&4?"protected":(methods[i].flags&2?"public":"private"),
                methods[i].flags&8?" static":"",
                methods[i].flags&1?"property":"method",
                idx[methods[i].classname],
                idx[methods[i].returnclass.type],
                idx[methods[i].returnclass.name],
                methods[i].returnclass.isptr?"*":"",
                strs[methods[i].returnclass.type],
                strs[methods[i].classname],
                strs[methods[i].returnclass.name]);
            if(methods[i].functionname!=NULL)
                fprintf(f,"%s\t\t# entry point",methods[i].functionname);
            else
                fprintf(f,"0");
            int last=0;
            for(j=0;j<12;j++) {
                if(methods[i].param[j].type==0) {
                    last=1;
                }
                if(last)
                    fprintf(f,"\n.hword 0, 0");
                else
                    fprintf(f,"\n.hword %d, %d\t\t# %s%s %s",
                        idx[methods[i].param[j].type],
                        idx[methods[i].param[j].name],
                        methods[i].param[j].isptr?"*":"",
                        strs[methods[i].param[j].name][0]==0?"...":strs[methods[i].param[j].type],
                        strs[methods[i].param[j].name]);
            }
            fprintf(f,"\n");
        }
        fprintf(f,"\n.byte 0xFF, 0xFF\t# no more records\n\n# String table\n");
        for(i=0;i<num_str;i++){
            fprintf(f,".asciz \"%s\"\n",strs[i]);
        }
        fprintf(f,".byte 0\n");
        fclose(f);
        // compile it into an ELF section
        // this way we can guarantee that the resulting
        // object file will be native. Although we don't
        // care as we only generate data, the linker does.
        char *arg[]={"gcc","-c","cl.S",NULL};
        execvp("gcc",arg);
        // no return
    }
}
