#include <errno.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <hw/ata.h>
#include <asm/io.h>
#include <mm/kheap.h>
#include <fs/skbdfs.h>


extern uint64_t fs_size;

uint8_t mode_calc(uint8_t user, uint8_t kernel) {
    return user << 3 | kernel;
}

static void __file_lock(file_t *file) {
    while (file->is_locked == 1);
    file->is_locked = 1;
}

static void __file_unlock(file_t *file) {
    file->is_locked = 0;
}

/*
    finds the next free block in the filesystem.
    returns null if there are no free blocks available

    iblk: starting block
*/
static uint32_t find_next_free_block(block_t *dest, uint32_t iblk) {
    uint64_t offset = iblk * BLOCK_SIZE;

    block_t *blk = (block_t *) kmalloc(BLOCK_SIZE);

    while (offset < fs_size) {
        file_t temp;
        temp.ptr_global = offset;

        memset(blk, 0, BLOCK_SIZE);
        ata_read_bytes(&temp, BLOCK_SIZE, (uint8_t *) blk);

        if (blk->attributes & BLOCK_FREE) {
            if (dest != NULL) {
                memcpy(dest, blk, BLOCK_SIZE);
            }
            kfree(blk);
            return offset / BLOCK_SIZE;
        }

        offset += BLOCK_SIZE;
    }

    kfree(blk);
    return 0;
}

static block_t *get_block_by_index(uint32_t iblk) {
    uint64_t offset = iblk * BLOCK_SIZE;
    block_t *blk = (block_t *) kmalloc(BLOCK_SIZE);

    file_t temp;
    temp.ptr_global = offset;

    ata_read_bytes(&temp, BLOCK_SIZE, (uint8_t *) blk);

    return blk;
}

static void read_block_into(uint32_t iblk, block_t *dst) {
    uint64_t offset = iblk * BLOCK_SIZE;
    file_t temp;
    temp.ptr_global = offset;

    ata_read_bytes(&temp, BLOCK_SIZE, (uint8_t *) dst);
}

uint32_t find_node(char *path, int type, int parent, node_t *dst) {
    char *pwd_split[16];
    memset(pwd_split, 0, sizeof(pwd_split));

    // path + 1: skip initial '/'
    path++;
    int nsplits = split_string(path, '/', pwd_split, 16);
    if (parent) nsplits--;

    block_t blk = {0};
    read_block_into(1, &blk); // read root block

    node_t node = {0};

    memcpy(&node, blk.data, sizeof(node_t));
    uint32_t *children = (uint32_t *) (blk.data + sizeof(node_t));

    int found = 0;
    uint32_t block_offset = 1;
    block_t *b;
    for (int i = 0; i < nsplits; i++) {
        found = 0;

        uint32_t node_size = node.size;
        for (int j = 0; j < node_size; j++) { // iterate through each subdirectory of the parent.
            // possible data corruption
            if (*children == 0) {
                return 0;
            }

            b = get_block_by_index(*children);

            assert(b->attributes & BLOCK_METADATA); // block has to contain a node
            memcpy(&node, b->data, sizeof(node_t));
            assert(node.magic == NODE_MAGIC);

            int target_type = (i == nsplits-1) ? type : FS_DIR;
            if (node.flags & target_type) {
                if (strcmp(node.name, pwd_split[i])) {
                    found = 1;
                    block_offset = *children;
                    memcpy(&blk, b, BLOCK_SIZE);

                    children = (uint32_t *) (blk.data + sizeof(node_t));

                    kfree(b);
                    break;
                }
            }

            kfree(b);
            children++;
        }

        // node was not found
        if (found == 0) {
            return 0;
        }
    }
    if (parent) nsplits++;

    memcpy(dst, &node, sizeof(node_t));

    return block_offset;
}

