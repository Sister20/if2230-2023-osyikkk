#include "../lib-header/stdtype.h"
#include "../lib-header/fat32.h"
#include "../lib-header/stdmem.h"
#include "../lib-header/disk.h"
// #include "../lib-header/cmos.h"

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
    memcpy(&(new_entry->ext), "\0\0\0", 3);

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
    int8_t error_code = 2;

    if (memcmp(request.name, "\0\0\0\0\0\0\0\0", 8) == 0) {
        // Folder name empty
        return error_code = -1;
    }

    if (request.buffer_size != 0) {
        // Requested data is not a folder
        return error_code = 1;
    }

    struct FAT32DirectoryTable parent_table;
    read_clusters(&parent_table, request.parent_cluster_number, 1);

    for(uint32_t i = 0; i < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry); i++) {
        if(memcmp(request.name, parent_table.table[i].name, 8) == 0) {
            if (parent_table.table[i].attribute == ATTR_SUBDIRECTORY) {
                // terbaca sebagai folder
                uint32_t curr_cluster = parent_table.table[i].cluster_high << 16 | parent_table.table[i].cluster_low;

                struct FAT32DirectoryTable curr_dir;
                read_clusters(&curr_dir, curr_cluster, 1);
                memcpy(request.buf, &curr_dir, sizeof(struct FAT32DirectoryTable));
                return error_code = 0;
            }
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
    int8_t error_code = 3;

    if(memcmp(request.name, "\0\0\0\0\0\0\0\0", 8) == 0) {
        // Empty request name error
        return error_code = -1;
    }

    if (request.buffer_size == 0) {
        // Requested data is not a folder
        return error_code = 1;
    }

    struct FAT32DirectoryTable parent_table;
    read_clusters(&parent_table, request.parent_cluster_number, 1);

    for(uint32_t i = 1; i < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry); i++) {
        if (memcmp(request.name, parent_table.table[i].name, 8) == 0 && memcmp(request.ext, parent_table.table[i].ext, 3) == 0) {
            uint32_t buffer_count = request.buffer_size/CLUSTER_SIZE;
            uint32_t cluster_number = parent_table.table[i].cluster_high << 16 | parent_table.table[i].cluster_low;

            // Check for not enough buffer
            uint32_t count = 0;
            while(cluster_number != FAT32_FAT_END_OF_FILE) {
                count++;
                cluster_number = driver_state.fat_table.cluster_map[cluster_number];
            }

            if(count > buffer_count) {
                // Not enough buffer error
                return error_code = 2;
            }

            uint32_t j = 0;
            cluster_number = parent_table.table[i].cluster_high << 16 | parent_table.table[i].cluster_low;
            while(cluster_number != FAT32_FAT_END_OF_FILE) {
                read_clusters((struct ClusterBuffer*) (request.buf) + j, cluster_number, 1);
                cluster_number = driver_state.fat_table.cluster_map[cluster_number];
                j++;
            }
            read_clusters((struct ClusterBuffer*) (request.buf) + j, cluster_number, 1);
            return error_code = 0;
        } 
    }

    return error_code;
}

/**
 * @brief 
 * 
 *
 *
 * @param request All attribute will be used for write, buffer_size == 0 then create a folder / directory
 * @return Error code: 0 success - 1 file/folder already exist - 2 invalid parent cluster - -1 unknown
 */
