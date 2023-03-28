#include "../lib-header/stdtype.h"
#include "../lib-header/fat32.h"
#include "../lib-header/stdmem.h"

static struct FAT32DriverState driver_state;

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '3', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};

/* -- Driver Interfaces -- */

/**
 * Convert cluster number to logical block address
 * 
 * @param cluster Cluster number to convert
 * @return uint32_t Logical Block Address
 */
uint32_t cluster_to_lba(uint32_t cluster){
    // dr nanobyte -> lba = data_region_begin + (cluster-2)*sector_per_cluster
    return cluster * CLUSTER_BLOCK_COUNT;
}   

/**
 * Initialize DirectoryTable value with parent DirectoryEntry and directory name
 * 
 * @param dir_table          Pointer to directory table
 * @param name               8-byte char for directory name
 * @param parent_dir_cluster Parent directory cluster number
 */
void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster){
    struct FAT32DirectoryEntry *new_entry = &(dir_table->table[0]);
    memcpy(&(new_entry->name), name, 8);
    new_entry->attribute = ATTR_SUBDIRECTORY;
    new_entry->user_attribute = UATTR_NOT_EMPTY;

    new_entry->undelete = 0;
    new_entry->create_time = 0;
    new_entry->create_date = 0;
    new_entry->access_date = 0;

    new_entry->cluster_high = (uint16_t)(parent_dir_cluster >> 16);

    new_entry->modified_time = 0;
    new_entry->modified_date = 0;

    new_entry->cluster_low = (uint16_t)(parent_dir_cluster & 0xFFFF);
    new_entry->filesize = 0;
    
    dir_table->table[0] = *new_entry;
}

/**
 * Checking whether filesystem signature is missing or not in boot sector
 * 
 * @return True if memcmp(boot_sector, fs_signature) returning inequality
 */
bool is_empty_storage(void){
    // read boot sector 
    uint8_t boot_sector[BLOCK_SIZE];
    read_blocks(boot_sector, BOOT_SECTOR, 1);
    return memcmp(boot_sector, fs_signature, BLOCK_SIZE) != 0;
}

/**
 * Create new FAT32 file system. Will write fs_signature into boot sector and 
 * proper FileAllocationTable (contain CLUSTER_0_VALUE, CLUSTER_1_VALUE, 
 * and initialized root directory) into cluster number 1
 */
void create_fat32(void){
    write_blocks(fs_signature, BOOT_SECTOR, 1);

    driver_state.fat_table.cluster_map[0] = CLUSTER_0_VALUE;
    driver_state.fat_table.cluster_map[1] = CLUSTER_1_VALUE;
    driver_state.fat_table.cluster_map[2] = FAT32_FAT_END_OF_FILE;

    write_clusters(&driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
    
    init_directory_table(&(driver_state.dir_table_buf), "ROOT\0\0\0\0", ROOT_CLUSTER_NUMBER);
    write_clusters(&(driver_state.dir_table_buf), ROOT_CLUSTER_NUMBER, 1);  
}

/**
 * @brief 
 * 
 *
 * 
 *
 * Else, read and cache entire FileAllocationTable (located at cluster number 1) into driver state
 */
void initialize_filesystem_fat32(void){
    if (is_empty_storage()){
        create_fat32();
    } else {
        read_clusters(&(driver_state.fat_table), FAT_CLUSTER_NUMBER, 1);
        read_clusters(&(driver_state.dir_table_buf), ROOT_CLUSTER_NUMBER, 1);        
    }
}

/**
 * Write cluster operation, wrapper for write_blocks().
 * Recommended to use struct ClusterBuffer
 * 
 * @param ptr            Pointer to source data
 * @param cluster_number Cluster number to write
 * @param cluster_count  Cluster count to write, due limitation of write_blocks block_count 255 => max cluster_count = 63
 */
void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count){
    write_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
}

/**
 * Read cluster operation, wrapper for read_blocks().
 * Recommended to use struct ClusterBuffer
 * 
 * @param ptr            Pointer to buffer for reading
 * @param cluster_number Cluster number to read
 * @param cluster_count  Cluster count to read, due limitation of read_blocks block_count 255 => max cluster_count = 63
 */
void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count){
    read_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
}

/* -- CRUD Operation -- */

/**
 * @brief 
 * 
 *
 *
 * @param request buf point to struct FAT32DirectoryTable,
 *                name is directory name,
 *                ext is unused,
 *                parent_cluster_number is target directory table to read,
 *                buffer_size must be exactly sizeof(struct FAT32DirectoryTable)
 * @return Error code: 0 success - 1 not a folder - 2 not found - -1 unknown
 */
