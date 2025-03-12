#include "inode.h"
#include "block_allocation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct inode* find_inode_by_id(struct inode* root, uint32_t id);
void resolve_inode_pointers(struct inode* root);

struct inode* create_file( struct inode* parent, const char* name, char readonly, int size_in_bytes )
{
    if (!parent || !parent->is_directory) {
        return NULL;
    }

    // 기존에 동일한 파일명이 있으면 실패
    if (find_inode_by_name(parent, name) != NULL) {
        return NULL;
    }

    struct inode* new_file = malloc(sizeof(struct inode));
    if (!new_file) return NULL;

    new_file->id = rand();  // 임시 ID 설정
    new_file->name = strdup(name);
    new_file->is_directory = 0;
    new_file->is_readonly = readonly;
    new_file->filesize = size_in_bytes;
    new_file->num_entries = (size_in_bytes + BLOCKSIZE - 1) / BLOCKSIZE;  // 필요한 블록 수

    new_file->entries = malloc(new_file->num_entries * 2 * sizeof(uint32_t));
    if (!new_file->entries) {
        free(new_file->name);
        free(new_file);
        return NULL;
    }

    uint32_t* extents = (uint32_t*)new_file->entries;
    for (int i = 0; i < new_file->num_entries; i++) {
        int block = allocate_block(1);
        if (block == -1) {
            for (int j = 0; j < i; j++)
                free_block(extents[j * 2]);
            free(new_file->entries);
            free(new_file->name);
            free(new_file);
            return NULL;
        }
        extents[i * 2] = block;
        extents[i * 2 + 1] = 1;  // extent 크기 1
    }

    parent->entries = realloc(parent->entries, (parent->num_entries + 1) * sizeof(uintptr_t));
    parent->entries[parent->num_entries++] = (uintptr_t)new_file;

    return new_file;
}

struct inode* create_dir( struct inode* parent, const char* name )
{
    fprintf( stderr, "%s is not implemented\n", __FUNCTION__ );
    return NULL;
}

struct inode* find_inode_by_name( struct inode* parent, const char* name )
{
    if (!parent || !parent->is_directory) return NULL;

    for (uint32_t i = 0; i < parent->num_entries; i++) {
        struct inode* child = (struct inode*)parent->entries[i];
        if (strcmp(child->name, name) == 0) {
            return child;
        }
    }
    return NULL;
}

int delete_file( struct inode* parent, struct inode* node )
{
    fprintf( stderr, "%s is not implemented\n", __FUNCTION__ );
    return -1;
}

int delete_dir( struct inode* parent, struct inode* node )
{
    fprintf( stderr, "%s is not implemented\n", __FUNCTION__ );
    return -1;
}

void save_inodes( const char* master_file_table, struct inode* root )
{
    FILE* file = fopen(master_file_table, "wb");
    if (!file) {
        fprintf(stderr, "Error opening master file table: %s\n", strerror(errno));
        return;
    }

    fwrite(&(root->id), sizeof(uint32_t), 1, file);
    uint32_t name_length = strlen(root->name) + 1;
    fwrite(&name_length, sizeof(uint32_t), 1, file);
    fwrite(root->name, sizeof(char), name_length, file);
    fwrite(&(root->is_directory), sizeof(char), 1, file);
    fwrite(&(root->is_readonly), sizeof(char), 1, file);
    fwrite(&(root->filesize), sizeof(uint32_t), 1, file);
    fwrite(&(root->num_entries), sizeof(uint32_t), 1, file);

    if (root->num_entries > 0) {
        for (uint32_t i = 0; i < root->num_entries; i++) {
            uint32_t entry_id = ((struct inode*)root->entries[i])->id;
            fwrite(&entry_id, sizeof(uint32_t), 1, file);
        }
    }
    fclose(file);
}

struct inode* load_inodes( const char* master_file_table )
{
    FILE* file = fopen(master_file_table, "rb");
    if (!file) {
        fprintf(stderr, "Error opening master file table: %s\n", strerror(errno));
        return NULL;
    }

    // root inode create
    struct inode* root = malloc(sizeof(struct inode));
    if (!root) {
        fprintf(stderr, "Memory allocation failed for root inode\n");
        fclose(file);
        return NULL;
    }

    //inode 기본 정보 읽기
    fread(&(root->id), sizeof(uint32_t), 1, file);

    uint32_t name_length;
    fread(&name_length, sizeof(uint32_t), 1, file);
    root->name = malloc(name_length);
    fread(root->name, sizeof(char), name_length, file);
    root->name[name_length - 1] = '\0';  // 문자열 끝에 NULL 추가

