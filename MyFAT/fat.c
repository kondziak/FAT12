#include "fat.h"
#define ROOT_PATH "/root"
FILE *file = NULL;
type_t cattegory;

void printTimeAndDate(uint16_t date, uint16_t time)
{
    uint8_t day,month,second = 0,minute = 0,hour = 0;
    uint16_t year = 0;
    
    hour |= (time >> 11) & ((1 << 5)-1);
    hour += 1;
    minute |= (time >> 5) & ((1 << 6)-1);
    minute +=1;
    second |= time & ((1 << 5)-1);
    second = second * 2 + 1;
    
    for(int i = 0; i < 5; i++){
        day |= date & (1 << i);
    }
    
    for(int i = 0; i < 4; i++){
        month |= (date >> 5) & (1 << i);
    }
    
    for(int i = 0; i < 7; i++){
        year |= (date >> 9) & (1 << i);
    }
    year += 1980;
    
    printf(" %02u:%02u:%02u\n",hour,minute,second);
    printf(" %02u-%02u-%04u\n",day,month,year);
    
}

void FAT_open(FileAllocationTable_t *table,const char * name)
{
    assert(table != NULL);
    assert(name != NULL);

    file = fopen(name,"rb+");
    assert(file != NULL);
    
    int bytes = sizeof(bootSector_t);
    
    int count = readblock(&table->sector,0,bytes);
    assert(count == 1);
    
    int check = table->sector.field.block.sectors_per_cluster;
    bool flag = false;
    for(int i = 1; i <= 128; i = i*2){
        if(check == i){
            flag = true;
        }
    }
    assert(flag == true);
    
    assert(table->sector.field.block.reserved_sectors > 0);
    
    check = table->sector.field.block.fat_count;
    assert(check == 1 || check == 2);
    
    assert((table->sector.field.block.logical_sectors16 == 0) ^ (table->sector.field.block.logical_sectors32 == 0));
    
    assert(table->sector.field.block.root_dir_capacity * sizeof(bootSector_t) % table->sector.field.block.bytes_per_sector == 0);
    
    assert(!(table->sector.field.block.logical_sectors16 != 0 && table->sector.field.block.logical_sectors32 > 65535));
    
    table->data.entries_per_cluster = table->sector.field.block.bytes_per_sector * table->sector.field.block.sectors_per_cluster;
    table->data.entries_per_cluster /= sizeof(root_directory_t); 
    
    table->data.table_position = table->sector.field.block.reserved_sectors; //Liczba sektorów przed 1 FAT
    table->data.tableInBytes = table->data.table_position * table->sector.field.block.bytes_per_sector;
    
    table->data.root_position = table->sector.field.block.fat_count * table->sector.field.block.sectors_per_fat; // ILOŚĆ Tablic · ilość sektorów na fat
    table->data.root_position += table->data.table_position;
    
    table->data.rootInBytes = table->data.root_position * table->sector.field.block.bytes_per_sector;
    
    table->data.clusters_position = (sizeof(root_directory_t) * table->sector.field.block.root_dir_capacity)/ table->sector.field.block.bytes_per_sector;
    table->data.clusters_position += table->data.root_position;
    
    table->data.clustersInBytes = table->data.clusters_position * table->sector.field.block.bytes_per_sector;
    
    table->data.End = table->sector.field.block.logical_sectors16;
    
    table->data.actual_position = table->data.root_position;
    
    strcpy(table->data.filePath,ROOT_PATH);
    strcat(table->data.filePath,"\0");
    table->data.tableSize = table->sector.field.block.sectors_per_fat * table->sector.field.block.bytes_per_sector;
    table->table = (uint8_t*) calloc(sizeof(uint8_t),table->data.tableSize);
    assert(table->table != NULL);
    
    count = readblock(table->table,table->data.tableInBytes,table->sector.field.block.sectors_per_fat * bytes);
    assert(count == table->sector.field.block.sectors_per_fat);
    
    uint32_t offset = table->data.tableInBytes;
    uint8_t *arr;
    
    for(int i = 1; i < table->sector.field.block.fat_count; i++){
        arr = (uint8_t*) calloc(sizeof(uint8_t),table->data.tableSize);
        assert(arr != NULL);
        
        offset =+ table->sector.field.block.sectors_per_fat * table->sector.field.block.bytes_per_sector;
        count = readblock(arr,offset,table->sector.field.block.sectors_per_fat * bytes);
        assert(count == 1);
        
        count = memcmp(table->table, arr, table->data.tableSize);
        assert(count == 0);
        free(arr);
        arr = NULL;
    }

}
void FAT_close(FileAllocationTable_t * table)
{
    if(table != NULL){
        if(table->table != NULL){
            free(table->table);
            table->table = NULL;
        }
        if(file != NULL)
            fclose(file);
    }
}
void printField_debug(BIOS_parameterBlock_t block)
{
    printf(" FAT Debug");
    printf("\n Bytes per sector : %d\n Sectors per cluster :%d\n ",block.bytes_per_sector,block.sectors_per_cluster);
    printf("Reserved sectors: %d\n Number of file allocation tables: %d\n ",block.reserved_sectors,block.fat_count);
    printf("Maximum number of root directory entries: %d\n Sectors per FAT: %d\n ",block.root_dir_capacity,block.sectors_per_fat);
}
void printData_debug(data_t data)
{
    printf("Actual position: %d\n Root position: %d\n Table position: %d\n ",data.actual_position,data.root_position,data.table_position);
    printf("Clusters position: %d\n Entries_per_cluster: %d\n End: %d\n ",data.clusters_position,data.entries_per_cluster,data.End);
}

