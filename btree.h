//! #pragma once

//! B-Tree implementation based off of 'Introduction to Alogorithms 3rd Edition` 
//! (https://edutechlearners.com/download/Introduction_to_algorithms-3rd%20Edition.pdf)
//! 
//! Implements an on-disk b-tree that uses strings as keys and uint64_t as values.
//! 
//! The class requires 2 parameters that determine its storage characteristics:
//!     * KeySize specifies the space used to save a key.  A key's length can be up to KeySize - 1.
//!     * Degree determines the number of keys each node stores ( KeysPerNode = 2 * Degree - 1)
//! 
//! The tree is implemented to have the following stuctures:
//!     * Header - this stores the properties of the b-tree
//!     * RootNode (Node)
//!     * Nodes (Node[]) - contains storage for nodes and entries of the FreeNodeStack
//! 
//! An empty b-tree will consist of the Header and RootNode, and these make up the beginning 
//! of every b-tree's file structure.  As nodes are added to the b-tree, they are appended to 
//! the back of the b-tree's file.  When a Node is added it will be made available by also adding 
//! the node index to the FreeNodeStack.  When a new node is required it is allocated by popping 
//! a node index from the FreeNodeStack.  If no nodes are available in the FreeNodeStack then 
//! this is an indication that a new Node must be added to the tree and the file must grow.
//! 
//! File Layout: Header (tree info), Node (root), Group[] (zero or more allocated node groups)
//! Node Layout: keyCount (4B), freeNode (4B), childNodes[2*Degree] (4B), keys[2*Degree-1] (KeySize), values[2*Degree-1] (8B)
//! 
//! HeaderSize = 20
//! NodeSize = 8 + 8 * Degree + ( 2 * Degree - 1 ) * ( keylen + 8 )
//! 

#include <cassert>
#include <cstdint>
#include <string>
#include <compare>
#include <optional>
#include <vector>



class BTree
{
public:
                                            BTree( 
                                                std::string filename,                    
                                                uint32_t degree = 1024,
                                                uint32_t keylen = 128 );                // 8 to 128

    using                                   value_t = uint64_t;

    std::optional<value_t>                  get( const std::string & key ) const;
    bool                                    put( std::string key, value_t value );
    std::optional<value_t>                  remove( const std::string & key );
    size_t                                  size( ) const;
    bool                                    contains( const std::string & key ) const;
    std::string                             lower( const std::string & key ) const;     // aka previousKey
    std::string                             lowerOrEqual( const std::string & key ) const;
    std::string                             higher( const std::string & key ) const;    // aka nextKey
    std::string                             higherOrEqual( const std::string & key ) const;
    std::string                             first( ) const;
    std::string                             last( ) const;

    // inspection methods
    std::vector<std::string>                getKeysInNode( int nodeId );
    std::vector<int>                        getChildrenInNode( int nodeId );
    std::vector<int>                        getFreeNodes( );

    // properties
    size_t                                  degree( ) const;
    size_t                                  keySize( ) const;
    size_t                                  freeNodeCount( ) const;
    size_t                                  maxChildrenPerNode( ) const;
    size_t                                  minKeysPerNode( ) const;
    size_t                                  maxKeysPerNode( ) const;
    size_t                                  nodeCount( ) const;

// data structures
private:
    struct Header
    {
        uint32_t                            keySize;                                    // readonly property of file
        uint32_t                            degree;                                     // readonly property of file
        uint32_t                            keyCount;                                   // i.e. size()
        uint32_t                            freeNodeCount;                              // index of head in the free node stack
    };

    using                                   NodeIndex = int32_t;
    struct Node
    {
        NodeIndex                           index;
        std::vector<std::string>            keys;
        std::vector<value_t>                values;
        std::vector<NodeIndex>              kids;
        NodeIndex                           freeNode;
        bool                                isLeaf( ) const
                                                { return kids.empty( ); }
    };

// private interface structures
private:
    struct IDrive
    {
                                            IDrive( BTree & btree );

