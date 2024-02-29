DESIGN:



2.Any implementation requirements that are not met
- all implementation required is met.

3.any part of the implementation that deviates from the precode(if creating own files explain what their purpose is)
void free_blocks(int *indexes, int length)
- help-function for free_block
- instead of only freeing 1 block and using a for-loop for all other blocks that got sucessfully allocated, implemented this function to pre-deallocate all blocks. making the call for this function simpler
  
int is_node_in_parent(Struct inode* parent, struct inode* node):
- verifies that node is indeed a child of parent dir
- returns 0 if false, used to return -1 in delete_file thus failing to complete the function
  
struct inode* load_inodes_recursive(FILE* fil, size_t offset:
- help-function to systematically read through MFT ensuring every node in the file gets read correctly
- also connects children inodes to their parent inodes, puts them in the correct children array

4.any tests that fail and what you think the cause may be
no failed tests.