void printRoot_debug(FileAllocationTable_t *FAT)
{
    root_directory_t temp;
    for(int i = 0; i < FAT->sector.field.block.root_dir_capacity; i++){
        fread(&temp,sizeof(root_directory_t),1,file);
        if(temp.DOS_file_name[0] == NOT_USED ) break;
        else if(temp.DOS_file_name[0] == REMOVED || temp.DOS_file_name[0] == 0x2E) continue;
        else if(temp.attribute & LONG_FILE_NAME || temp.attribute & VOLUME_ID) continue;
        else if(temp.attribute & DIRECTORY) printf("<DIRECTORY>\n");
        changeFormat(temp.DOS_file_name);
        printf(", length = %u %u \n",temp.fileSize,temp.FirstClusterNum);
        printTimeAndDate(temp.modificationDate,temp.modificationTime);
    }
    fseek(file,FAT->data.root_position * FAT->sector.field.block.bytes_per_sector,SEEK_SET);
}
void printDirectory_debug(FileAllocationTable_t *FAT)
{
    uint32_t pos = FAT->data.actual_position;
    char path[200];
    strcpy(path,FAT->data.filePath);
    path[strlen(path)] = 0;
    if(FAT->data.actual_position == FAT->data.root_position){
        printRoot_debug(FAT);
    }
    else{
        uint16_t begin = get_position(SECTOR_TO_CLUSTER,FAT->data.actual_position,*FAT);
        fseek(file,FAT->data.actual_position * FAT->sector.field.block.bytes_per_sector,SEEK_SET);
        uint16_t next = calculate_next(begin,FAT);
        printDir_debug(FAT);
        while((next != DOESNT_EXIST && next != BROKEN &&  next <= LAST_CHAIN)){
            printDir_debug(FAT);
            int offset = get_position(CLUSTER_TO_SECTOR,next,*FAT);
            fseek(file,offset * FAT->sector.field.block.bytes_per_sector,SEEK_SET);
        }
    }
    FAT->data.actual_position = pos;
    int i = 0;
    for(; i < strlen(path);i++){
        FAT->data.filePath[i] = path[i];
    }
    FAT->data.filePath[i] = 0;
}

