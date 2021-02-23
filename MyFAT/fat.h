#ifndef FAT_H
#define FAT_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#define SIZE_AND_EXTENSION 11

typedef struct field_t field_t;
typedef struct BIOS_parameterBlock_t BIOS_parameterBlock_t;
typedef struct bootSector_t bootSector_t;
typedef struct ExtendedBIOS_parameterBlock_t ExtendedBIOS_parameterBlock_t;
typedef struct root_directory_t root_directory_t;
typedef struct data_t data_t;
typedef struct FileAllocationTable_t FileAllocationTable_t;
typedef enum type_t type_t;

extern FILE *file;

struct BIOS_parameterBlock_t{
    int16_t bytes_per_sector; // Liczba bajtów w sektorze
    uint8_t sectors_per_cluster; // Liczba sektorów w klastrze
    uint16_t reserved_sectors; // Liczba zarezerwowanych sektorów
    uint8_t fat_count; // Liczba tabel alokacji plików
    uint16_t root_dir_capacity; // Liczba wpisów w katalogu głównym woluminu
    uint16_t logical_sectors16; // Całkowita liczba sektorów w woluminie
    uint8_t mediaType; // Typ nośnika
    uint16_t sectors_per_fat; // Liczba sektorów na jedną tablicę alokacji
    uint16_t sectors_per_track; // Liczba sektorów na ścieżce
    uint16_t numberOfHeads; // Liczba ścieżek w cylindrze
    uint32_t hiddenSectorsCount; // Liczba ukrytych sektorów
    uint32_t logical_sectors32; // Całkowita liczba sektorów w woluminie
} __attribute__((packed));

struct field_t{
    int8_t jumpSection[3]; //Skok do programu ładującego
    char OEM_name[8]; // Nazwa OEM
    BIOS_parameterBlock_t block;
} __attribute__((packed));

struct ExtendedBIOS_parameterBlock_t{
    uint8_t physicalDiskNumber; // Oznaczenie numeru dysku
    uint8_t reserverd; // Zarezerwowane kiedyś numer głowicy tzw. Current Head
    uint8_t signature; // Sygnatura rozszerzonego rekordu ładującego
    uint16_t volumeSerialNumber[2]; // Unikalny numer seryjny woluminu najczęśćiej podawany w formacie %04X - %04X
    char volumeLabel[11]; // Nazwa woluminu kiedyś , teraz wpis w katalogu głównym FAT
    char SystemID[8]; // Identyfikator systemu plików
}__attribute__((packed));


struct bootSector_t{
    field_t field;
    ExtendedBIOS_parameterBlock_t block;
    uint8_t Bootstrap_code[448];
    uint16_t signature; // Sygnatura rekordu ładującego
} __attribute__((packed));

struct root_directory_t{
    char DOS_file_name[SIZE_AND_EXTENSION]; //Nazwa pliku + rozszerzenie
    uint8_t attribute; // Bajt atrybutów
    uint8_t reserved; // Zarezerwowane do użytku przez Windows NT
    uint8_t creationTime; // Dziesiątki sekund, 0 - 199
    uint16_t File_creationTime; // Czasu w którym plik został utworzony
    uint16_t File_dateTime; //Data utworzenia pliku
    uint16_t lastAcessedDate; //Ostatni raz kiedy korzystano z pliku
    uint16_t FirstClusterFAT32; // 16 bitów 1 klastra. W FACIE 12,16 zawsze 0
    uint16_t modificationTime; // Czas od ostatniej modyfikacji/utworzenia pliku
    uint16_t modificationDate; // Data od ostatniej modyfikacji/utworzenia pliku
    uint16_t FirstClusterNum; // Numer pierwszego klastra
    uint32_t fileSize; //Długość pliku w bajtach
} __attribute__((packed));

enum flags_t{
    READ_ONLY = 0x01,
    HIDDEN = 0x02,
    SYSTEM = 0x04,
    VOLUME_ID = 0x08,
    DIRECTORY = 0x10,
    ARCHIVE = 0x20,
    LONG_FILE_NAME = READ_ONLY | HIDDEN | SYSTEM | VOLUME_ID
};

struct data_t{
    char filePath[200];
    uint16_t root_position;
    uint16_t table_position;
    uint16_t clusters_position;
    uint16_t End;
    uint16_t entries_per_cluster;
    uint32_t actual_position;
    uint32_t tableSize;
    uint32_t tableInBytes;
    uint32_t clustersInBytes;
    uint32_t rootInBytes;
};

struct FileAllocationTable_t{
    uint8_t *table;
    data_t data;
    bootSector_t sector;
};

enum type_t{
    CAT,
    GET,
    NO_OPERATION,
    SECTOR_TO_CLUSTER,
    CLUSTER_TO_SECTOR
};

enum index_t{
    NOT_USED = 0x000 ,
    DOESNT_EXIST = 0x001,
    LEFT_USED = 0x002,
    RIGHT_USED = 0xFF6,
    BROKEN = 0xFF7,
    LAST_CHAIN = 0xFF8,
    REMOVED = 0xE5
};

enum error_t{
    ERROR = -1,
    SUCCESS = 0,
    NEXT_ERROR = -2
};


uint16_t calculate_next(uint16_t cluster,FileAllocationTable_t *FAT);
uint16_t get_position(type_t type,uint16_t value,FileAllocationTable_t FAT);

void printTimeAndDate(uint16_t date, uint16_t time);
void FAT_open(FileAllocationTable_t *,const char *);
void FAT_close(FileAllocationTable_t *);
void printField_debug(BIOS_parameterBlock_t block);
void printData_debug(data_t data);
void printRoot_debug(FileAllocationTable_t *FAT);
void printDirectory_debug(FileAllocationTable_t *FAT);
void printDir_debug(FileAllocationTable_t *FAT);
void changeFormat(char *ptr);
void rootInfo(FileAllocationTable_t *FAT);
void spaceInfo(FileAllocationTable_t* FAT);
void fileInfo(FileAllocationTable_t* FAT, char *name);
char* AllLower(char *name);
root_directory_t* TryToFindDirectory(FileAllocationTable_t *FAT,const char *name,root_directory_t *directory, uint32_t num);
root_directory_t* TryToFindFile(FileAllocationTable_t *FAT,const char *name,root_directory_t *directory, uint32_t num);
int cd(FileAllocationTable_t *FAT,const char *name);
int cat(FileAllocationTable_t *FAT, const char *name);
int get(FileAllocationTable_t *FAT, const char *name);
int zip(FileAllocationTable_t *FAT, const char* firstName, const char* secondName, const char* thirdName);
size_t readblock(void *buffer,uint32_t first_block,size_t block_count);

#endif