        size_t                              headerSize( ) const;
        size_t                              nodeSize( ) const;
        size_t                              nodePos( NodeIndex index ) const;
        size_t                              nodeCount( ) const;

        bool                                read32( uint32_t * value ) const;
        bool                                write32( uint32_t value );
        bool                                read64( uint64_t * value ) const;
        bool                                write64( uint64_t value );

        bool                                readHeader( Header * header ) const;
        bool                                writeHeader( Header & header );
        Node                                readNode( NodeIndex index ) const;
        void                                writeNode( const Node & nodeRef );
        void                                pushNode( );
        void                                popNode( );
        void                                pushFreeNode( NodeIndex nodeIndex );
        NodeIndex                           popFreeNode( );
        private: BTree &                    self;
    };
    struct ITree
    {
                                            ITree( BTree & btree );

        struct                              KeyRef { NodeIndex nodeIndex; uint32_t keyIndex; };
        enum class                          RemoveType { Key, Min, Max };
        std::optional<KeyRef>               search( const Node & x, const std::string & key ) const;
        bool                                insert( const std::string & key, value_t value );
        std::optional<value_t>              remove( Node & x, const std::string & key );
        void                                removeNodeKey( Node & node, uint32_t keyIndex, std::string & key, value_t & value );
        bool                                removeKey( Node & x, const std::string & key, value_t & value );
        void                                removeMax( Node & node, std::string & key, value_t & value );
        void                                growChild( Node & node, Node & child, uint32_t index );
        void                                splitChildNode( Node & node, size_t childIndex );
        bool                                insertNonfull( Node & node, const std::string & key, value_t value );
        bool                                insertNonfull( NodeIndex nodeIndex, const std::string & key, value_t value );
        bool                                findKeyIndex( const Node & x, const std::string & key, uint32_t * keyIndex ) const;
        private: BTree &                    self;
    };
    friend struct                           IDrive;
    friend struct                           INodes;
    friend struct                           IGroups;
    friend struct                           ITree;

    IDrive                                  drive;
    ITree                                   tree;

// data
private:
    std::string                             m_path;
    FILE *                                  m_file;
    Header                                  m_header;
    Node                                    m_root;
    size_t                                  m_nodeCount;
};


inline BTree::BTree( std::string filename, uint32_t keylen, uint32_t degree ) :
    m_path( filename ), 
    m_file( nullptr ), 
    m_header{ keylen, degree, 0, 0 }, 
    m_root( ), 
    m_nodeCount( 1 ),
    drive( *this ), 
    tree( *this )
{
    assert( m_header.degree > 1 );
    assert( keylen % 8 == 0 );

    if ( fopen_s( &m_file, m_path.c_str( ), "r+b" ) )
    {
        if ( fopen_s( &m_file, m_path.c_str( ), "w+b" ) )
            { throw std::exception{ "unable to open b-tree file" }; }
    }

    // initialize
    if ( drive.readHeader( &m_header ) )
    {
        m_root = drive.readNode( 0 );
        m_nodeCount = drive.nodeCount( );
    }
    else
    {
        drive.writeHeader( m_header );
        
        m_root.index = 0;
        drive.writeNode( m_root );
    }
}


inline std::optional<BTree::value_t> BTree::get( const std::string & key ) const
{
    auto keyRef = tree.search( m_root, key );
    if ( !keyRef.has_value( ) )
        { return std::optional<value_t>{ }; }
    Node node = drive.readNode( keyRef->nodeIndex );
    return node.values[keyRef->keyIndex];
}


inline bool BTree::put( std::string key, value_t value )
{
    return tree.insert( key, value );
}


std::optional<BTree::value_t> BTree::remove( const std::string & key )
{
    return tree.remove( m_root, key );
}


size_t BTree::size( ) const
{
    return m_header.keyCount;
}


std::vector<std::string> BTree::getKeysInNode( int nodeId )
{
    if ( nodeId == 0 )
        { return m_root.keys; }
    auto node = drive.readNode( nodeId );
    return node.keys;
}


