# btree

c++ on-disk btree

B-Tree implementation based off of [Introduction to Alogorithms 3rd Edition](https://edutechlearners.com/download/Introduction_to_algorithms-3rd%20Edition.pdf)

Implements an on-disk b-tree that uses strings as keys and uint64_t as values.

## Feature Goals

* C++, header-only 
* on-disk, not in memory
* strings as keys
* 64-bit number as values

## Random Details

The class requires 2 parameters that determine its storage characteristics:

* `Degree` determines the number of keys each node stores ( `KeysPerNode` = 2 * `Degree` - 1)
* `KeySize` specifies the space used to save a key.  A key's length can be up to `KeySize` - 1.

The tree is implemented to have the following stuctures:

* Header - this stores the properties of the b-tree
* RootNode (Node)
* Nodes (Node[]) - contains storage for nodes; each node has an entry for the FreeNodeStack

An empty b-tree will consist of the Header and RootNode, and these make up the beginning 
of every b-tree's file structure.  As nodes are added to the b-tree, they are appended to 
the back of the b-tree's file.  When a Node is added it will be made available by also adding 
the node index to the FreeNodeStack.  When a new node is required it is allocated by popping 
a node index from the FreeNodeStack.  If no nodes are available in the FreeNodeStack then 
this is an indication that a new Node must be added to the tree and the file must grow.

### File Layout
* Header (tree info)
* Node (root)
* Nodes[] (zero or more allocated nodes)

### Header Layout
* keySize (4B)
* degree (4B)
* keyCount (4B)
* freeNodeCount (4B)

### Node Layout
* keyCount (4B)
* childCount (4B)
* freeNode (4B)
* unused (4B)
* childNodes[2*Degree] (4B each)
* keys[2*Degree-1] (KeySize each)
* values[2*Degree-1] (8B each)

