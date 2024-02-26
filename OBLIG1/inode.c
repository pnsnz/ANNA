#include "allocation.h"
#include "inode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#define BLOCKSIZE 4096

static int num_inode_ids = 0;

static int blocks_needed( int bytes )
{
    int blocks = bytes / BLOCKSIZE;
    if( bytes % BLOCKSIZE != 0 )
        blocks += 1;
    return blocks;
}

static int next_inode_id( )
{
    int retval = num_inode_ids;
    num_inode_ids += 1;
    return retval;
}

struct inode* create_file( struct inode* parent, char* name, int size_in_bytes )
{
    /* to be implemented */
    return NULL;
}

struct inode* create_dir( struct inode* parent, char* name )
{
    /* to be implemented */
    return NULL;
}

struct inode* find_inode_by_name( struct inode* parent, char* name )
{
    /* to be implemented */
    return NULL;
}

static int verified_delete_in_parent( struct inode* parent, struct inode* node )
{
    /* to be implemented */
    return 0;
}

int is_node_in_parent( struct inode* parent, struct inode* node )
{
    /* to be implemented */
    return 0;
}

int delete_file( struct inode* parent, struct inode* node )
{
    /* to be implemented */
    return 0;
}

int delete_dir( struct inode* parent, struct inode* node )
{
    // når slettet skal det ikke være noen filer i dir!
    /* to be implemented */
    return 0;
}

struct inode* load_inodes( char* master_file_table )
{
    FILE *fil = fopen(master_file_table, "r");
    if (fil == NULL) {
        perror("File opening");
        return EXIT_FAILURE;
    }

    // struct
    // kanskje ikke smart å kalle den root mtp den skal være generell?
    struct inode* root = malloc(sizeof(struct inode));

    // ID
    fread(&root->id, 1, sizeof(int),fil);

    //LENGTH OF NAME
    int length_of_name;
    fread(&length_of_name, 1, sizeof(int), fil);

    // ->/.? 
    root->name = malloc(length_of_name + 1); //+1 for good meassure?
    if (root->name == NULL){
        printf("heap alloc failed.\n");
        return EXIT_FAILURE;
    }

    // leser fra fil inn i struct navn
    fread(root->name, 1, length_of_name, fil);

    //flag
    fread(&root->is_directory, 1, sizeof(char), fil);

    // dom c syntax ugh
    if (root->is_directory) {
        //num_children
        fread(&root->num_children, 1, sizeof(int), fil);

        root->children = malloc(sizeof(struct inode*) * (root->num_children));

        for (int i = 0; i < root->num_children; i++) {
            fread(&root->children[i], 1, sizeof(size_t), fil);
            root->children[i] = load_inodes(master_file_table); //?
        }
    }
    else {
        fread(&root->filesize, 1, sizeof(int), fil); //file_size
        fread(&root->num_blocks, 1, sizeof(int), fil); //num_blocks

        root->blocks = malloc(sizeof(size_t) * root->num_blocks);

        fread(&root->blocks, sizeof(size_t), root->num_blocks, fil);
    }

    fclose(fil);

    return EXIT_SUCCESS;
}

static void save_inode( FILE* file, struct inode* node )
{
    if( !node ) return;

    int len = strlen( node->name ) + 1;

    fwrite( &node->id, 1, sizeof(int), file );
    fwrite( &len, 1, sizeof(int), file );
    fwrite( node->name, 1, len, file );
    fwrite( &node->is_directory, 1, sizeof(char), file );
    if( node->is_directory )
    {
        fwrite( &node->num_children, 1, sizeof(int), file );
        for( int i=0; i<node->num_children; i++ )
        {
            struct inode* child = node->children[i];
            size_t id = child->id;
            fwrite( &id, 1, sizeof(size_t), file );
        }

        for( int i=0; i<node->num_children; i++ )
        {
            struct inode* child = node->children[i];
            save_inode( file, child );
        }
    }
    else
    {
        fwrite( &node->filesize, 1, sizeof(int), file );
        fwrite( &node->num_blocks, 1, sizeof(int), file );
        for( int i=0; i<node->num_blocks; i++ )
        {
            fwrite( &node->blocks[i], 1, sizeof(size_t), file );
        }
    }
}

void save_inodes( char* master_file_table, struct inode* root )
{
    if( root == NULL )
    {
        fprintf( stderr, "root inode is NULL\n" );
        return;
    }

    FILE* file = fopen( master_file_table, "w" );
    if( !file )
    {
        fprintf( stderr, "Failed to open file %s\n", master_file_table );
        return;
    }

    save_inode( file, root );

    fclose( file );
}

static int indent = 0;

void debug_fs( struct inode* node )
{
    if( node == NULL ) return;
    for( int i=0; i<indent; i++ )
        printf("  ");

    if( node->is_directory )
    {
        printf("%s (id %d)\n", node->name, node->id );
        indent++;
        for( int i=0; i<node->num_children; i++ )
        {
            struct inode* child = (struct inode*)node->children[i];
            debug_fs( child );
        }
        indent--;
    }
    else
    {
        printf("%s (id %d size %db blocks ", node->name, node->id, node->filesize );
        for( int i=0; i<node->num_blocks; i++ )
        {
            printf("%d ", (int)node->blocks[i]);
        }
        printf(")\n");
    }
}

void fs_shutdown( struct inode* inode )
{
    if( !inode ) return;

    if( inode->is_directory )
    {
        for( int i=0; i<inode->num_children; i++ )
        {
            fs_shutdown( inode->children[i] );
        }
    }

    if( inode->name )     free( inode->name );
    if( inode->children ) free( inode->children );
    if( inode->blocks )   free( inode->blocks );
    free( inode );
}

