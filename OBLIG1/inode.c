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
    /*
    //if file with same name exist, return NULL, do nothing
    if(!parent->is_directory || find_inode_by_name(parent, name) != NULL){
        return NULL;
    }

    int number_of_blocks = blocks_needed(size_in_bytes);

    //indexes in the block_table that we used
    int index[];

    for(int i = 0; i < number_of_blocks; i++)
    {
        //can only allocate one block
        if(allocate_block() == -1)
        {
            perror("Cannot allocate all needed blocks");
            for(int i)
            free_block()
            return NULL;
        }
        index[i] = allocate_block()
    }
    */

    /* to be implemented */
    return NULL;
}

struct inode* create_dir( struct inode* parent, char* name )
{

    if (find_inode_by_name(parent, name) != NULL){
        return NULL;
    }

    // create struct
    struct inode* new_dir = malloc(sizeof(struct inode*));

    //set ID
    new_dir->id = next_inode_id();

    // set name
    new_dir->name = malloc(sizeof(name));
    new_dir->name = name;
    if (new_dir->name == NULL){
        perror("couldnt allocate name correctly");
        return -1;
    }

    // set flag
    new_dir->is_directory = 1;

    // num_children
    new_dir->num_children = 0;

    // malloc struct * num_children0?
    new_dir->children = malloc(sizeof(struct inode*) * (new_dir->num_children));

    return new_dir;
}

struct inode* find_inode_by_name( struct inode* parent, char* name )
{
    //antagelse at parent er dictionary
    if(!parent->is_directory)
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

//-------------------------------------------------------------

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
//-----------------------------------------------------------


/*
struct inode* load_inodes( char* master_file_table )
{
    int length_of_name;

    FILE *fil = fopen(master_file_table, "r");
    if (fil == NULL) {
        perror("File opening");
        return NULL;
    }

    struct inode* root = malloc(sizeof(struct inode)); // kall annet enn root

    fread(&root->id, 1, sizeof(int),fil); // ID

    fread(&length_of_name, 1, sizeof(int), fil); //LENGTH OF NAME

    root->name = malloc(length_of_name); //lager plass til navnet på heap
    if (root->name == NULL){
        printf("heap alloc failed.\n");
        return NULL;
    }

    fread(root->name, 1, length_of_name, fil); // leser fra fil inn i struct navn

    fread(&root->is_directory, 1, sizeof(char), fil); //flag

    if (root->is_directory) { // 1 for dir 0 for fil

        fread(&root->num_children, 1, sizeof(int), fil); // num_children

        root->children = malloc(sizeof(struct inode*) * (root->num_children));

        for (int i = 0; i < root->num_children; i++) {
            fread(&root->children[i], 1, sizeof(size_t), fil);
            root->children[i] = load_inodes(master_file_table); //?
        }

    } else {
        fread(&root->filesize, 1, sizeof(int), fil); //file_size
        fread(&root->num_blocks, 1, sizeof(int), fil); //num_blocks

        root->blocks = malloc(sizeof(size_t) * root->num_blocks);

        fread(&root->blocks, sizeof(size_t), root->num_blocks, fil);
    }

    fclose(fil);
    return root;
}
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