int8_t read_directory(struct FAT32DriverRequest request){
    int8_t error_code = -1;

    struct FAT32DirectoryTable parent_table;
    read_clusters(&parent_table, request.parent_cluster_number, 1);

    for(uint32_t i = 0; i < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry); i++) {
        if(memcmp(request.name, parent_table.table[i].name, 8) == 0) {
            if (parent_table.table[i].attribute == ATTR_SUBDIRECTORY) {
                // terbaca sebagai folder
                error_code = 0;
                uint32_t curr_cluster = parent_table.table[i].cluster_high << 16 & parent_table.table[i].cluster_low;

                struct FAT32DirectoryTable curr_dir;
                read_clusters(&curr_dir, curr_cluster, 1);
                memcpy(request.buf, &curr_dir, sizeof(struct FAT32DirectoryTable));
                break;
            } else {
                error_code = 1;
            }
        } else {
            // TODO: implement unknown 
            error_code = 2;
        }
    }
    return error_code;
}


/**
 * FAT32 read, read a file from file system.
 *
 * @param request All attribute will be used for read, buffer_size will limit reading count
 * @return Error code: 0 success - 1 not a file - 2 not enough buffer - 3 not found - -1 unknown
 */ 
int8_t read(struct FAT32DriverRequest request){
    struct FAT32DirectoryTable parent_table;
    read_clusters(&parent_table, request.parent_cluster_number, 1);

    for(uint32_t i = 0; i < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry); i++) {
        
    }
    // for (uint32_t i = 0; i < .buffer_size; i++){
    //     if (memcmp(request.name, , 8) == 0){
    //         // found
    //         if (request.buf[i].attributes & FAT32_ATTR_DIRECTORY){
    //             // not a file
    //             return 1;
    //         } else {
    //             // read file
    //             read_clusters(request.buf, request.buf[i].first_cluster, request.buffer_size);
    //             return 0;
    //         }
    //     }
    // }
    // return 3;
    
}

/**
 * FAT32 write, write a file or folder to file system.
 *
 * @param request All attribute will be used for write, buffer_size == 0 then create a folder / directory
 * @return Error code: 0 success - 1 file/folder already exist - 2 invalid parent cluster - -1 unknown
 */
int8_t write(struct FAT32DriverRequest request){
    struct FAT32DirectoryTable parent_table;
    read_clusters(&parent_table, request.parent_cluster_number, 1);

    uint32_t idx_empty = 0;
    for(uint32_t i = 0; i < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry); i++) {
        if(parent_table.table[i].attribute == 0x0) {
            idx_empty = i;
        }
        // TODO: Implement full directory table exception
    }

    if(request.buffer_size == 0) {
        // Create a directory
        memcpy(parent_table.table[idx_empty].name, request.name, 8);
        parent_table.table[idx_empty].attribute = ATTR_SUBDIRECTORY;
        parent_table.table[idx_empty].user_attribute = UATTR_NOT_EMPTY;

        parent_table.table[idx_empty].undelete = 0;
        parent_table.table[idx_empty].create_time = 0;
        parent_table.table[idx_empty].create_date = 0;
        parent_table.table[idx_empty].access_date = 0;

        parent_table.table[idx_empty].cluster_high = (uint16_t)(request.parent_cluster_number >> 16);

        parent_table.table[idx_empty].modified_time = 0;
        parent_table.table[idx_empty].modified_date = 0;

        parent_table.table[idx_empty].cluster_low = (uint16_t)(request.parent_cluster_number & 0xFFFF);
        parent_table.table[idx_empty].filesize = 0;

        // TODO: Implement write to FAT
        
    } else {
        memcpy(parent_table.table[idx_empty].name, request.name, 8);
        memcpy(parent_table.table[idx_empty].ext, request.ext, 3);

        parent_table.table[idx_empty].attribute = ATTR_ARCHIVE;
        parent_table.table[idx_empty].user_attribute = UATTR_NOT_EMPTY;

        parent_table.table[idx_empty].undelete = 0;
        parent_table.table[idx_empty].create_time = 0;
        parent_table.table[idx_empty].create_date = 0;
        parent_table.table[idx_empty].access_date = 0;

        parent_table.table[idx_empty].cluster_high = (uint16_t)(request.parent_cluster_number >> 16);

        parent_table.table[idx_empty].modified_time = 0;
        parent_table.table[idx_empty].modified_date = 0;

        parent_table.table[idx_empty].cluster_low = (uint16_t)(request.parent_cluster_number & 0xFFFF);
        parent_table.table[idx_empty].filesize = 0;

        // TODO: Implement write to FAT
    }
}


/**
 * FAT32 delete, delete a file or empty directory (only 1 DirectoryEntry) in file system.
 *
 * @param request buf and buffer_size is unused
 * @return Error code: 0 success - 1 not found - 2 folder is not empty - -1 unknown
 */
int8_t delete(struct FAT32DriverRequest request){
    struct FAT32DirectoryTable parent_table;
    read_clusters(&parent_table, request.parent_cluster_number, 1);

    uint8_t found = 0;
    uint32_t idx_find = 0;
    // if (request.)
    for(uint32_t i = 0; i < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry); i++) {
        // if ()
        // TODO: Implement full directory table exception
    }

}