uint16_t calculate_next(uint16_t cluster,FileAllocationTable_t *FAT)
{
    if(cluster % 2){
        int OlderValue = cluster + (cluster-3)/2 + 1;
        int ElderValue = OlderValue+1;
        return (((uint16_t)((FAT->table[ElderValue]) << 4)) | ((FAT->table[OlderValue] >> 4) & 0x0f));
    }
    int OlderValue = (cluster*3)/2;
    int ElderValue = OlderValue+1;
    return ((uint16_t)((FAT->table[ElderValue] & 0x0f) << 8) | (FAT->table[OlderValue]));
} 

void printDir_debug(FileAllocationTable_t *FAT)
{
    root_directory_t temp;
    for(int i = 0; i < FAT->data.entries_per_cluster; i++){
        int result = fread(&temp,sizeof(root_directory_t),1,file);
        if(result != 1) break;
        if(temp.DOS_file_name[0] == NOT_USED) break;
        else if(temp.DOS_file_name[0] == REMOVED || temp.DOS_file_name[0] == 0x2E  || temp.DOS_file_name[0] < 0x0) continue;
        else if(temp.attribute & LONG_FILE_NAME || temp.attribute & VOLUME_ID) continue;
        else if(temp.attribute & DIRECTORY) printf("<DIRECTORY>\n");
        changeFormat(temp.DOS_file_name);
        printf(", length = %u %u \n",temp.fileSize,temp.FirstClusterNum);
        printTimeAndDate(temp.modificationDate,temp.modificationTime);
    }
}

void changeFormat(char *ptr)
{
    char *temp = ptr;
    for(int i = 0; i < 8 && *temp != ' '; i++){
        printf("%c",*temp++);
    }
    if(*(ptr + 8) == ' ' && *(ptr + 9) == ' ' && *(ptr + 10) == ' ')return;
    putchar('.');
    temp = (ptr + 8);
    for(int i = 0; i < 3 && *temp != ' '; i++){
        printf("%c",*temp++);
    }
}

