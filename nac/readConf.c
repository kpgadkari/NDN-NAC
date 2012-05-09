#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int read_config(char * filename){
    FILE *cfg;
    char *type, *last, *cp, *namespace, *gw;
    int len, i;
    i = 0;
    int count = 0;
    char buf[1024];
    const char *seps = " \t\n";
    cfg = fopen(filename, "r");
    if(cfg == NULL){
        fprintf(stderr, "opening file %s failed\n", filename);
	exit(1);
    }

    while (fgets((char*)buf, sizeof (buf), cfg)){
        len = strlen(buf);
	if(buf[0] == '#' || len == 0)//skip all lines starts with a #
	    continue;

	if(buf[len -1] == '\n')
	    buf[len - 1] = '\0';

	cp = index(buf, '#');//skip comments
	if(cp != NULL)
	    *cp = '\0';

        type = strtok_r(buf, seps, &last);
	if(type == NULL)//blank line
	    continue;
	if(strcmp(type, "namespace") != 0 && strcmp(type, "gateways") != 0){
	    printf("cannot recognize %s\n", type);
	    exit(1);
        }

	for (i=0; type[i]; i++)
   	    type[i] = tolower(type[i]);

	if(strcmp(type, "namespace") == 0){
	    //printf("name:<%s>\n", last);
	    while((namespace = strtok_r(NULL, seps, &last)) != NULL)
                printf("namespace:%s\n", namespace);
	}
	else if(strcmp(type, "gateways") == 0){
	    //printf("gw: <%s>\n", last);
	    while((gw = strtok_r(NULL, seps, &last)) != NULL)
                printf("gw:<%s>\n", gw);
	}
	else
	    printf("cannot  recognize %s\n", type);

	count ++;
    }
    fclose(cfg);
    return count;
}

int main(){
    char * dir = getenv("HOME");
    char path[100];
    sprintf(path, "%s/.ccnx/nac.conf", getenv("HOME"));
    read_config(path);
}
