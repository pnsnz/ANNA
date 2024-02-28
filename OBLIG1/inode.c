#include "allocation.h"
#include "inode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* The number of bytes in a block.
 * Do not change.
 */
#define BLOCKSIZE 4096

/* The lowest unused node ID.
 * Do not change.
 */
static int num_inode_ids = 0;

/* This helper function computes the number of blocks that you must allocate
 * on the simulated disk for a give file system in bytes. You don't have to use
 * it.
 * Do not change.
 */
static int blocks_needed( int bytes )
{
    int blocks = bytes / BLOCKSIZE;
    if( bytes % BLOCKSIZE != 0 )
        blocks += 1;
    return blocks;
}

/* This helper function returns a new integer value when you create a new inode.
 * This helps you do avoid inode reuse before 2^32 inodes have been created. It
 * keeps the lowest unused inode ID in the global variable num_inode_ids.
 * Make sure to update num_inode_ids when you have loaded a simulated disk.
 * Do not change.
 */
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

int is_node_in_parent( struct inode* parent, struct inode* node ) // 0 = false, 1 = true
{
    if (parent == NULL | node == NULL) { // hvis forelder/barn ikke finnes
        return 0;
    }
    
    if (!parent->is_directory) { // hvis fil
        return 0;
    }
    if (parent->num_children == 0) { // hvis ingen barn
        return 0;
    }

    // parent er en directory med barn
    for ( int i = 0; i < parent->num_children; i++ ) { // iterer gjennom barn
        struct inode* child = parent->children[i]; // mellomlagrer

        if ( strcmp(parent->name, child->name) == 0 ) {
            return 1; 
        }
    }

    return 0;
}

int delete_file( struct inode* parent, struct inode* node )
{
    
    // hvis ikke direkte forelder, denne sjekker også at begge finnes, og at parent har barn
    if ( is_node_in_parent(parent, node) == 0 ) { 
        return -1;
    }

    for ( int i = 0; i < parent->num_children; i++ ) { // fjern node fra parent->children[]
        struct inode* child = parent->children[i];

        if ( child->id == node->id ) { // finn riktig node i children
            parent->children[i] == NULL;
        }
    }
    
    for ( int i = 0; i < node->num_blocks; i++ ) { // frigjør blokkene
        free_block( node->blocks[i] ); // litt usikker på denne
    }

    free(node->name);
    free(node);

    return 0;
}

int delete_dir( struct inode* parent, struct inode* node )
{
    if ( is_node_in_parent(parent, node) == 0 ) { 
        return -1;
    }

    if (!node->is_directory) { // hvis barnet er en fil
        return -1;
    }

    // antar at vi har endret num_children riktig til nå
    if (node->num_children > 0) {  
        return -1; // hvis noden ikke er tom
    }

    for ( int i = 0; i < parent->num_children; i++ ) { // fjern node fra parent->children[]
        struct inode* child = parent->children[i];

        if ( child->id == node->id ) { // finn riktig node i children
            parent->children[i] == NULL;
        }
    }

    free(node->children);
    free(node->name);
    free(node);

    return 0;
}

struct inode* load_inodes_recursive(FILE* fil, size_t offset)
{   
    //sets the file position to given offset, returns 0 if success
    fseek(fil, offset, SEEK_SET);

    //allocate memory for the node struct
    struct inode* node = malloc(sizeof(struct inode));

    if(node == NULL) {
        perror("Failed allocating memory for inode");
        return NULL;
    }

    fread(&node->id, 1, sizeof(int),fil);

    int len;
    fread(&len, 1, sizeof(int), fil);

    //allocate memory for char array, name
    node->name = malloc(len);
    fread(node->name, 1, len, fil);

    if (node->name == NULL){
        printf("Failed reading name");
        free(node);
        return NULL;
    }

    fread(&node->is_directory, 1, sizeof(char), fil);

    if(node->is_directory) {
        //each child entry will fill 8 bytes.
        fread(&node->num_children, 1, sizeof(int), fil);

        //allocate memory for children array
        node->children = malloc(sizeof(struct inode*) * (node->num_children));

        for (int i = 0; i < node->num_children; i++) {

            //calculating the offset
            size_t bytes_read = ftell(fil);

            fread(&node->children[i], 1, sizeof(size_t), fil);

            if (i == 0) {
                //use recursive with the right offset
                node->children[i] = load_inodes_recursive(fil, bytes_read + (node->num_children * sizeof(size_t)));
            } else {
                node->children[i] = load_inodes_recursive(fil, bytes_read);
            }
        }
    }
    else {
        //node is a file, no children

        fread(&node->filesize, 1, sizeof(int), fil);
        fread(&node->num_blocks, 1, sizeof(int), fil);

        //allocate memory for blocks array
        node->blocks = malloc(sizeof(size_t) * node->num_blocks);

        fread(node->blocks, sizeof(size_t), node->num_blocks, fil);
    }

    return node;
}


struct inode* load_inodes( char* master_file_table ) {
    FILE *fil = fopen(master_file_table, "r");

    if (fil == NULL) {
        perror("File opening");
        return NULL;
    }

    struct inode *root = load_inodes_recursive(fil,  0);

    fclose(fil);

    return root;
}

/* The function save_inode is a recursive functions that is
 * called by save_inodes to store a single inode on disk,
 * and call itself recursively for every child if the node
 * itself is a directory.
 */
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

/* This static variable is used to change the indentation while debug_fs
 * is walking through the tree of inodes and prints information.
 */
static int indent = 0;

/* Do not change.
 */
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

/* Do not change.
 */
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