root_directory_t* TryToFindDirectory(FileAllocationTable_t *FAT,const char *name,root_directory_t *directory, uint32_t num)
{
    int check = 0,flag = 0;
    char fileName[13];
    for(int i = 0; i < num; i++){
        assert(fread(directory,sizeof(root_directory_t),1,file) == 1);
        if(directory->DOS_file_name[0] == 0x0) break;
        else if(directory->DOS_file_name[0] == 0xE5 ) continue;
        else if(!(directory->attribute & DIRECTORY) && !(directory->attribute & ARCHIVE)) continue;
        
        int j = 0;
        for(; j < 8 && directory->DOS_file_name[j] != ' ';j++)
                fileName[j] = directory->DOS_file_name[j];
        if(! (directory->attribute & DIRECTORY)){
            fileName[j++] = '.';
        }
        for(int k = 0; k < 3 && directory->DOS_file_name[8 + k] != ' '; k++){
            fileName[j++] = directory->DOS_file_name[8 + k];
        }
        fileName[j] = 0;
        if(strlen(fileName) == strlen(name)){
            for(int j = 0; j < strlen(fileName); j++){
                if(tolower(fileName[j]) == tolower(name[j])) check++;
            }
            if(check == strlen(fileName)){
                flag = 1;
            }
            
        }
        if(flag > 0) break;
              
    }
    if(flag == 0) return NULL;
    return directory;
    
}
void rootInfo(FileAllocationTable_t *FAT)
{
    uint16_t start = FAT->data.actual_position;
    
    root_directory_t directory;
    uint16_t counter = 0;
    for(uint16_t i = 0; i < FAT->sector.field.block.root_dir_capacity; i++){
        readblock(&directory,FAT->data.rootInBytes + i*sizeof(root_directory_t),sizeof(root_directory_t));
        if(directory.DOS_file_name[0] == 0x0) break;
        else if(directory.DOS_file_name[0] == 0xE5) continue;
        else if(directory.attribute & LONG_FILE_NAME || directory.attribute & VOLUME_ID) continue;
        counter++;
        
    }
    
    printf(" Number of entries: %u\n Root dir capacity: %u\n",counter,FAT->sector.field.block.root_dir_capacity);
    printf(" Root filled in %lf%c\n",(double)(counter)/(double)FAT->sector.field.block.root_dir_capacity*100,'%');
    fseek(file,start * FAT->sector.field.block.bytes_per_sector,SEEK_SET);
}
void spaceInfo(FileAllocationTable_t* FAT)
{
   uint16_t taken = 0, free = 0, broken = 0, ending = 0;
   uint16_t l,r;
   for(int i = 2 ; i < FAT->sector.field.block.sectors_per_fat * FAT->sector.field.block.bytes_per_sector/3; i+=2){
       l = calculate_next(i,FAT);
       r = calculate_next(i+1,FAT);
       if(l == 0x000) free++;
       if(r == 0x000) free++;
       if(l >= 0x002 && l <=0xff6) taken++;
       if(r >= 0x002 && r <=0xff6) taken++;
       if(l == 0xff7) broken++;
       if(r == 0xff7) broken++;
       if(l >= 0xff8 && l <= 0xfff) ending++;
       if(r >= 0xff8 && r <= 0xfff) ending++;
   }
   
   printf("Free clusters:%u\nTaken clusters:%u\nBroken clusters:%u\n",free,taken,broken);
   printf("Ending clusters:%u\nSize of Cluster:%u sectors which in bytes equals = %u\n",ending,FAT->sector.field.block.sectors_per_cluster,
   FAT->sector.field.block.bytes_per_sector * FAT->sector.field.block.sectors_per_cluster);
   
}
void fileInfo(FileAllocationTable_t* FAT, char *name)
{
    int check = 0,flag = 0,size = 0;
    uint16_t start = FAT->data.actual_position;
    root_directory_t root;
    char fileName[13];
    int num = FAT->data.actual_position == FAT->data.root_position ? FAT->sector.field.block.root_dir_capacity : FAT->data.entries_per_cluster;
    for(int i = 0; i < num ;i++){
        assert(fread(&root,sizeof(root_directory_t),1,file) == 1);
        if(root.DOS_file_name[0] == 0x0) break;
        else if(root.DOS_file_name[0] == 0xE5 ) continue;
        int j = 0;
        for(; j < 8 && root.DOS_file_name[j] != ' ';j++)
                fileName[j] = root.DOS_file_name[j];
        
        if(!(root.attribute & DIRECTORY)){
            fileName[j++] = '.';
            for(int k = 0; k < 3 && root.DOS_file_name[8 + k] != ' '; k++){
                fileName[j++] = root.DOS_file_name[8 + k];
            }
        }
        fileName[j] = 0;
        if(strlen(fileName) == strlen(name)){
            for(int j = 0; j < strlen(fileName); j++){
                if(tolower(fileName[j]) == tolower(name[j])) size++;
            }
            if(size == strlen(name)){
                flag = 1;
                check = SUCCESS;
            }
            size = 0;
        }
        if(flag > 0) {break;}
        check = ERROR;
    }
    if(check == ERROR) return;

    printf("Full name:%s/%s",FAT->data.filePath,name);
    printf("\n Attributes:");
    (root.attribute & READ_ONLY) ? printf("R+,") : printf("R-,");
    (root.attribute & HIDDEN) ? printf("H+,") : printf("H-,");
    (root.attribute & SYSTEM) ? printf("S+,") : printf("S-,");
    (root.attribute & VOLUME_ID) ? printf("V+,") : printf("V-,");
    (root.attribute & DIRECTORY) ? printf("D+,") : printf("D-,");
    (root.attribute & ARCHIVE) ? printf("A+") : printf("A-");
    printf("\n File size: %u\n Last Modification:",root.fileSize);
    printTimeAndDate(root.modificationDate,root.modificationTime);
    printf("\n Last Access:");
    printTimeAndDate(root.lastAcessedDate,root.modificationTime);
    printf("\n Created in:");
    printTimeAndDate(root.creationTime,root.File_creationTime);
    printf("\n");
    uint16_t cluster = root.FirstClusterNum;
    uint16_t clustersNUM = 1;
    printf(" Clusters: [%u]",cluster);
    cluster = calculate_next(cluster,FAT);
    while(cluster >= 0x002 && cluster <= 0xff6){
        printf(" %u",cluster);
        clustersNUM++;
        cluster = calculate_next(cluster,FAT);
    }
    printf("\n Number of clusters: %u \n",clustersNUM);
    fseek(file,start * FAT->sector.field.block.bytes_per_sector,SEEK_SET);
    FAT->data.actual_position = start;
}


