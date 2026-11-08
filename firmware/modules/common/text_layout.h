#pragma once
#include <string.h>
#include "utf8.h"

namespace TextLayout {
struct WrapResult {
  int count;
  size_t consumed;
  bool complete;
};

inline size_t previousChar(const char* text,size_t end){
  size_t i=0,prev=0;
  while(i<end){ prev=i; i+=TextUtf8::charBytes(text+i); }
  return prev;
}

inline bool isClosing(const char* text){
  char ch[5]{};
  int bytes=TextUtf8::charBytes(text);
  if(!bytes) return false;
  memcpy(ch,text,bytes);
  return strstr("，。、；：！？）】》」』,.!?;:)]",ch)!=nullptr;
}

inline bool isOpening(const char* text){
  char ch[5]{};
  int bytes=TextUtf8::charBytes(text);
  if(!bytes) return false;
  memcpy(ch,text,bytes);
  return strstr("（【《「『([",ch)!=nullptr;
}

template<size_t Capacity,typename Measure>
WrapResult wrap(const char* text,int maxWidth,char lines[][Capacity],int limit,Measure width){
  if(!text) text="";
  size_t offset=0;
  int count=0;
  while(text[offset] && count<limit){
    while(text[offset]==' ') ++offset;
    if(!text[offset]) break;
    char* line=lines[count];
    line[0]=0;
    size_t used=0;
    while(text[offset+used] && text[offset+used]!='\n'){
      int bytes=TextUtf8::charBytes(text+offset+used);
      if(used+bytes>=Capacity) break;
      memcpy(line+used,text+offset+used,bytes);
      line[used+bytes]=0;
      if(width(line)>maxWidth){ line[used]=0; break; }
      used+=bytes;
    }
    if(!used && text[offset]!='\n') return {count,offset,false};
    if(text[offset+used] && text[offset+used]!='\n' && used){
      // Carry a character with closing punctuation; never hang it past the margin.
      size_t cut=previousChar(line,used);
      if(cut && (isClosing(text+offset+used) || isOpening(line+cut))){
        used=cut;
        line[used]=0;
      }
      // Keep space-separated activity names together when there is a break point.
      if(text[offset+used]!=' '){
        for(size_t i=used;i>0;--i){
          if(line[i-1]==' ' && i>1){ used=i; line[used]=0; break; }
        }
      }
    }
    offset+=used;
    if(text[offset]=='\n') ++offset;
    size_t visible=used;
    while(visible && line[visible-1]==' ') --visible;
    line[visible]=0;
    ++count;
  }
  while(text[offset]==' ' || text[offset]=='\n') ++offset;
  return {count,offset,text[offset]==0};
}

template<size_t Capacity,typename Measure>
void ellipsize(char (&line)[Capacity],int maxWidth,Measure width){
  size_t used=strlen(line);
  for(;;){
    if(used+3<Capacity){
      memcpy(line+used,"...",4);
      if(width(line)<=maxWidth) return;
    }
    line[used]=0;
    if(!used) return;
    used=previousChar(line,used);
    line[used]=0;
  }
}
}