std::vector<int> BTree::getChildrenInNode( int nodeId )
{
    if ( nodeId == 0 )
        { return m_root.kids; }
    auto node = drive.readNode( nodeId );
    return node.kids;
}


std::vector<int> BTree::getFreeNodes( )
{
    std::vector<int> result;

    // find the pos of head of FreeNodeStack
    int freeNodeCount = m_header.freeNodeCount;
    for ( int i = 0; i < freeNodeCount; i++ )
    {
        size_t pos = drive.nodePos( freeNodeCount - i ) + sizeof( uint64_t );

        // read the NodeIndex at the head of the FreeNodeStack
        NodeIndex nodeIndex;
        fseek( m_file, (long)pos, SEEK_SET );
        drive.read32( (uint32_t *)&nodeIndex );
        result.push_back( nodeIndex );
    }

    return result;
}


inline size_t BTree::keySize( ) const
    { return m_header.keySize; }


inline size_t BTree::degree( ) const
    { return m_header.degree; }


inline size_t BTree::freeNodeCount( ) const
    { return m_header.freeNodeCount; }


inline size_t BTree::maxChildrenPerNode( ) const
    { return degree( ) * 2; }


inline size_t BTree::minKeysPerNode( ) const
    { return degree( ) - 1; }


inline size_t BTree::maxKeysPerNode( ) const
    { return degree( ) * 2 - 1; }


inline size_t BTree::nodeCount( ) const
    { return m_nodeCount; }



inline BTree::IDrive::IDrive( BTree & btree )
    : self( btree ) { }


inline size_t BTree::IDrive::headerSize( ) const
{
    assert( sizeof( Header ) == sizeof( uint32_t ) * 4 );
    return sizeof( uint32_t ) * 4;
}


inline size_t BTree::IDrive::nodeSize( ) const
{
    // key count, free node, children[], key[], value[]
    return sizeof( uint32_t ) * 4 + 
        sizeof( NodeIndex ) * self.maxChildrenPerNode( ) + 
        ( self.keySize( ) + sizeof( uint64_t ) ) * self.maxKeysPerNode( );
}



inline size_t BTree::IDrive::nodePos( NodeIndex index ) const
{
    return headerSize( ) + index * nodeSize( );
}


inline size_t BTree::IDrive::nodeCount( ) const
{
    long pos = ftell( self.m_file );
    fseek( self.m_file, 0, SEEK_END );
    long end = ftell( self.m_file );
    fseek( self.m_file, pos, SEEK_SET );

    size_t begin = headerSize( );
    assert( ( ( end - begin ) % nodeSize( ) ) == 0 );

    size_t count = ( end - begin ) / nodeSize( );
    return count;
}


inline bool isBigEndian( ) 
{
    static int x = 0x12345678;
    static bool isBigEndian = ( *( reinterpret_cast<char *>( &x ) ) == 0x12 );
    return isBigEndian;
}


inline bool BTree::IDrive::read32( uint32_t * value ) const
{
    bool result = fread( value, sizeof( uint32_t ), 1, self.m_file ) != 0;
    if ( result && isBigEndian( ) )
        { *value = _byteswap_ulong( *value ); }
    return result;
}


inline bool BTree::IDrive::write32( uint32_t value )
{
    if ( isBigEndian( ) )
        { value = _byteswap_ulong( value ); }
    return fwrite( &value, sizeof( uint32_t ), 1, self.m_file ) != 0;
}


inline bool BTree::IDrive::read64( uint64_t * value ) const
{
    bool result = fread( value, sizeof( uint64_t ), 1, self.m_file ) != 0;
    if ( result && isBigEndian( ) )
        { *value = _byteswap_uint64( *value ); }
    return result;
}


inline bool BTree::IDrive::write64( uint64_t value )
{
    if ( isBigEndian( ) )
        { value = _byteswap_uint64( value ); }
    return fwrite( &value, sizeof( uint64_t ), 1, self.m_file ) != 0;
}


