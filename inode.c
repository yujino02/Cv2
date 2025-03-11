#include "inode.h"
#include "block_allocation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct inode* create_file( struct inode* parent, const char* name, char readonly, int size_in_bytes )
{
    fprintf( stderr, "%s is not implemented\n", __FUNCTION__ );
    return NULL;
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
    fprintf( stderr, "%s is not implemented\n", __FUNCTION__ );
    return;
}

struct inode* load_inodes( const char* master_file_table )
{
    // Open the master file table for reading
    FILE* file = fopen(master_file_table, "rb");
    if (!file) {
        fprintf(stderr, "Error opening master file table: %s\n", strerror(errno));
        return NULL;
    }

    // Allocate memory for the root inode
    struct inode* root = malloc(sizeof(struct inode));
    if (!root) {
        fprintf(stderr, "Memory allocation failed for root inode\n");
        fclose(file);
        return NULL;
    }

    // Read basic inode metadata from disk
    fread(&(root->id), sizeof(uint32_t), 1, file);

    // Read the name length and allocate memory for the name
    uint32_t name_length;
    fread(&name_length, sizeof(uint32_t), 1, file);
    root->name = malloc(name_length);
    fread(root->name, sizeof(char), name_length, file);

    // Read inode attributes
    fread(&(root->is_directory), sizeof(char), 1, file);
    fread(&(root->is_readonly), sizeof(char), 1, file);
    fread(&(root->filesize), sizeof(uint32_t), 1, file);
    fread(&(root->num_entries), sizeof(uint32_t), 1, file);

    // Load inode entries (children)
    if (root->num_entries > 0) {
        root->entries = malloc(root->num_entries * sizeof(uintptr_t));
        for (uint32_t i = 0; i < root->num_entries; i++) {
            uint32_t entry_id;
            fread(&entry_id, sizeof(uint32_t), 1, file);
            root->entries[i] = (uintptr_t)entry_id; // Store ID for later resolution
        }
    } else {
        root->entries = NULL;
    }

    fclose(file);
    return root;
}

void fs_shutdown( struct inode* inode )
{
    fprintf( stderr, "%s is not implemented\n", __FUNCTION__ );
    return;
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