char* AllLower(char *name)
{
    for(int i = 0; *(name + i); i++)
        if(*(name + i) >= 'A' && *(name + i) <= 'Z') *(name + i) = tolower(*(name + i));
    return name;
}
size_t readblock(void *buffer,uint32_t first_block,size_t block_count)
{
    if(!buffer || !block_count){
        return ERROR;
    }
    fseek(file,first_block,SEEK_SET);
    int result = fread(buffer,block_count,1,file);
    return result;
}
int cd(FileAllocationTable_t *FAT,const char *name)
{
    assert(FAT != NULL && name != NULL);
    
    if(strncmp(name,"..",2) == 0 && FAT->data.actual_position == FAT->data.root_position)
        return SUCCESS;
    if((strncmp(name,ROOT_PATH,5) == 0  && strlen(name) == 5)|| strncmp(name,"~",1) == 0){
        FAT->data.actual_position = FAT->data.root_position;
        FAT->data.filePath[5]=0;
        fseek(file,FAT->data.rootInBytes,SEEK_SET);
        return SUCCESS;
    }
    
    if(strncmp(name,"..",2) == 0 && FAT->data.actual_position != FAT->data.root_position){
        char *ptr = strrchr(FAT->data.filePath,'/');
        char newPath[200];
        int i = 0;
        for(;(FAT->data.filePath + i) != ptr; i++){
            newPath[i] = *(FAT->data.filePath + i);
        }
        newPath[i] = 0;
        return cd(FAT,newPath);
    }
    
    
    if(strchr(name,'/') == NULL){
        if(FAT->data.actual_position == FAT->data.root_position){
            root_directory_t temp;
            root_directory_t *root = &temp ;
            root = TryToFindDirectory(FAT,name,root,FAT->sector.field.block.root_dir_capacity);
            if(root == NULL){
                printf("Couldn't find file\n");
                return ERROR;
            }
            FAT->data.actual_position = get_position(CLUSTER_TO_SECTOR,root->FirstClusterNum,*FAT);
            fseek(file,FAT->data.actual_position * FAT->sector.field.block.bytes_per_sector,SEEK_SET);
            strcat(FAT->data.filePath,"/");
            strcat(FAT->data.filePath,name);
            return SUCCESS;
        }
        else{
            root_directory_t temp;
            root_directory_t *root = &temp ;
            root = TryToFindDirectory(FAT,name,root,FAT->data.entries_per_cluster);
            if(root == NULL){
                printf("Couldn't find file\n");
                return ERROR;
            }
            FAT->data.actual_position = get_position(CLUSTER_TO_SECTOR,root->FirstClusterNum,*FAT);
            fseek(file,FAT->data.actual_position * FAT->sector.field.block.bytes_per_sector,SEEK_SET);
            strcat(FAT->data.filePath,"/");
            strcat(FAT->data.filePath,name);
            return SUCCESS;
        }
    }
    
    FAT->data.actual_position = FAT->data.root_position;
    fseek(file,FAT->data.rootInBytes,SEEK_SET);
    char tempArr[200];
    strcpy(tempArr,(name+5));
    strcat(tempArr,"\0");
    char *ptr = strtok(tempArr,"/");
    while(ptr != NULL){
        root_directory_t temp;
        root_directory_t * root = &temp;
        if(FAT->data.actual_position == FAT->data.root_position){
            root = TryToFindDirectory(FAT,ptr,root,FAT->sector.field.block.root_dir_capacity);
        }
        else{
            root = TryToFindDirectory(FAT,ptr,root,FAT->data.entries_per_cluster);
        }
        
        if(root == NULL){
            printf("Couldn't find file\n");
            return ERROR;
        }
        FAT->data.actual_position = get_position(CLUSTER_TO_SECTOR,root->FirstClusterNum,*FAT);
        fseek(file,FAT->data.actual_position * FAT->sector.field.block.bytes_per_sector,SEEK_SET);
        ptr = strtok(NULL,"/");
    }
    int i = 0;
    for(; i < strlen(name); i++){
        FAT->data.filePath[i] = name[i];
    }
    FAT->data.filePath[i]=0;

    return SUCCESS;
}
uint16_t get_position(type_t type,uint16_t value,FileAllocationTable_t FAT)
{
    assert(type == SECTOR_TO_CLUSTER || type == CLUSTER_TO_SECTOR);
    
    if(type == SECTOR_TO_CLUSTER){
        uint16_t returnV =  (value - FAT.data.clusters_position)/FAT.sector.field.block.sectors_per_cluster;
        return returnV+2;
    }
    uint16_t returnV = (value - 2) * FAT.sector.field.block.sectors_per_cluster + FAT.data.clusters_position;
    return returnV;
}
int cat(FileAllocationTable_t *FAT, const char *name)
{
    assert(FAT != NULL && name != NULL);
    root_directory_t temp;
    root_directory_t *root = &temp;
    int readSize = 0;
    
    if(strcmp(FAT->data.filePath,ROOT_PATH) == 0){
        root = TryToFindFile(FAT,name,root,FAT->sector.field.block.root_dir_capacity);
    }
    else{
        root = TryToFindFile(FAT,name,root,FAT->data.entries_per_cluster);
    }
    if(root == NULL){
        printf("File doesn't exist\n");
        return ERROR;
    }
    int fileSize = temp.fileSize;
    int clusterSize = FAT->sector.field.block.sectors_per_cluster * FAT->sector.field.block.bytes_per_sector;
    int start = FAT->data.actual_position;
    int character;
    int clustNum = root->FirstClusterNum;
    do{
        int offset = get_position(CLUSTER_TO_SECTOR,clustNum,*FAT);
        fseek(file,offset * FAT->sector.field.block.bytes_per_sector,SEEK_SET);
        int size = clusterSize > fileSize ? clusterSize : fileSize - readSize;
        for(int i = 0; i < size;i++){
            character = fgetc(file);
            if(character == 0) break;
            printf("%c",character);
            
        }
        if(character == 0) break;
        readSize += size;
        clustNum = calculate_next(clustNum,FAT);
    }while(clustNum >= 0x002 && clustNum <= 0xff6 && readSize <= fileSize);
    
    fseek(file,start * FAT->sector.field.block.bytes_per_sector,SEEK_SET);
    FAT->data.actual_position = start;
    return SUCCESS;
}
root_directory_t* TryToFindFile(FileAllocationTable_t *FAT,const char *name,root_directory_t *directory, uint32_t num)
{
    int check = 0,flag = 0;
    char fileName[13];
    for(int i = 0; i < num; i++){
        assert(fread(directory,sizeof(root_directory_t),1,file) == 1);
        if(directory->DOS_file_name[0] == 0x000) break;
        else if(directory->DOS_file_name[0] == 0xE5 ) continue;
        else if(directory->attribute & LONG_FILE_NAME ||(directory->attribute & VOLUME_ID && num != FAT->sector.field.block.root_dir_capacity)) continue;
        int j = 0;
        for(; j < 8 && directory->DOS_file_name[j] != ' ';j++)
                fileName[j] = directory->DOS_file_name[j];
        fileName[j++] = '.';
        for(int k = 0; k < 3 && directory->DOS_file_name[8 + k] != ' '; k++){
            fileName[j++] = directory->DOS_file_name[8 + k];
        }
        fileName[j]=0;
        if(strlen(fileName) == strlen(name)){
            for(int j = 0; j < strlen(fileName); j++){
                if(tolower(fileName[j]) == tolower(name[j])) check++;
            }
            if(check == strlen(fileName)){
                flag = 1;
            }
            check = 0;
        }
        if(flag > 0) break;
    }
    return directory;
}
int get(FileAllocationTable_t *FAT, const char *name)
{
    assert(FAT != NULL && name != NULL);
    root_directory_t temp;
    root_directory_t *root = &temp;
    int readSize = 0;
    char path[200];
    strcpy(path,FAT->data.filePath);
    strcat(path,"\0");
    if(strcmp(FAT->data.filePath,ROOT_PATH) == 0){
        root = TryToFindFile(FAT,name,root,FAT->sector.field.block.root_dir_capacity);
    }
    else{
        root = TryToFindFile(FAT,name,root,FAT->data.entries_per_cluster);
    }
    if(root == NULL){
        printf("File doesn't exist\n");
        return ERROR;
    }
    int fileSize = temp.fileSize;
    int clusterSize = FAT->sector.field.block.sectors_per_cluster * FAT->sector.field.block.bytes_per_sector;
    int start = FAT->data.actual_position;
    
    FILE *writeFile = fopen(name,"w");
    assert(writeFile != NULL);
    rewind(writeFile);
    int clustNum = root->FirstClusterNum;
    int character;
    do{
        int offset = get_position(CLUSTER_TO_SECTOR,clustNum,*FAT);
        fseek(file,offset * FAT->sector.field.block.bytes_per_sector,SEEK_SET);
        int size = clusterSize > fileSize ? clusterSize : fileSize - readSize;
        for(int i = 0; i < size;i++){
            character = getc(file);
            if(character == 0)break;
            putc(character,writeFile);
        }
        if(character == 0) break;
        readSize += size;
        clustNum = calculate_next(clustNum,FAT);
    }while(clustNum >= 0x002 && clustNum <= 0xff6 && readSize <= fileSize);
    fclose(writeFile);
    fseek(file,start * FAT->sector.field.block.bytes_per_sector,SEEK_SET);
    FAT->data.actual_position = start;
    int i = 0;
    for(;i < strlen(path); i++){
        FAT->data.filePath[i] = path[i];
    }
    FAT->data.filePath[i] = 0;
    return SUCCESS;
}
int zip(FileAllocationTable_t *FAT, const char* firstName, const char* secondName, const char* thirdName)
{
    assert(FAT != NULL && firstName != NULL && secondName != NULL && thirdName != NULL);
    
    int start = FAT->data.actual_position;
    char path[200];
    strcpy(path,FAT->data.filePath);
    strcat(path,"\0");
    root_directory_t temp1,temp2;
    root_directory_t * dir1, *dir2;
    FILE *newFile = fopen(thirdName,"w");
    
    if(strncmp(firstName,ROOT_PATH,5) != 0){
        if(FAT->data.actual_position == FAT->data.root_position){
            dir1 = TryToFindFile(FAT,firstName,&temp1,FAT->sector.field.block.root_dir_capacity);
        }
        else{
            dir1 = TryToFindFile(FAT,firstName,&temp1,FAT->data.entries_per_cluster);
        }
        if(dir1 == NULL){
            printf("Couldn't find file\n Executive process has been stopped");
            return ERROR;
        }
    }
    else{
        char *ptr = strrchr(firstName,'/');
        char path[80];
        char editedName[30];
        int i;
        for(i = 0; i < strlen(firstName); i++){
            if(((firstName + i) == ptr)) {
                path[i] = 0;
                break;
            }
            path[i] = firstName[i];
        }
        i++;
        int j;
        for(j = 0; i < strlen(firstName); j++){
            editedName[j] = firstName[i];
            i++;
        }
        editedName[j] = 0;
        int result = cd(FAT,path);
        if(result == ERROR){
            printf("Couldn't find file\n Executive process has been stopped");
            return ERROR;
        }
        if(FAT->data.actual_position == FAT->data.root_position){
            dir1 = TryToFindFile(FAT,editedName,&temp1,FAT->sector.field.block.root_dir_capacity);
        }
        else{
            dir1 = TryToFindFile(FAT,editedName,&temp1,FAT->data.entries_per_cluster);
        }
        if(!dir1){
            printf("Couldn't find file\n Executive process has been stopped");
            return ERROR;
        }
    }
    
    if(strncmp(secondName,ROOT_PATH,5) != 0){
        if(FAT->data.actual_position == FAT->data.root_position){
            dir2 = TryToFindFile(FAT,secondName,&temp2,FAT->sector.field.block.root_dir_capacity);
        }
        else{
            dir2 = TryToFindFile(FAT,secondName,&temp2,FAT->data.entries_per_cluster);
        }
        if(!dir2){
            printf("Couldn't find file\n Executive process has been stopped");
            return ERROR;
        }
    }
    else{
        char *ptr = strrchr(secondName,'/');
        char path[80];
        char editedName[30];
        int i;
        printf("%lu\n",strlen(secondName));
        for(i = 0; i < strlen(secondName); i++){
            if(((secondName + i) == ptr)) {
                path[i] = 0;
                break;
            }
            path[i] = secondName[i];
        }
        i++;
        int j;
        for(j= 0; i < strlen(secondName); j++){
            editedName[j] = secondName[i];
            i++;
        }
        editedName[j] = 0;
        int result = cd(FAT,path);
        if(result == ERROR){
            printf("Couldn't find file\n Executive process has been stopped");
            return ERROR;
        }
        if(FAT->data.actual_position == FAT->data.root_position){
            dir2 = TryToFindFile(FAT,editedName,&temp2,FAT->sector.field.block.root_dir_capacity);
        }
        else{
            dir2 = TryToFindFile(FAT,editedName,&temp2,FAT->data.entries_per_cluster);
        }
        if(dir2 == NULL){
            printf("Couldn't find file\n Executive process has been stopped");
            return ERROR;
        }
    }
    int clusterSize = FAT->sector.field.block.bytes_per_sector * FAT->sector.field.block.sectors_per_cluster;
    int read1 = 0, read2 = 0;
    int fileSize1 = temp1.fileSize, fileSize2 = temp2.fileSize;
    int character_1 = 's',character_2 = 's';
    int next1,next2,offset1,offset2;
    
    next1 = get_position(CLUSTER_TO_SECTOR,dir1->FirstClusterNum,*FAT);
    next2 = get_position(CLUSTER_TO_SECTOR,dir2->FirstClusterNum,*FAT);
    offset1 = next1  * FAT->sector.field.block.bytes_per_sector;
    offset2 = next2 * FAT->sector.field.block.bytes_per_sector;
    while(read1 <= fileSize1 && read2 <= fileSize2){
        
        if(read1 <= fileSize1 && character_1 != 0){
            fseek(file,offset1, SEEK_SET);
            character_1 = getc(file);
            while(character_1 != 0){
                read1++;
                putc(character_1,newFile);
                if(character_1 == '\n') break;
                character_1 = getc(file);
            }
            offset1 = ftell(file);
            if(clusterSize == read1){
                next1 = get_position(CLUSTER_TO_SECTOR,next1,*FAT);
                offset1 = next1 * FAT->sector.field.block.bytes_per_sector;
            }
        }
        
        if(read2 <= fileSize2 && character_2 != 0){
            fseek(file,offset2,SEEK_SET);
            character_2 = getc(file);
            while(character_2 != 0){
                read2++;
                putc(character_2,newFile);
                if(character_2 == '\n') break;
                character_2 = getc(file);
            }
            offset2=ftell(file);
            if(clusterSize == read2){
                next2 = get_position(CLUSTER_TO_SECTOR,next2,*FAT);
                offset2 = next2 * FAT->sector.field.block.bytes_per_sector;
            }
        }
        if(character_1 == 0 && character_2 == 0) break;
    }
    
    FAT->data.actual_position = start;
    fseek(file,start * FAT->sector.field.block.bytes_per_sector,SEEK_SET);
    fclose(newFile);
    int i = 0;
    for(;i < strlen(path); i++){
        FAT->data.filePath[i] = path[i];
    }
    FAT->data.filePath[i] = '\0';
    return SUCCESS;
}