inline bool BTree::IDrive::readHeader( Header * header ) const
{
    fseek( self.m_file, 0, SEEK_SET );
    if ( !read32( (uint32_t *)&( header->keySize ) ) ) return false;
    if ( !read32( (uint32_t *)&( header->degree ) ) ) return false;
    if ( !read32( &( header->keyCount ) ) ) return false;
    if ( !read32( &( header->freeNodeCount ) ) ) return false;
    return true;
}


inline bool BTree::IDrive::writeHeader( Header & header )
{
    fseek( self.m_file, 0, SEEK_SET );
    if ( !write32( (uint32_t)header.keySize ) ) return false;
    if ( !write32( (uint32_t)header.degree ) ) return false;
    if ( !write32( header.keyCount ) ) return false;
    if ( !write32( header.freeNodeCount ) ) return false;
    return true;
}


inline BTree::Node BTree::IDrive::readNode( NodeIndex index ) const
{
    assert( index >= 0 && index < self.nodeCount( ) );

    BTree::Node node;
    node.index = index;

    size_t posOfNode = nodePos( index );
    size_t nodeHeaderSize = sizeof( uint32_t ) * 4;
    size_t posOfKeys = posOfNode + nodeHeaderSize + sizeof( NodeIndex ) * self.maxChildrenPerNode( );
    size_t posOfValues = posOfKeys + self.keySize( ) * self.maxKeysPerNode( );

    fseek( self.m_file, (long)posOfNode, SEEK_SET );

    bool result = true;
    uint32_t keyCount, kidCount, padding;
    result &= read32( &keyCount );
    result &= read32( &kidCount );
    result &= read32( (uint32_t *)&( node.freeNode ) );
    result &= read32( &padding );

    for ( uint32_t i = 0; i < kidCount; i++ )
    {
        NodeIndex childIndex;
        result &= read32( (uint32_t *)&childIndex );
        node.kids.push_back( childIndex );
    }

    // seek to start of keys
    fseek( self.m_file, (long)posOfKeys, SEEK_SET );

    std::string keyBuffer( self.keySize( ), '\0' );
    for ( uint32_t i = 0; i < keyCount; i++ )
    {
        result &= ( fread( keyBuffer.data( ), self.keySize( ), 1, self.m_file ) != 0 );
        int len = keyBuffer[0];
        node.keys.push_back( keyBuffer.substr( 1, len ) );
    }

    // seek to start of values
    fseek( self.m_file, (long)posOfValues, SEEK_SET );

    uint64_t value;
    for ( uint32_t i = 0; i < keyCount; i++ )
    {
        result &= read64( &value );
        node.values.push_back( value );
    }

    assert( result == true );
    return node;
}


inline void BTree::IDrive::writeNode( const Node & node )
{
    assert( node.index >= 0 && node.index < self.nodeCount( ) );

    size_t posOfNode = nodePos( node.index );
    size_t nodeHeaderSize = sizeof( uint32_t ) * 4;
    size_t posOfKeys = posOfNode + nodeHeaderSize + sizeof( NodeIndex ) * self.maxChildrenPerNode( );
    size_t posOfValues = posOfKeys + self.keySize( ) * self.maxKeysPerNode( );

    fseek( self.m_file, (long)posOfNode, SEEK_SET );

    bool result = true;
    result &= write32( (uint32_t)node.keys.size( ) );
    result &= write32( (uint32_t)node.kids.size( ) );
    result &= write32( (uint32_t)node.freeNode );
    result &= write32( 0 );

    for ( auto node : node.kids )
        { result &= write32( (uint32_t)node ); }

    // seek to start of keys
    std::string keyBuffer;
    fseek( self.m_file, (long)posOfKeys, SEEK_SET );
    for ( const auto & key : node.keys )
    {
        keyBuffer.clear( );
        keyBuffer += (char)key.size( );
        keyBuffer += key;
        keyBuffer.resize( self.keySize( ) );
        result &= ( fwrite( keyBuffer.data( ), self.keySize( ), 1, self.m_file ) != 0 );
    }

    // seek to start of values
    fseek( self.m_file, (long)posOfValues, SEEK_SET );
    for ( auto value : node.values )
    {
        result &= write64( value );
    }

    assert( result == true );
    fflush( self.m_file );
}