int8_t write(struct FAT32DriverRequest request){
    int8_t error_code = -1;

    if(memcmp(request.name, "\0\0\0\0\0\0\0\0", 8) == 0) {
        // Empty request name error
        return error_code;
    }

    // reserved cluster number
    if (request.parent_cluster_number < 2){
        return error_code = 2;
    }

    struct FAT32DirectoryTable parent_table;
    read_clusters(&parent_table, request.parent_cluster_number, 1);

    uint32_t idx_empty = 0, i = 0;
    while(i < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry)) {
        if(parent_table.table[i].user_attribute != UATTR_NOT_EMPTY) {
            idx_empty = i;
            break;
        }
        i++;
    }

    // Writes to parent directory table
    memcpy(parent_table.table[idx_empty].name, request.name, 8);
    parent_table.table[idx_empty].attribute = request.buffer_size == 0 ? ATTR_SUBDIRECTORY : ATTR_ARCHIVE;
    parent_table.table[idx_empty].user_attribute = UATTR_NOT_EMPTY;
    parent_table.table[idx_empty].undelete = 0;
    parent_table.table[idx_empty].create_time = 0;
    parent_table.table[idx_empty].create_date = 0;
    parent_table.table[idx_empty].access_date = 0;
    parent_table.table[idx_empty].modified_time = 0;
    parent_table.table[idx_empty].modified_date = 0;
    parent_table.table[idx_empty].filesize = request.buffer_size;

    if(request.buffer_size == 0) {
        // Create a new directory
        uint32_t i = ROOT_CLUSTER_NUMBER;
        uint8_t found = 0;
        while(i < CLUSTER_MAP_SIZE && !found) {
            if(driver_state.fat_table.cluster_map[i] == FAT32_FAT_EMPTY_ENTRY) {
                driver_state.fat_table.cluster_map[i] = FAT32_FAT_END_OF_FILE;
                found = 1;
            }
            i++;
        }

        if(i == CLUSTER_MAP_SIZE) {
            // Full file allocation table error
            return error_code = 1;
        }

        parent_table.table[idx_empty].cluster_high = (uint16_t) (((i-1) >> 16) & 0xFFFF);
        parent_table.table[idx_empty].cluster_low = (uint16_t) ((i-1) & 0xFFFF);
    } else {
        // Create a new file

        // Search for empty cells in file allocation table
        uint32_t i = 0, j = ROOT_CLUSTER_NUMBER;
        uint32_t empty_fat_clusters[request.buffer_size / CLUSTER_SIZE];
        while(i < request.buffer_size/CLUSTER_SIZE && j < CLUSTER_MAP_SIZE) {
            if(driver_state.fat_table.cluster_map[j] == FAT32_FAT_EMPTY_ENTRY) {
                empty_fat_clusters[i] = j;
                i++;
            }
            j++;
        }

        if(i < request.buffer_size/CLUSTER_SIZE) {
            // Full file allocation table error
            return error_code = 1;
        }

        // Create a linked list in file allocation table
        for(i = 0; i < (request.buffer_size/CLUSTER_SIZE)-1; i++) {
            driver_state.fat_table.cluster_map[empty_fat_clusters[i]] = empty_fat_clusters[i+1];
        }
        driver_state.fat_table.cluster_map[empty_fat_clusters[(request.buffer_size/CLUSTER_SIZE)-1]] = FAT32_FAT_END_OF_FILE;

        // Set parent directory table entry
        memcpy(parent_table.table[idx_empty].ext, request.ext, 3);
        parent_table.table[idx_empty].cluster_high = (uint16_t) ((empty_fat_clusters[0] >> 16) & 0xFFFF);
        parent_table.table[idx_empty].cluster_low = (uint16_t) (empty_fat_clusters[0] & 0xFFFF);

        // Write file content to storage
        if(request.buffer_size > 0) {
            for(i = 0; i < request.buffer_size/CLUSTER_SIZE; i++) {
                write_clusters(((struct ClusterBuffer*) (request.buf) + i), empty_fat_clusters[i], 1);
            }
        }
    }

    // Write updated file allocation table and parent directory table to storage
    write_clusters(&(driver_state.fat_table), FAT_CLUSTER_NUMBER, 1);
    write_clusters(&(parent_table), request.parent_cluster_number, 1);

    return error_code = 0;
}


/**
 * FAT32 delete, delete a file or empty directory (only 1 DirectoryEntry) in file system.
 *
 * @param request buf and buffer_size is unused
 * @return Error code: 0 success - 1 not found - 2 folder is not empty - -1 unknown
 */
