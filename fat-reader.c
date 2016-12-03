#include <stdio.h>
#include <unistd.h>
#include <string.h>

//definice na vyznam hodnot FAT tabulky
const int FAT_UNUSED = 65535;
const int FAT_FILE_END = 65534;
const int FAT_BAD_CLUSTER = 65533;

FILE *p_file;

//struktura na boot record - nova verze
struct boot_record {
    char volume_descriptor[251];               //popis
    int fat_type;                             //typ FAT - pocet clusteru = 2^fat_type (priklad FAT 12 = 4096)
    int fat_copies;                           //kolikrat je za sebou v souboru ulozena FAT
    unsigned int cluster_size;                //velikost clusteru ve znacich (n x char) + '/0' - tedy 128 znamena 127 vyznamovych znaku + '/0'
    long root_directory_max_entries_count;    //pocet polozek v root_directory = pocet souboru MAXIMALNE, nikoliv aktualne - pro urceni kde zacinaji data - resp velikost root_directory v souboru
    unsigned int cluster_count;               //pocet pouzitelnych clusteru (2^fat_type - reserved_cluster_count)
    unsigned int reserved_cluster_count;      //pocet rezervovanych clusteru pro systemove polozky
    char signature[4];                        //pro vstupni data od vyucujicich konzistence FAT - "OK","NOK","FAI" - v poradku / FAT1 != FAT2 != FATx / FAIL - ve FAT1 == FAT2 == FAT3, ale obsahuje chyby, nutna kontrola
};

//struktura na root directory - nova verze
struct root_directory{
    char file_name[13];             //8+3 format + '/0'
    char file_mod[10];              //unix atributy souboru+ '/0'
    short file_type;                //0 = soubor, 1 = adresar
    long file_size;                 //pocet znaku v souboru 
    unsigned int first_cluster;     //cluster ve FAT, kde soubor zacina - POZOR v cislovani root_directory ma prvni cluster index 0 (viz soubor a.txt)
};


/*
Hloupoucka ctecka obsahu dat pro verifikaci 
*/

int main(){
               
    int i;
    //pointery na struktury root a boot                         
    struct boot_record *p_boot_record;
    struct root_directory *p_root_directory;      
    
    //alokujeme pamet
    p_boot_record = (struct boot_record *) malloc (sizeof (struct boot_record));
    p_root_directory = (struct root_directory *) malloc (sizeof (struct root_directory));
        
           
    //otevru soubor a pro jistotu skocim na zacatek           
    p_file = fopen("output.fat", "r");
    fseek(p_file, SEEK_SET, 0);
    
    //prectu boot
    fread(p_boot_record, sizeof(struct boot_record), 1, p_file);
    printf("-------------------------------------------------------- \n");
    printf("BOOT RECORD \n");
    printf("-------------------------------------------------------- \n");
    printf("volume_descriptor :%s\n",p_boot_record->volume_descriptor);
  	printf("fat_type :%d\n",p_boot_record->fat_type);
  	printf("fat_copies :%d\n",p_boot_record->fat_copies);
  	printf("cluster_size :%d\n",p_boot_record->cluster_size);
  	printf("root_directory_max_entries_count :%ld\n",p_boot_record->root_directory_max_entries_count);    
  	printf("cluster count :%d\n",p_boot_record->cluster_count);
  	printf("reserved clusters :%d\n",p_boot_record->reserved_cluster_count);
  	printf("signature :%s\n",p_boot_record->signature);
    
    //prectu fat_copies krat 
    printf("-------------------------------------------------------- \n");
    printf("FAT \n");
    printf("-------------------------------------------------------- \n");
    long fat_items = p_boot_record->cluster_count;
    long l;
    
    unsigned int *fat_item;
    fat_item = (unsigned int *) malloc (sizeof (unsigned int));
    int j; 
    for (j = 0; j < 2 ; j++)    
    {
        printf("\nFAT KOPIE %d\n", j + 1);
      for (l = 0; l < p_boot_record->cluster_count; l++)
      {
        fread(fat_item, sizeof(*fat_item), 1, p_file);   
          if (*fat_item != FAT_UNUSED)   
          {
            if (*fat_item == FAT_FILE_END)
                printf("%d - FILE_END\n", l);
            else if (*fat_item == FAT_BAD_CLUSTER)
                printf("%d - BAD_CLUSTER\n", l);
            else
              printf("%d - %d\n", l, *fat_item);
            
          }        
      }
    }        
    
    //prectu root tolikrat polik je maximalni pocet zaznamu v bootu - root_directory_max_entries_count        
    printf("-------------------------------------------------------- \n");
    printf("ROOT DIRECTORY \n");
    printf("-------------------------------------------------------- \n");
    
  
    for (i = 0; i < p_boot_record->root_directory_max_entries_count; i++)
    {
      fread(p_root_directory, sizeof(struct root_directory), 1, p_file);
      printf("FILE %d \n",i);
      printf("file_name :%s\n",p_root_directory->file_name); 
      printf("file_mod :%s\n",p_root_directory->file_mod);
      printf("file_type :%d\n",p_root_directory->file_type);
      printf("file_size :%d\n",p_root_directory->file_size);
      printf("first_cluster :%d\n",p_root_directory->first_cluster);
    }
    
    printf("-------------------------------------------------------- \n");
    printf("CLUSTERY - OBSAH \n");
    printf("-------------------------------------------------------- \n");
    
    char *p_cluster = malloc(sizeof(char) * p_boot_record->cluster_size);
    for (i = 0; i < 10; i++)
    {    
      fread(p_cluster, sizeof(char) * p_boot_record->cluster_size, 1, p_file);
      //pokud je prazdny (tedy zacina 0, tak nevypisuj obsah)
      if(p_cluster[0] != '\0')
        printf("Cluster %d:%s\n",i,p_cluster);
    }
    
    //uklid
    free(p_cluster);
    free(p_root_directory);
    free(p_boot_record);
    fclose(p_file);
   
    return 0;
    
    
}// End Of main
