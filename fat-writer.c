#include <stdio.h>
#include <unistd.h>
#include <string.h>

const int FAT_UNUSED = 65535;
const int FAT_FILE_END = 65534;
const int FAT_BAD_CLUSTER = 65533;

struct boot_record {
    char volume_descriptor[251];
    int fat_type;
    int fat_copies;
    unsigned int cluster_size;
    long root_directory_max_entries_count;
    unsigned int cluster_count;
    unsigned int reserved_cluster_count;
    char signature[4];
};

struct root_directory{
    char file_name[13];    
    char file_mod[10];
    short file_type;
    long file_size;
    unsigned int first_cluster;
};


int main(){
    
    int alen = 100;
    int blen = 300;
    int clen = 140;
    char a[alen];
    char b[blen];
    char c[blen];
    FILE *fp;    
    long rd_count;
  
    struct boot_record br;    
    unsigned int fat[4086];    
    
    char cluster_a[128];
    char cluster_b1[128];
    char cluster_b2[128];
    char cluster_b3[128];
    char cluster_c1[128];
    char cluster_c2[128];
    char cluster_empty[128];
    
    //priprava souboru
    strcpy(cluster_empty,"");
    memset(cluster_a, '\0', sizeof(cluster_a));
    memset(cluster_b1, '\0', sizeof(cluster_b1));
    memset(cluster_b2, '\0', sizeof(cluster_b2));
    memset(cluster_b3, '\0', sizeof(cluster_b3));
    memset(cluster_c1, '\0', sizeof(cluster_c1));
    memset(cluster_c2, '\0', sizeof(cluster_c2));
    strcpy(cluster_a,"0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789");        
    strcpy(cluster_b1,"Tohle je fakt dlouhy text, ktery je ulozen v souboru b.txt. Tento soubor zabira opravdu, ale opravdu velike mnozstvi mista, tak");
    strcpy(cluster_b2,"ze v nasi virtualni FATce bude zabirat vice clusteru. Jak je videt v root directory, ma delku 300, takze zabira cele 3 clustery");
    strcpy(cluster_b3," a to 2,3,4. to je videt jasne z FAT......");        
    strcpy(cluster_c1,"A tohle je soubor c.txt - je to uplne obycejny soubor, ve kterem se uplne obycejny text. Jeho zakernosti je, ze neni uplne frag");
    strcpy(cluster_c2,"mentovany...");

    //zapisu natvrdo acko
    fat[0] = FAT_FILE_END;
    struct root_directory root_a;
    memset(root_a.file_name, '\0', sizeof(root_a.file_name));
    strcpy(root_a.file_name, "a.txt");
    root_a.file_size = 100;
    root_a.file_type = 1;
    memset(root_a.file_mod, '\0', sizeof(root_a.file_mod));
    strcpy(root_a.file_mod, "rwxrwxrwx");    
    root_a.first_cluster = 0;            
        
    //zapisu natvrdo bcko
    fat[1] = 2;
    fat[2] = 3;
    fat[3] = FAT_FILE_END;
    struct root_directory root_b;
    memset(root_b.file_name, '\0', sizeof(root_b.file_name));
    strcpy(root_b.file_name, "b.txt");
    root_b.file_size = 300;
    root_b.file_type = 1;
    memset(root_b.file_mod, '\0', sizeof(root_b.file_mod));
    strcpy(root_b.file_mod, "rwxrwxrwx");
    root_b.first_cluster = 1;   

    //nejake prazdne a vadne clustery pred ceckem
    fat[4] = FAT_UNUSED;
    fat[5] = FAT_BAD_CLUSTER;

    //zapisu natvrdo cecko   
    struct root_directory root_c;
    memset(root_c.file_name, '\0', sizeof(root_c.file_name));
    strcpy(root_c.file_name, "c.txt");
    root_c.file_size = 140;
    root_c.file_type = 1;
    memset(root_c.file_mod, '\0', sizeof(root_c.file_mod));
    strcpy(root_c.file_mod, "rwxrwxrwx");
    root_c.first_cluster = 6;        
    fat[6] = 8;
    fat[7] = FAT_UNUSED;
    fat[8] = FAT_FILE_END;    
        
    //clearnu zbytek fatky          
    for (int i = 9; i<=4086; i++)
    {
        fat[i] = FAT_UNUSED;
    }

    //zapis dat - boot record
    memset(br.signature, '\0', sizeof(br.signature));
    memset(br.volume_descriptor, '\0', sizeof(br.volume_descriptor));
    strcpy(br.signature, "OK");    
    strcpy(br.volume_descriptor, "Testovaci data s trema soubory a.txt, b.txt a c.txt. Cecko NENI fragmentovane");
    br.fat_copies = 2;
    br.fat_type = 12;
    br.cluster_size = 128;
    br.cluster_count = 4086;
    br.reserved_cluster_count = 10;
    br.root_directory_max_entries_count = 3;
    
    unlink("output.fat");
    fp = fopen("output.fat", "w");
    //boot record
    fwrite(&br, sizeof(br), 1, fp);
    // 2x FAT
    fwrite(&fat, sizeof(fat), 1, fp);
    fwrite(&fat, sizeof(fat), 1, fp);
    // root directory
    fwrite(&root_a, sizeof(root_a), 1, fp);
    fwrite(&root_b, sizeof(root_b), 1, fp);
    fwrite(&root_c, sizeof(root_c), 1, fp);
    // clustery - data
    fwrite(&cluster_a, sizeof(cluster_a), 1, fp);                             //cluster 0
    fwrite(&cluster_b1, sizeof(cluster_b1), 1, fp);                           //cluster 1
    fwrite(&cluster_b2, sizeof(cluster_b2), 1, fp);                           //cluster 2
    fwrite(&cluster_b3, sizeof(cluster_b3), 1, fp);                           //cluster 3
    fwrite(&cluster_empty, sizeof(cluster_empty), 1, fp);                     //cluster 4
    fwrite(&cluster_empty, sizeof(cluster_empty), 1, fp);                     //cluster 5
    fwrite(&cluster_c1, sizeof(cluster_c1), 1, fp);                           //cluster 6
    fwrite(&cluster_empty, sizeof(cluster_empty), 1, fp);                     //cluster 7
    fwrite(&cluster_c2, sizeof(cluster_c2), 1, fp);                           //cluster 8
    //vynuluj zbytek datovych bloku
    for (int i=9; i<4076; i++)
        fwrite(&cluster_empty, sizeof(cluster_empty), 1, fp);
    fclose(fp);   
    return 0;
    
    
}// End Of main