node_t *mknode(char *path, int type) {
    node_t *node = (node_t *) kmalloc(sizeof(node_t));
    uint32_t block_offset = find_node(path, FS_DIR, 1, node);
    if (block_offset == 0) {
        kfree(node);
        return NULL;
    }

    char *temp[16];
    int nsplits = split_string(path + 1, '/', temp, 16);
    char name[32];

    strcpy(name, temp[nsplits-1]);

    // for (int i = 0; i < nsplits; i++) {
    //     kfree(temp[i]);
    // }
    // kfree(temp);

    block_t *blk = get_block_by_index(block_offset);
    memcpy(node, blk->data, sizeof(node_t));

    uint32_t parent_size = node->size;
    uint32_t parent_off = node->first_block;

    block_t *block = (block_t *) kmalloc(BLOCK_SIZE);
    uint32_t r = find_next_free_block(block, block_offset);
    assert(r > 0);

    memcpy(blk->data + sizeof(node_t) + node->size * 4, &r, 4);
    node->size++;
    memcpy(blk->data, node, sizeof(node_t));

    file_t tempf;
    tempf.ptr_global = parent_off * BLOCK_SIZE;

    ata_write_bytes(
        &tempf,
        BLOCK_SIZE, (uint8_t *) blk
    );
    
    memset(blk, 0, BLOCK_SIZE);
    memset(node, 0, sizeof(node_t));

    node->magic = NODE_MAGIC;
    node->first_block = r;
    node->flags |= type;
    if (type == FS_FILE) node->mode = 63; // rwx rwx
    else if (type == FS_DIR) node->mode = 54; // rw- rw-
    else if (type == FS_CHARDEV) node->mode = 38; // r-- rw-
    node->size = 0;
    strcpy(node->name, name);

    blk->attributes |= BLOCK_METADATA;
    blk->next = 0;
    memcpy(blk->data, node, sizeof(node_t));

    tempf.ptr_global = r * BLOCK_SIZE;
    ata_write_bytes(&tempf, BLOCK_SIZE, (uint8_t *) blk);

    kfree(blk);
    return node;
}

node_t **listdir(char *path) {
    node_t *node = (node_t *) kmalloc(sizeof(node_t));
    uint32_t off = find_node(path, FS_DIR, 0, node);

    if (off == 0) {
        kfree(node);
        return NULL; // directory not found
    }

    block_t *blk = get_block_by_index(off);
    node_t **children = (node_t **) kmalloc(node->size * sizeof(node_t *));
    uint32_t *child_indices = (uint32_t *) (blk->data + sizeof(node_t));

    for (int i = 0; i < node->size; i++) {
        children[i] = (node_t *) kmalloc(sizeof(node_t));
        block_t *child_blk = get_block_by_index(child_indices[i]);
        memcpy(children[i], child_blk->data, sizeof(node_t));
        kfree(child_blk);
    }

    kfree(blk);
    kfree(node);

    return children;
}

// if both append and write is selected, write will be executed
file_t *fopen(char *path, uint8_t mode) {
    if (mode == 0) return NULL;

    node_t *node = (node_t *) kmalloc(sizeof(node_t));
    file_t *file = (file_t *) kmalloc(sizeof(file_t));
    if (!node || !file) {
        #ifdef DEBUG
        serial_printf("fopen: kmalloc failed\n");
        #endif
        return NULL;
    }

    uint32_t off = find_node(path, FS_FILE, 0, node);

    if (mode & FMODE_W) {
        if (off == 0) { // file does not exist
            kfree(node); // free old node
            node = mknode(path, FS_FILE);
        }

        if ((node->mode & MODE_W) == 0) {
            kfree(node);
            kfree(file);
            return NULL; // not writeable
        }

        file->node = node;
        file->mode = mode;
        file->is_locked = 0;
        file->ptr_local = 0;
        file->ptr_global = BLOCK_SIZE * node->first_block;
    } else if (mode & FMODE_A) { // append
        if (off == 0) { // file does not exist
            kfree(node);
            node = mknode(path, FS_FILE);
        }

        if ((node->mode & MODE_W) == 0) {
            kfree(node);
            kfree(file);
            return NULL;
        }

        // we need to find the last block to set the file pointer
        block_t *block = get_block_by_index(node->first_block);
        int next = node->first_block;
        while (block->next != 0) {
            next = block->next;
            kfree(block);

            block = get_block_by_index(block->next);
        }

        file->node = node;
        file->mode = mode;
        file->is_locked = 0;
        file->ptr_local = node->size;
        file->ptr_global = next * BLOCK_SIZE + node->size % BLOCK_SIZE;
        if (next == node->first_block) file->ptr_global += sizeof(node_t);
    } else if (mode & FMODE_R) {
        if (off == 0) {
            #ifdef DEBUG
            serial_printf("fopen: File %s doesn't exist\n", path);
            #endif

            kfree(node);
            kfree(file);
            return NULL;
        }

        file->node = node;
        file->mode = mode;
        file->is_locked = 0;
        file->ptr_local = 0;
        file->ptr_global = BLOCK_SIZE * node->first_block;        
    }

    return file;
}