inline void BTree::IDrive::pushNode( )
{
    // seek to the end of the file
    fseek( self.m_file, 0, SEEK_END );
    long end = ftell( self.m_file );

    NodeIndex nodeIndex = (NodeIndex)self.m_nodeCount++;
    size_t count = nodeSize( ) / sizeof( uint64_t );
    for ( size_t i = 0; i < count; i++ )
        { write64( 0 ); }
    
    pushFreeNode( nodeIndex );
}


inline void BTree::IDrive::popNode( )
{

}


inline void BTree::IDrive::pushFreeNode( NodeIndex nodeIndex )
{
    // get the head of the FreeNodeStack
    int freeNodeCount = self.m_header.freeNodeCount;

    // find the pos of head of FreeNodeStack - don't use the root node for FreeNodeStack
    size_t pos = nodePos( 1 + freeNodeCount ) + sizeof( uint64_t );

    // add nodeIndex to the top of FreeNodeStack
    fseek( self.m_file, (long)pos, SEEK_SET );
    bool result = write32( nodeIndex );

    // update the freeNodeCount in the header
    self.m_header.freeNodeCount++;
    result &= writeHeader( self.m_header );

    assert( result );
}


inline BTree::NodeIndex BTree::IDrive::popFreeNode( )
{
    // When the FreeNodeStack is empty, this is indication to grow the file by creating another node
    if ( self.m_header.freeNodeCount == 0 )
        { pushNode( ); }
    assert( self.m_header.freeNodeCount > 0 );

    // find the pos of head of FreeNodeStack
    int freeNodeCount = self.m_header.freeNodeCount - 1;
    size_t pos = nodePos( 1 + freeNodeCount ) + sizeof( uint64_t );

    // read the NodeIndex at the head of the FreeNodeStack
    NodeIndex nodeIndex;
    fseek( self.m_file, (long)pos, SEEK_SET );
    bool result = read32( (uint32_t *)&nodeIndex );

    // update the freeNodeCount in the header
    self.m_header.freeNodeCount = freeNodeCount;
    result &= writeHeader( self.m_header );

    assert( result );
    return nodeIndex;
}


inline BTree::ITree::ITree( BTree & btree )
    : self( btree ) { }


//!
//! B-TREE-SEARCH(x,k)
//! 1 i = 1
//! 2 while i <= x.n and k > x.key[i]
//! 3     i = i + 1
//! 4 if i <= x.n and k == x.key[i]
//! 5     return (x,i)
//! 6 elseif x.leaf
//! 7     return NIL
//! 8 else DISK-READ(x.c[i])
//! 9     return B-TREE-SEARCH(x.c[i], k)
std::optional<BTree::ITree::KeyRef> BTree::ITree::search( const Node & x, const std::string & key ) const
{
    Node nodeData;
    const Node * node = &x;
    while ( true )
    {
        uint32_t i;
        if (findKeyIndex( *node, key, &i ))
            { return KeyRef{ node->index, i }; }
        if ( node->isLeaf( ) )
            { return std::optional<BTree::ITree::KeyRef>{ }; }
        nodeData = self.drive.readNode( node->kids[i] );
        node = &nodeData;
    }
}


//! B-TREE-INSERT(T,k)
//! 1  r = T.root
//! 2  if r.n === 2t - 1
//! 3      s = ALLOCATE-NODE()
//! 4      T.root = s
//! 5      s.leaf = FALSE
//! 6      s.n = 0
//! 7      s.c[1] = r
//! 8      B-TREE-SPLIT-CHILD(s,1)
//! 9      B-TREE-INSERT-NONFULL(s,k)
//! 10 else B-TREE-INSERT-NONFULL(r,k)
bool BTree::ITree::insert( const std::string & key, value_t value )
{
    Node & root = self.m_root;
    if ( root.keys.size( ) == self.maxKeysPerNode( ) )
    {
        // duplicate root as new node
        NodeIndex nodeIndex = self.drive.popFreeNode( );
        Node s = self.drive.readNode( nodeIndex );
        assert( s.index == nodeIndex );
        s.keys = std::move( root.keys );
        s.values = std::move( root.values );
        s.kids = std::move( root.kids );
        self.drive.writeNode( s );

        // set root to empty except for single child
        assert( root.keys.empty( ) );
        assert( root.values.empty( ) );
        assert( root.kids.empty( ) );
        root.kids.push_back( s.index );
        self.drive.writeNode( root );

        splitChildNode( root, 0 );
    }
    return insertNonfull( root, key, value );

}


