#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>

#define PARAM_NONE 	0
#define PARAM_F 	1
#define PARAM_R 	2
#define PARAM_L 	4
#define PARAM_S 	8
#define PARAM_V 	16

#define MY_ERR(x) my_err(x,__LINE__)

void my_err(const char *err_string, int line);
void cp(const char *src, const char *dst);
void get_name(char* name, const char *pathname);
void cp_real(int fd, const char *src, const char *dst);

uint32_t dst_mode;
uint32_t arg = 0;

int main(int argc, char *argv[]){
	if(argc <= 2){
		printf("Usage: %s [-frlsv] source dest\n", argv[0]);
		return EXIT_SUCCESS;
	}
	const char *src = NULL, *dst = NULL;
	for(int i = 1; i < argc; ++i){
		if(argv[i][0] == '-'){
			size_t len = strlen(argv[i]);
			for(int j = 1; j < len; ++j){
				switch(argv[i][j]){
					case 'r':
						arg |= PARAM_R;
						break;
					case 'f':
						arg |= PARAM_F;
						break;
					case 'l':
						arg |= PARAM_L;
						break;
					case 's':
						arg |= PARAM_S;
						break;
					case 'v':
						arg |= PARAM_V;
						break;
					default:
						break;
				}
			}
		}else{
			if( !src )
				src = argv[i];
			else if( !dst )
				dst = argv[i];
			else
				MY_ERR("param too much");
		}
	}

	dst_mode = O_RDWR | O_CREAT;
	if(arg & PARAM_F)
		dst_mode |= O_TRUNC;
	cp(src, dst);

	return 0;
}

void cp_real(int fd, const char *src, const char *dst){
	if( (arg & PARAM_L) ){
		if(arg & PARAM_V) printf("link %s -> %s\n", src, dst);
		link(src, dst);
	}else if ( (arg & PARAM_S) ){
		if(arg & PARAM_V) printf("symlink %s -> %s\n", src, dst);
		symlink(src, dst);
	}else{
		if(arg & PARAM_V) printf("copydata %s -> %s\n", src, dst);
		struct stat src_statbuf;
		fstat(fd, &src_statbuf);
		int dstfd = open(dst, dst_mode, 0644);
		struct stat dst_statbuf;
		fstat(dstfd, &dst_statbuf);
		if(!dst_statbuf.st_size)
			ftruncate(dstfd, src_statbuf.st_size);
		char buf[100];
		int size;
		while( (size=read(fd, buf, sizeof(buf) ) ) > 0)
			write(dstfd, buf, size );
	}
}

void cp(const char *src, const char *dst){
	int srcfd = open(src, O_RDONLY);
	if( srcfd < 0 ){
		MY_ERR("source file not exist");
		return;
	}
	struct stat src_statbuf;
	fstat(srcfd, &src_statbuf);
	int dstfd = open(dst, O_RDONLY);
	if( dstfd > 0 && !(arg & PARAM_F) ){
		MY_ERR("dest file is exist");
		goto exit;
	}
	struct stat dst_statbuf;
	if(dstfd > 0)
		fstat(dstfd, &dst_statbuf);
	if( S_ISDIR(src_statbuf.st_mode) ){
		if(arg & PARAM_V) printf("process directory %s\n", src);
		if( !(arg & PARAM_R) ){
			MY_ERR("source file is directory");
			goto exit;
		}
		mkdir(dst, 0775);

		DIR *pDir;
		struct dirent *ent;
		pDir = opendir(src);
		while( (ent = readdir(pDir) )!= NULL){
			if( !strcmp(ent->d_name,".") || !strcmp(ent->d_name,"..") )
				continue;
			char buf1[512];		
			char buf2[512];
			strcpy(buf1, src);
			strcat(buf1, "/");
			strcat(buf1, ent->d_name);
			strcpy(buf2, dst);
			strcat(buf2, "/");
			strcat(buf2, ent->d_name);
			cp(buf1, buf2);
		}
	}else{
		if( dstfd > 0 && S_ISDIR(dst_statbuf.st_mode) ){
			if(arg & PARAM_V) printf("process directory %s\n", dst);
			char buf[512];
			char name[512];
			strcpy(buf, dst);
			strcat(buf, "/");
			get_name(name, src);
			strcat(buf, name);
			cp(src, buf);
		}else{
			close(dstfd);
			cp_real(srcfd, src, dst);
		}
		close(dstfd);
	}
exit:
	close(srcfd);
}

void get_name(char* name, const char *pathname)
{
	int i = 0 , j = 0;

	for (i = 0, j = 0; i < strlen(pathname); i++){
		if (pathname[i] == '/'){
			j = 0;
			continue;
		}
		name[j++] = pathname[i];
	}
	name[j] = '\0';
}

void my_err(const char *err_string, int line){
	fprintf(stderr, "line: %d ", line);
	perror(err_string);
	exit(1);
}