void fclose(file_t *file) {
    kfree(file->node);
    kfree(file);

    return;
}

int fread(file_t *file, uint32_t size, uint8_t *buffer) {
    if ((file->mode & FMODE_R) == 0) {
        return -1;
    }

    __file_lock(file);

    // the first block of the file contains metadata
    int offset = file->ptr_local + sizeof(node_t);

    int remaining_size = file->node->size - file->ptr_local;
    if (remaining_size <= 0) {
        __file_unlock(file);
        return -1; // invalid seek/reached EOF
    } else if (remaining_size < size) {
        size = remaining_size;
    }

    // find seek pointer in current block.
    int nblocks_real = offset / BLOCK_DATA_SIZE; // get number of real blocks.
    int off_crt_block = offset - nblocks_real * BLOCK_DATA_SIZE;

    block_t *crt_block = get_block_by_index(file->ptr_global / BLOCK_SIZE);

    // read blocks
    // remaining data = BLOCK_DATA_SIZE - off_crt_block
    if (size <= BLOCK_DATA_SIZE - off_crt_block) {
        memcpy(buffer, crt_block->data + off_crt_block, size);

        file->ptr_local += size;
        file->ptr_global += size;

        __file_unlock(file);
        return 0;
    }

    // size > current block
    memcpy(buffer, crt_block->data + off_crt_block, BLOCK_DATA_SIZE - off_crt_block);
    buffer += BLOCK_DATA_SIZE - off_crt_block;
    size -= BLOCK_DATA_SIZE - off_crt_block;

    int temp = size / BLOCK_DATA_SIZE; // number of whole blocks to read
    int next = crt_block->next;
    for (int i = 0; i < temp; i++) {
        if (crt_block->next == 0) {
            __file_unlock(file);
            return -1; // I/O error
        }

        // find next block
        kfree(crt_block);
        crt_block = get_block_by_index(next);
        memcpy(buffer, crt_block->data, BLOCK_DATA_SIZE);

        next = crt_block->next;
        buffer += BLOCK_DATA_SIZE;
        size -= BLOCK_DATA_SIZE;
        file->ptr_local += BLOCK_DATA_SIZE;
    }

    // we do not need the entire block.
    if (size > 0) {
        if (crt_block->next == 0) {
            __file_unlock(file);
            return -1; // I/O error
        }

        int next = crt_block->next;
        kfree(crt_block);
        crt_block = get_block_by_index(next);
        memcpy(buffer, crt_block->data, size);
    }

    file->ptr_global = BLOCK_SIZE * next + size;
    file->ptr_local += size;
   
    __file_unlock(file);

    kfree(crt_block);
    return 0;
}