std::optional<BTree::value_t> BTree::ITree::remove( Node & node, const std::string & key )
{
    value_t value;
    std::optional<value_t> result = removeKey( node, key, value )
        ? value
        : std::optional<value_t>{ };

    // handle the case of the last key of root being merged into its child
    if ( node.index == 0 && node.keys.size( ) == 0 && node.kids.size( ) != 0 )
    {
        assert( &node == &self.m_root );
        assert( node.kids.size( ) == 1 );
        auto child = self.drive.readNode( node.kids[0] );
        node.keys = std::move( child.keys );
        node.values = std::move( child.values );
        node.kids = std::move( child.kids );
        
        self.drive.writeNode( child );
        self.drive.writeNode( node );
        self.drive.pushFreeNode( child.index );
    }
    return result;
}


void BTree::ITree::removeNodeKey( Node & node, uint32_t keyIndex, std::string & key, value_t & value )
{
    key = std::move( node.keys[keyIndex] );
    value = node.values[keyIndex];
    // update node
    node.keys.erase( node.keys.begin( ) + keyIndex );
    node.values.erase( node.values.begin( ) + keyIndex );
    self.drive.writeNode( node );
    // update header
    self.m_header.keyCount--;
    self.drive.writeHeader( self.m_header );
}


bool BTree::ITree::removeKey( Node & node, const std::string & key, value_t & value )
{
    uint32_t index = 0;
    bool hasKey = findKeyIndex( node, key, &index );

    if ( node.isLeaf( ) )
    {
        if ( !hasKey )
            { return false; }

        std::string removedKey;
        removeNodeKey( node, index, removedKey, value );
        assert( removedKey == key );
        return true;
    }

    auto child = self.drive.readNode( node.kids[index] );
    if ( child.keys.size( ) <= self.minKeysPerNode( ) )
    { 
        growChild( node, child, index );
        return removeKey( node, key, value ); 
    }

    if ( hasKey )
    {
        assert( key == node.keys[index] );
        value = node.values[index];

        removeMax( child, node.keys[index], node.values[index] );
        return true;
    }

    return removeKey( child, key, value );
}


void BTree::ITree::removeMax( Node & node, std::string & key, value_t & value )
{
    bool hasKey = true;
    uint32_t index = (uint32_t)node.keys.size( ) - 1;

    if ( node.isLeaf( ) )
    {
        removeNodeKey( node, index, key, value );
    }
    else
    {
        index++;
        auto child = self.drive.readNode( node.kids[index] );
        if ( child.keys.size( ) <= self.minKeysPerNode( ) )
        {
            growChild( node, child, index ); 
            removeMax( node, key, value ); 
            return;
        }

        removeMax( child, key, value );
    }
}


