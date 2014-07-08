/* small re-entrant dir library by me, which is a wrapper for os-specific
   routines */
#include <string.h>
#include "dir.h"

#ifdef _WIN32
int dirwin(dir_t *h) {
	h->dir=(h->f.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)?1:0;
	h->s=h->f.cFileName;
	h->len=((ull)h->f.nFileSizeHigh<<32)+h->f.nFileSizeLow;
	h->nolen=0;
	return 1;
}
#else
int dirunix(dir_t *h) {
	h->f=readdir(h->d);
	if(!h->f) return 0;
	h->s=h->f->d_name;
#ifdef _DIRENT_HAVE_D_TYPE
	h->dir=h->f->d_type==DT_DIR;
#else
	h->dir=-1;
#endif
	h->nolen=1;
	return 1;
}
#endif

/* return 1 if ok, 0 if error */
/* warning, windows requires * at the end of the path, while linux does not! */
int findfirst(char *const s,dir_t *h) {
#ifdef _WIN32
	h->hfind=FindFirstFile(s,&h->f);
	if(INVALID_HANDLE_VALUE==h->hfind) return 0;
	return dirwin(h);
#else
	h->d=opendir(s);
	if(!h->d) return 0;
	return dirunix(h);
#endif
}

/* return 0 if no more files in directory */
int findnext(dir_t *h) {
#ifdef _WIN32
	if(!FindNextFile(h->hfind,&h->f)) return 0;
	return dirwin(h);
#else
	return dirunix(h);
#endif
}

void findclose(dir_t *h) {
#ifdef _WIN32
	FindClose(h->hfind);
#else
	closedir(h->d);
#endif
}

/* utility functions! */

/* get extension from file name, s is a buffer of n characters */
void getextension(const char *const t,char *s,const int n) {
	int p=strlen(t)-1,j;
	while(p && t[p]!='.') p--;
	if(p<0) s[0]=0;
	else {
		for(j=0,p++;t[p] && j<n-1;p++) s[j++]=t[p];
		s[j]=0;
	}
}

void getbasefilename(const char *const t,char *s,const int n) {
	int p=strlen(t)-1,j;
	while(p && t[p]!='.') p--;
	if(p<0) p=strlen(t);
	for(j=0;j<n-1 && t[j] && t[j]!='.';j++) s[j]=t[j];
	s[j]=0;
}
