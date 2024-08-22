#include<regex.h>
#include<ctype.h>
#include<stdio.h>
#include<stdlib.h>

#define LENGTH 100

int checkfrom(char * buf){
    const char *pattern = "^\\s*From\\s*:\\s*[a-zA-Z0-9_.]+@[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+\\s*$";
    regex_t regex;
    if (regcomp(&regex, pattern, REG_EXTENDED /*| REG_ICASE*/) != 0) {
        fprintf(stderr, "Error compiling regular expression\n");
        return -1;
    }
    int match = regexec(&regex, buf, 0, NULL, 0);
    printf("Match in checkfrom : %d\n", match);
    if (match == 0){
        printf("Valid email format: %s\n", buf);
        return 1;
    } else{
        printf("Invalid email format: %s\n", buf);
        return 0;
    }
    regfree(&regex);
}

int main(){
    char* buff;
    buff = "From:user1@127.0.0.1";
    int i=checkfrom(buff);
    printf("value returned is %d",i);
    return 0; 

}