void BTree::ITree::growChild( Node & node, Node & child, uint32_t index )
{
    bool notLeftMostChild = index > 0;
    bool notRightMostChild = index < node.kids.size( ) - 1;
    Node leftChild, rightChild;

    if ( notLeftMostChild )
    {
        // tryBorrowFromLeft
        leftChild = self.drive.readNode( node.kids[index - 1] );
        if ( leftChild.keys.size( ) > self.minKeysPerNode( ) )
        {
            // borrowFromLeft
            child.keys.insert( child.keys.begin( ), node.keys[index - 1] );
            child.values.insert( child.values.begin( ), node.values[index - 1] );
            node.keys[index - 1] = leftChild.keys.back( );
            node.values[index - 1] = leftChild.values.back( );
            leftChild.keys.pop_back( );
            leftChild.values.pop_back( );

            if ( !leftChild.isLeaf( ) )
            {
                child.kids.insert( child.kids.begin( ), leftChild.kids.back( ) );
                leftChild.kids.pop_back( );
            }

            self.drive.writeNode( leftChild );
            self.drive.writeNode( child );
            self.drive.writeNode( node );
            return;
        }
    }
    if ( notRightMostChild )
    {
        // tryBorrowFromRight
        rightChild = self.drive.readNode( node.kids[index + 1] );
        if ( rightChild.keys.size( ) > self.minKeysPerNode( ) )
        {
            // borrowFromRight
            child.keys.insert( child.keys.end( ), node.keys[index] );
            child.values.insert( child.values.end( ), node.values[index] );
            node.keys[index] = rightChild.keys.front( );
            node.values[index] = rightChild.values.front( );
            rightChild.keys.erase( rightChild.keys.begin() );
            rightChild.values.erase( rightChild.values.begin( ) );

            if ( !rightChild.isLeaf( ) )
            {
                child.kids.insert( child.kids.end( ), rightChild.kids.front( ) );
                rightChild.kids.erase( rightChild.kids.begin( ) );
            }

            self.drive.writeNode( rightChild );
            self.drive.writeNode( child );
            self.drive.writeNode( node );
            return;
        }
    }
    // merge
    Node * left;
    Node * right;

    // simplify merge by always merging from right node to left node
    if ( notRightMostChild )
    {
        left = &child;
        right = &rightChild;
    }
    else
    {
        // if looking at the right-most node, perform merge from the preceeding child
        index--;
        right = &child;
        left = &leftChild;
    }
    // index identifies the key in node which will be merged into the left child
    left->keys.push_back( node.keys[index] );
    left->values.push_back( node.values[index] );
    node.keys.erase( node.keys.begin( ) + index );
    node.values.erase( node.values.begin( ) + index );
    node.kids.erase( node.kids.begin( ) + index + 1 );

    left->keys.insert( left->keys.end( ), right->keys.begin( ), right->keys.end( ) );
    left->values.insert( left->values.end( ), right->values.begin( ), right->values.end( ) );
    left->kids.insert( left->kids.end( ), right->kids.begin( ), right->kids.end( ) );

    right->keys.clear( );
    right->values.clear( );
    right->kids.clear( );


    self.drive.writeNode( *right );
    self.drive.writeNode( *left );
    self.drive.writeNode( node );
    self.drive.pushFreeNode( right->index );
}


//! B-TREE-INSERT-NONFULL(x,k)
//! 1  i = x.n
//! 2  if x.leaf
//! 3      while i >= 1 and k < x:key[i]
//! 4          x.key[i+1] = x.key[i]
//! 5              i = i - 1
//! 6      x.key[i+1] = k
//! 7      x.n = x.n + 1
//! 8      DISK-WRITE(x)
//! 9  else while i >= 1 and k < x.key[i]
//! 10         i = i - 1
//! 11     i = i + 1
//! 12     DISK-READ(x.c[i])
//! 13     if x.c[i].n == 2t - 1
//! 14         B-TREE-SPLIT-CHILD(x,i)
//! 15         if k > x.key[i]
//! 16             i = i + 1
//! 17     B-TREE-INSERT-NONFULL(x.c[i],k)
bool BTree::ITree::insertNonfull( Node & x, const std::string & key, value_t value )
{
    size_t n = x.keys.size( );
    int index = (int)n - 1;
    if ( x.isLeaf( ) )
    {
        while ( index >= 0 )
        {
            auto result = key <=> x.keys[index];
            if ( result == std::strong_ordering::equal )
                { x.values[index] = value; self.drive.writeNode( x ); return false; }
            if ( result == std::strong_ordering::greater )
                { break; }
            index--;
        }
        index++;
        x.keys.insert( x.keys.begin( ) + index, key );
        x.values.insert( x.values.begin( ) + index, value );
        self.drive.writeNode( x );

        self.m_header.keyCount++;
        self.drive.writeHeader( self.m_header );

        return true;
    }
    else
    {
        while ( index >= 0 )
        {
            auto result = key <=> x.keys[index];
            if ( result == std::strong_ordering::equal )
                { x.values[index] = value; return false; }
            if ( result == std::strong_ordering::greater )
                { break; }
            index--;
        }
        index++;
        auto child = self.drive.readNode( x.kids[index] );
        if ( child.keys.size( ) == self.maxKeysPerNode( ) )
        {
            splitChildNode( x, index );
            auto result = key <=> x.keys[index];
            if ( result == std::strong_ordering::greater ) 
                { index++; }
        }
        return insertNonfull( x.kids[index], key, value );
    }
}