int8_t delete(struct FAT32DriverRequest request){
    int8_t error_code = -1;

    if(memcmp(request.name, "\0\0\0\0\0\0\0\0", 8) == 0) {
        // Empty request name error
        return error_code;
    }
    struct FAT32DirectoryTable parent_table;
    read_clusters(&parent_table, request.parent_cluster_number, 1);

    for(uint32_t i = 1; i < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry); i++) {
        if (memcmp(request.name, parent_table.table[i].name, 8) == 0 && memcmp(request.ext, parent_table.table[i].ext, 3) == 0) {
            uint32_t cluster_number = parent_table.table[i].cluster_high << 16 | parent_table.table[i].cluster_low;

            if (parent_table.table[i].attribute == ATTR_SUBDIRECTORY) {
                // Delete directory

                // Get current directory table
                struct FAT32DirectoryTable curr_table;
                read_clusters(&curr_table, cluster_number, 1);

                // Check if the directory is empty
                uint8_t empty = 1;
                for(uint32_t i = 1; i < CLUSTER_SIZE/sizeof(struct FAT32DirectoryEntry); i++) {
                    if(curr_table.table[i].user_attribute == UATTR_NOT_EMPTY) {
                        empty = 0;
                        break;
                    }
                }

                if(!empty) {
                    // Directory not empty error
                    error_code = 2;
                    return error_code;
                }

                // Remove directory from storage
                struct ClusterBuffer cbuf;
                for(uint32_t i = 0; i < CLUSTER_SIZE; i++) {
                    cbuf.buf[i] = '\0';
                }
                write_clusters(&cbuf, cluster_number, 1);

                // Remove directory from file allocation table
                driver_state.fat_table.cluster_map[cluster_number] = FAT32_FAT_EMPTY_ENTRY;

                error_code = 0;
            } else if (parent_table.table[i].attribute == ATTR_ARCHIVE) {

                // Delete file
                // Remove file from file allocation table and storage
                
                struct ClusterBuffer cbuf;
                for(uint32_t i = 0; i < CLUSTER_SIZE; i++) {
                    cbuf.buf[i] = '\0';
                }
                do {
                    // Remove file from storage
                    write_clusters(&cbuf, cluster_number, 1);

                    // Remove file from file allocation table
                    uint32_t temp_cluster_number = cluster_number;
                    cluster_number = driver_state.fat_table.cluster_map[cluster_number];
                    driver_state.fat_table.cluster_map[temp_cluster_number] = FAT32_FAT_EMPTY_ENTRY;
                } while (driver_state.fat_table.cluster_map[cluster_number] != FAT32_FAT_END_OF_FILE);
                // Handle EOF cluster
                driver_state.fat_table.cluster_map[cluster_number] = FAT32_FAT_EMPTY_ENTRY;
                write_clusters(&cbuf, cluster_number, 1);

                error_code = 0;
            }

            // Remove parent directory entry
            memcpy(parent_table.table[i].name, "\0\0\0\0\0\0\0\0", 8);
            memcpy(parent_table.table[i].ext, "\0\0\0", 3);
            parent_table.table[i].attribute = 0;
            parent_table.table[i].user_attribute = 0;
            parent_table.table[i].undelete = 0;
            parent_table.table[i].create_time = 0;
            parent_table.table[i].create_date = 0;
            parent_table.table[i].access_date = 0;
            parent_table.table[i].modified_time = 0;
            parent_table.table[i].modified_date = 0;
            parent_table.table[i].filesize = 0;
            parent_table.table[i].cluster_low = 0;
            parent_table.table[i].cluster_high = 0;

            // Write updated file allocation table and parent directory table to storage
            write_clusters(&(driver_state.fat_table), FAT_CLUSTER_NUMBER, 1);
            write_clusters(&(parent_table), request.parent_cluster_number, 1);

            return error_code;
        }
    }
    return error_code;
}