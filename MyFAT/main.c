#include <stdio.h>
#include <string.h>
#include "fat.h"
void operateOnFAT(FileAllocationTable_t FAT,char command[]){
    while(strcmp(AllLower(command),"exit") != 0){
        printf("%s :~# ",FAT.data.filePath);
        fgets(command,199,stdin);
        command[strlen(command)-1] = 0;
        if(strcmp(command,"pwd") == 0){
            printf(" Filepath: %s\n",FAT.data.filePath);
        }
        else if(strncmp(command,"cd ",3) == 0){
            cd(&FAT,command+3);
        }
        else if(strcmp(command,"rootinfo") == 0){
            rootInfo(&FAT);
        }
        else if(strcmp(command,"spaceinfo") == 0){
            spaceInfo(&FAT);
        }
        else if(strncmp(command,"fileinfo ",9) == 0){
            fileInfo(&FAT,command+9);
        }
        else if(strcmp(command,"dir") == 0){
            printDirectory_debug(&FAT);
        }
        else if(strncmp(command,"cat ",4) == 0){
            cat(&FAT,command+4);
            putchar('\n');
        }
        else if(strncmp(command,"get ",4) == 0){
            get(&FAT,command+4);
        }
        else if(strncmp(command,"zip ",4) == 0){
            
            char name1[200],name2[200],name3[200];
            int i = 0, j = 0, k = 0;
            char *ptr = (command + 4);
            if(strncmp(command+4,"/root",5) != 0){
                for(; *(ptr + i) != ' '; i++){
                    name1[i] = *(ptr+i);
                }
                name1[i] = 0;
                i++;
                ptr = command + 4 + i;
            }
            else{
                while(*(ptr + i) != ' ') {name1[i] = *(ptr+i);i++;}
                name1[i]= 0;
                ptr = command + 4 + i + 1 ;
            }
            if(strncmp(ptr,"/root",5) != 0){
                for(;*(ptr + j) != ' ';j++){
                    name2[j] = *(ptr + j);
                }
                name2[j] = 0;
                j++;
                ptr = ptr + j;
            }
            else{
                while(*(ptr + j) != ' ') {name2[j] = *(ptr+j);j++;}
                name2[j] = 0;
                ptr = ptr + j + 1;
            }
            for(; *(ptr + k); k++){
                name3[k] = *(ptr + k);
            }
            name3[k] = 0;
            zip(&FAT,name1,name2,name3);
        }
    }
}
int main(int argc, char **argv)
{
    if(argc < 2){
        return printf("Pass more arguments"),1;
    }    
	FileAllocationTable_t FAT;
    char command[200] = "No option";
    FAT_open(&FAT,argv[1]);
    FAT.data.actual_position = FAT.data.root_position;
    fseek(file,FAT.data.rootInBytes,0);
    strcpy(FAT.data.filePath,"/root");
    strcat(FAT.data.filePath,"\0");
    operateOnFAT(FAT,command);
    FAT_close(&FAT);
    
	return 0;
}