int fwrite(file_t *file, uint32_t size, uint8_t *buffer) {
    if ((file->node->mode & MODE_W) == 0) {
        return -1; // file is readonly
    }

    if ((file->mode & FMODE_A) != 0) {
        // append (can only append to end of file.)

        __file_lock(file);
    }

    // write
    if (size == 0) return 0;

    __file_lock(file);

    file_t tempf;

    int offset = file->ptr_local + sizeof(node_t);
    int nblocks_real = offset / BLOCK_DATA_SIZE; // get number of real blocks.
    int off_crt_block = offset - nblocks_real * BLOCK_DATA_SIZE;

    block_t *crt_block = get_block_by_index(file->ptr_global / BLOCK_SIZE);

    // set new size
    if (file->ptr_local + size > file->node->size) {
        file->node->size += file->ptr_local + size - file->node->size;
    }

    // update node size
    if (file->ptr_global / BLOCK_SIZE == file->node->first_block) {
        // current block is the first one
        memcpy(crt_block->data, file->node, sizeof(node_t));
    } else {
        // find first block
        block_t *first_blk = get_block_by_index(file->node->first_block);
        memcpy(first_blk->data, file->node, sizeof(node_t));

        tempf.ptr_global = file->node->first_block * BLOCK_SIZE;
        ata_write_bytes(
            &tempf,
            BLOCK_SIZE, (uint8_t *) first_blk
        );
        kfree(first_blk);
    }

    // write single block
    if (size <= BLOCK_DATA_SIZE - off_crt_block) {
        memcpy(crt_block->data + off_crt_block, buffer, size);

        tempf.ptr_global = (file->ptr_global / BLOCK_SIZE) * BLOCK_SIZE;
        ata_write_bytes(
            &tempf,
            BLOCK_SIZE, (uint8_t *) crt_block
        );

        file->ptr_global += size;
        file->ptr_local += size;
        
        kfree(crt_block);

        __file_unlock(file);
        return 0;
    }

    // first block
    memcpy(crt_block->data + off_crt_block, buffer, BLOCK_DATA_SIZE - off_crt_block);

    // for efficiency reasons, all the blocks that belong to a file will be placed after the previous file block.
    uint32_t next_idx = find_next_free_block(NULL, file->ptr_global / BLOCK_SIZE);
    crt_block->next = next_idx;

    tempf.ptr_global = (file->ptr_global / BLOCK_SIZE) * BLOCK_SIZE;
    ata_write_bytes(
        &tempf,
        BLOCK_SIZE, (uint8_t *) crt_block
    );


    uint8_t buff[1];
    buff[0] = 0;

    // set block as not free, so its not detected by find_next_free_block
    tempf.ptr_global = next_idx * BLOCK_SIZE + 4;
    ata_write_bytes(&tempf, 1, buff);
    kfree(crt_block);

    file->ptr_local += BLOCK_DATA_SIZE - off_crt_block;
    buffer += BLOCK_DATA_SIZE - off_crt_block;
    size -= BLOCK_DATA_SIZE - off_crt_block;

    int c = 0;
    int temp = size / BLOCK_DATA_SIZE;
    for (int i = 0; i < temp; i++) {
        crt_block = get_block_by_index(next_idx);

        c = next_idx; // save current index to calculate offset later.
        next_idx = find_next_free_block(NULL, next_idx);

        crt_block->attributes = 0; // remove BLOCK_FREE flag.
        crt_block->next = (size > 0) ? next_idx : 0;
        memcpy(crt_block->data, buffer, BLOCK_DATA_SIZE);

        tempf.ptr_global = c * BLOCK_SIZE;
        ata_write_bytes(
            &tempf,
            BLOCK_SIZE, (uint8_t *) crt_block
        );

        // mark next block as not free.
        tempf.ptr_global = next_idx * BLOCK_SIZE + 4;
        ata_write_bytes(
            &tempf,
            1, buff
        ); 

        file->ptr_local += BLOCK_DATA_SIZE;
        buffer += BLOCK_DATA_SIZE;
        size -= BLOCK_DATA_SIZE;

        kfree(crt_block);
    }

    // write last block
    if (size > 0) {
        crt_block = get_block_by_index(next_idx);

        crt_block->attributes = 0;
        crt_block->next = 0;
        memcpy(crt_block->data, buffer, size);

        tempf.ptr_global = next_idx * BLOCK_SIZE;
        ata_write_bytes(
            &tempf,
            BLOCK_SIZE, (uint8_t *) crt_block
        );

        file->ptr_local += size;
    }

    file->ptr_global = next_idx * BLOCK_SIZE + size;

    __file_unlock(file);
    return 0;
}

int fseek(file_t *file, uint32_t n, uint8_t mode) {
    if (mode == SEEK_SET) {
        assert(n < file->node->size);

        file->ptr_local = n;
        // now we have to iterate the blocks to find the ptr_global.
        int i = n / BLOCK_DATA_SIZE; // i = 0x2002

        // the offset is set somewhere in the first block
        if (i == 0) {
            file->ptr_global = file->node->first_block * BLOCK_SIZE + n;
        }

        /*
        1st iteration (1st block, meaning we have read 0th block) -> 0x1001
        2nd iteration -> 0x2001
        */
        block_t *blk = get_block_by_index(file->node->first_block);
        int next = blk->next;
        while (i > 0) {
            blk = get_block_by_index(next);
            next = blk->next;
            i--;
        }

        file->ptr_global = next * BLOCK_SIZE + n % BLOCK_DATA_SIZE;
    }
}