#include <stdio.h>
#include <string.h>

void reverse_str(char* str){
    int length = strlen(str),i=0;

    char rev_str[length+1];
    rev_str[length] = '\0';

    for(i=0;i<length;i++){
        rev_str[i] = str[length-i-1];
    }
    
    printf("Length : %d\n",length);
    printf("Reverse : %s\n",rev_str);
}

void split(char* ori_str, char* p){
    printf("Before tok : %s\n",ori_str);

    printf("After tok :\n");

    char* str;
    str = strtok(ori_str,p);
    while(str != NULL){
        printf("%s\n",str);
        str = strtok(NULL,p);
    }
}

int main(int argc,char* argv[]){
    if(argc <= 1)
        return 0;

    char ori_str[] = "123451234512345";
    reverse_str(ori_str);
    split(ori_str,argv[1]);


    return 0;
}