    fread(&(root->is_directory), sizeof(char), 1, file);
    fread(&(root->is_readonly), sizeof(char), 1, file);
    fread(&(root->filesize), sizeof(uint32_t), 1, file);
    fread(&(root->num_entries), sizeof(uint32_t), 1, file);

    //하위 inode 로드
    if (root->num_entries > 0) {
        root->entries = malloc(root->num_entries * sizeof(struct inode*));
        if (!root->entries) {
            free(root->name);
            free(root);
            fclose(file);
            return NULL;
        }

        //하위 inode의 ID 목록을 먼저 읽기
        uint32_t* entry_ids = malloc(root->num_entries * sizeof(uint32_t));
        fread(entry_ids, sizeof(uint32_t), root->num_entries, file);

        //ID → 실제 inode로 변환
        for (uint32_t i = 0; i < root->num_entries; i++) {
            struct inode* child = malloc(sizeof(struct inode));
            if (!child) {
                free(entry_ids);
                free(root->entries);
                free(root->name);
                free(root);
                fclose(file);
                return NULL;
            }

            child->id = entry_ids[i];  // ID 설정
            child->name = NULL;  // 아직 이름을 모름 → 나중에 설정
            child->is_directory = 0;
            child->is_readonly = 0;
            child->filesize = 0;
            child->num_entries = 0;
            child->entries = NULL;

            root->entries[i] = (uintptr_t)child;
        }

        free(entry_ids);
    } else {
        root->entries = NULL;
    }

    fclose(file);
    return root;
}

void resolve_inode_pointers(struct inode* root) {
    if (!root || !root->is_directory) return;

    for (uint32_t i = 0; i < root->num_entries; i++) {
        struct inode* child = (struct inode*)root->entries[i];
        if (child) {
            // Finn inode basert på id
            child = find_inode_by_id(root, child->id);
            root->entries[i] = (uintptr_t)child;
        }
        resolve_inode_pointers(child);
    }
}

struct inode* find_inode_by_id(struct inode* root, uint32_t id) {
    if (!root) return NULL;
    if (root->id == id) return root;

    for (uint32_t i = 0; i < root->num_entries; i++) {
        struct inode* found = find_inode_by_id((struct inode*)root->entries[i], id);
        if (found) return found;
    }
    return NULL;
}

void fs_shutdown( struct inode* node )
{
    if (!node) return;

    free(node->name);
    if (node->num_entries > 0) {
        for (uint32_t i = 0; i < node->num_entries; i++) {
            fs_shutdown((struct inode*)node->entries[i]);
        }
        free(node->entries);
    }
    free(node);
}

/* This static variable is used to change the indentation while debug_fs
 * is walking through the tree of inodes and prints information.
 */
static int indent = 0;

static void debug_fs_print_table( const char* table );
static void debug_fs_tree_walk( struct inode* node, char* table );

void debug_fs( struct inode* node )
{
    char* table = calloc( NUM_BLOCKS, 1 );
    debug_fs_tree_walk( node, table );
    debug_fs_print_table( table );
    free( table );
}

static void debug_fs_tree_walk( struct inode* node, char* table )
{
    if( node == NULL ) return;
    for( int i=0; i<indent; i++ )
        printf("  ");
    if( node->is_directory )
    {
        printf("%s (id %d)\n", node->name, node->id );
        indent++;
        for( int i=0; i<node->num_entries; i++ )
        {
            struct inode* child = (struct inode*)node->entries[i];
            debug_fs_tree_walk( child, table );
        }
        indent--;
    }
    else
    {
        printf("%s (id %d size %d)\n", node->name, node->id, node->filesize );

        /* The following is an ugly solution. We expect you to discover a
         * better way of handling extents in the node->entries array, and did
         * it like this because we don't want to give away a good solution here.
         */
        uint32_t* extents = (uint32_t*)node->entries;

        for( int i=0; i<node->num_entries; i++ )
        {
            for( int j=0; j<extents[2*i+1]; j++ )
            {
                table[ extents[2*i]+j ] = 1;
            }
        }
    }
}

static void debug_fs_print_table( const char* table )
{
    printf("Blocks recorded in master file table:");
    for( int i=0; i<NUM_BLOCKS; i++ )
    {
        if( i % 20 == 0 ) printf("\n%03d: ", i);
        printf("%d", table[i] );
    }
    printf("\n\n");
}