bool BTree::ITree::insertNonfull( NodeIndex nodeIndex, const std::string & key, value_t value )
{
    auto node = self.drive.readNode( nodeIndex );
    return insertNonfull( node, key, value );
}


//! B-TREE-SPLIT-CHILD(x,i)
//! 1  z = ALLOCATE-NODE()
//! 2  y = x.c[i]
//! 3  z.leaf = y.leaf
//! 4  z.n = t - 1
//! 5  for j = 1 to t - 1
//! 6      z.key[j] = y.key[j+1]
//! 7  if not y.leaf
//! 8      for j = 1 to t
//! 9          z.c[j] = y.c[j+1]
//! 10 y.n = t - 1
//! 11 for j = x.n + 1 downto i + 1
//! 12     x.c[j+1] = x.c[j]
//! 13 x.c[i+1] = z
//! 14 for j = x.n downto i
//! 15     x.key[j+1] = x.key[j]
//! 16 x.key[i] = y.key[t]
//! 17 x.n = x.n + 1
//! 18 DISK-WRITE(x)
//! 19 DISK-WRITE(y)
//! 20 DISK-WRITE(z)
void BTree::ITree::splitChildNode( Node & x, size_t childIndex )
{
    // node must contain entry for childIndex
    assert( childIndex < x.kids.size( ) );

    size_t degree = self.degree( );
    size_t splitIndex = degree - 1;

    // create z
    NodeIndex zIndex = self.drive.popFreeNode( );
    Node z = self.drive.readNode( zIndex );
    assert( z.index == zIndex );
    // read y
    auto y = self.drive.readNode( x.kids[childIndex] );

    // childNode should be full
    assert( y.keys.size( ) == self.maxKeysPerNode( ) );

    // insert z as child of x
    x.kids.insert( x.kids.begin( ) + childIndex + 1, z.index );
    // insert y's key/value at split into parent
    x.keys.insert( x.keys.begin( ) + childIndex, y.keys[splitIndex] );
    x.values.insert( x.values.begin( ) + childIndex, y.values[splitIndex] );

    // move right-most keys from childNode to newNode
    for ( int i = 0; i < splitIndex; i++ )
    { 
        z.keys.push_back( y.keys[degree + i] );
        z.values.push_back( y.values[degree + i] );
    }
    if ( !y.isLeaf() )
    {
        assert( y.kids.size( ) == self.maxChildrenPerNode( ) );
        for ( int i = 0; i < degree; i++ )
            { z.kids.push_back( y.kids[degree + i] ); }
        y.kids.resize( degree );
    }
    y.keys.resize( splitIndex );
    y.values.resize( splitIndex );

    self.drive.writeNode( x );
    self.drive.writeNode( y );
    self.drive.writeNode( z );
}

//! Returns the lower bound of the key among the node's keys
//! If the key is present, keyIndex will point to the key in node and returns true.
//! If the key is not present, keyIndex will point to the index where the key would be and returns false.
bool BTree::ITree::findKeyIndex( const Node & node, const std::string & key, uint32_t * keyIndex ) const
{
    uint32_t i = 0;
    while ( i < node.keys.size( ) )
    {
        auto result = key <=> node.keys[i];
        if ( result == std::strong_ordering::equal )
            { *keyIndex = i; return true; }
        if ( result == std::strong_ordering::less )
            { break; }
        i++;
    }
    *keyIndex = i;
    return false;
}