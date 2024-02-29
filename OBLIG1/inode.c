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

void free_blocks(int *indexes, int length)
{
    for(int i = 0; i < length; i++)
    {
        free_block(indexes[i]);
    }
}

struct inode* create_file( struct inode* parent, char* name, int size_in_bytes )
{

    //if file with same name exist, return NULL, do nothing
    if(!parent->is_directory || find_inode_by_name(parent, name) != NULL){
        return NULL;
    }

    int number_of_blocks = blocks_needed(size_in_bytes);

    //indexes in the block_table that we used
    int index[number_of_blocks];

    for(int i = 0; i < number_of_blocks; i++)
    {
        int allocated_block = allocate_block();
        //can only allocate one block
        if(allocated_block == -1)
        {
            perror("Cannot allocate all needed blocks");
            free_blocks(index, number_of_blocks);
            return NULL;
        }
        index[i] = allocated_block;
    }
    //if place is there make a node, first allocate memory for it
    struct inode *file = malloc(sizeof(struct inode));
    if (file == NULL) {
        perror("Couldnt allocate space for file");
        return NULL;
    }

    file->id = next_inode_id();

    file->name = malloc(strlen(name)+1);
    if(file->name == NULL){
        perror("Couldnt allocate space for name");
        return NULL;
    }

    //have to copy, cant point at the same address
    //if the name changes, program goes booooo
    strcpy(file->name,name);

    file->is_directory = 0;
    file->filesize = size_in_bytes;
    file->num_blocks = number_of_blocks;

    file->blocks = malloc(sizeof(size_t) * number_of_blocks);
    if(file->blocks == NULL) {
        perror("Couldnt allocate space for blocks");
        free_blocks(index,number_of_blocks);
        return NULL;
    }

    for(int i = 0; i< number_of_blocks; i++) {
        file-> blocks[i] = index[i];
    }

    parent->num_children++;
    //making previously allocated memory bigger
    parent->children = realloc(parent->children, sizeof(struct inode*)*parent->num_children);
    //getting the last index of the array
    parent->children[parent->num_children-1] = file;

    return file;
}

struct inode* create_dir( struct inode* parent, char* name )
{

    if (find_inode_by_name(parent, name) != NULL){
        return NULL;
    }

    // create struct
    struct inode* new_dir = malloc(sizeof(struct inode));

    //set ID
    new_dir->id = next_inode_id();

    // set name
    new_dir->name = malloc(strlen(name)+1);
    if (new_dir->name == NULL){
        perror("couldnt allocate name correctly");
        return NULL;
    }
    strcpy(new_dir->name, name);

    // set flag
    new_dir->is_directory = 1;

    // num_children
    new_dir->num_children = 0;

    // malloc struct * num_children0?
    new_dir->children = malloc(sizeof(struct inode*) * (new_dir->num_children));


    //det kan hende vi oppretter root noden
    if(parent)
    {
        parent->num_children++;
        //making previously allocated memory bigger
        parent->children = realloc(parent->children, sizeof(struct inode *) * parent->num_children);
        //getting the last index of the array
        parent->children[parent->num_children - 1] = new_dir;
    }

    return new_dir;
}

struct inode* find_inode_by_name( struct inode* parent, char* name )
{
    //antagelse at parent er dictionary
    if(parent == NULL || parent->is_directory == 0 )
    {
        return NULL ;
    }

    for( int i = 0; i < parent->num_children; i++ ) {
        struct inode* current_child = parent->children[i];

        if(strcmp(current_child->name, name) == 0 ) {
            return current_child;
        }
    }

    return NULL;
}

static int verified_delete_in_parent( struct inode* parent, struct inode* node )
{
    /* to be implemented
     * not a must*/
    return 0;
}

int is_node_in_parent( struct inode* parent, struct inode* node )
{
    if (parent == NULL || node == NULL) { // hvis forelder/barn ikke finnes
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
        if (child == NULL) {
            continue;
        }
        if ( child->id == node->id ) {
            return 1;
        }
    }

    return 0;
}

int delete_file( struct inode* parent, struct inode* node )
{
    // hvis ikke direkte forelder. Denne sjekker også at begge finnes, og at parent har barn
    if ( is_node_in_parent(parent, node) == 0 ) {
        return -1;
    }

    char foundChild = 0;

    for ( int i = 0; i < parent->num_children; i++ ) { // fjern node fra parent->children[]
        struct inode *child = parent->children[i];

        if ( child->id == node->id ) {// finn riktig node i children
            foundChild = 1;

            //frigjør minne for navn og blocks
            free(parent->children[i]->name);

            //frigjør noden
            free(parent->children[i]);

            for ( int b = 0; b < parent->children[i]->num_blocks; b++ ) { // frigjør blokkene
                free_block( parent->children[i]->blocks[b] ); // litt usikker på denne
            }
            free(parent->children[i]->blocks);

            parent->num_children--;


            //skyve arrayet på plass
            for(int j = i; j < parent->num_children; j++){
                parent->children[j] = parent->children[j+1];
            }

        }
    }

    //realloc so more effective, but idk if necessary
    parent->children = realloc(parent->children, sizeof(struct inode *) * (parent->num_children));

    if (!foundChild) {
        return -1;
    }

    return 0;
}


int delete_dir( struct inode* parent, struct inode* node )
{
    if ( is_node_in_parent(parent, node) == 0 || !(node->is_directory)) {
        return -1;
    }

    // antar at vi har endret num_children riktig til nå
    if (node->num_children > 0) {
        return -1; // hvis noden ikke er tom
    }

    char foundChild = 0;
    for ( int i = 0; i < parent->num_children; i++ ) { // fjern node fra parent->children[]
        struct inode *child = parent->children[i];

        if ( child->id == node->id ) { // finn riktig node i children

            foundChild = 1;

            //free them kids

            for ( int c = 0; c < child->num_children; c++ ) {

                if (child->children[c]->is_directory)
                {
                    delete_dir(child, child->children[c]);
                }
                else
                {
                    delete_file(child, child->children[c]);
                }
            }

            free(parent->children[i]->children);
            free(parent->children[i]->name);
            free(parent->children[i]);

            parent->num_children--;

            //skyve arrayet på plass
            for(int j = i; j < parent->num_children; j++){
                parent->children[j] = parent->children[j+1];
            }
        }
    }
    //realloc so more effective, but idk if necessary
    parent->children = realloc(parent->children, sizeof(struct inode *) * (parent->num_children ));

    if (!foundChild) {
        return -1;
    }
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
                // ved første barnet må vi hoppe over array
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
            // EGEN if sjekk
            if ( child == NULL) {
                continue;
            }
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

