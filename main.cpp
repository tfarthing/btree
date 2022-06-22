#include <cassert>
#include <conio.h>
#include <chrono>
#include <filesystem>

#include "btree.h"



std::string printBTree( BTree& btree );


void printTime()
{
    std::string time = std::format("{}\n", std::chrono::system_clock::now());
    printf(time.c_str());
}


// This sample will randomly select a key to put or remove to a btree in a loop.
// The BTree is printed out every 10k ops
int main(int argc, char** argv)
{
    try 
    {
        std::string filename = "test.btree";
        std::filesystem::remove( filename );

        BTree btree{ filename, 2 };                 // small tree that is easy to visualize using small number of keys
        //BTree btree{ filename, 128 };             // larger tree-nodes useful for a lot ok keys

        uint32_t seed = 0;                          // using fixed seed to generate same tree sequence each run
        //uint32_t (uint32_t)time( nullptr );
        srand( seed );

        int atoz = 'z' + 1 - 'a';

        std::string lastOp;                         // stores the tree visualization from the last operation, useful for debugging
        for ( int i = 0; i < 1000000; i++ )
        {
            std::string op;

            char c1 = ( rand( ) % ( atoz ) ) + 'a';
            //int c2 = ( rand( ) % ( atoz ) ) + 'a';
            //int c3 = ( rand( ) % ( atoz ) ) + 'a';
            std::string key{ c1 };
            //std::string key{ c1, c2, c3 };        // uncomment to use a larger key set
            op += std::format( "#{} >> '{}'", i, key );
            
            if ( btree.remove( key ) )
            { 
                op += " removed <<\n";
            }
            else
            { 
                btree.put( key, i ); 
                op += " added <<\n";
            }

            if ( ( i % 10000 ) == 0 )
            {
                op += printBTree( btree ) + "\n";
                
                printTime();
                printf( op.c_str( ) );
            }
            //op += printBTree( btree ) + "\n";     // uncomment to visualize the tree with every op
            lastOp = op;
        }
        printTime();
        printf( lastOp.c_str( ) );
    }
    catch ( std::exception & e )
    {
        printf( e.what( ) );
    }
}


std::string printBTree( BTree & btree )
{
    std::string result;

    std::vector<int> nodes = { 0 };
    while ( !nodes.empty( ) )
    {
        std::vector<int> kids;
        for ( int nodeIndex : nodes )
        {
            auto children = btree.getChildrenInNode( nodeIndex );
            kids.insert( kids.end( ), children.begin( ), children.end( ) );

            result += std::format( "{}:[", nodeIndex );

            if ( btree.degree( ) < 4 )
            {
                bool initial = true;
                for ( auto & key : btree.getKeysInNode( nodeIndex ) )
                {
                    result += std::format( "{}{}", initial ? "" : ", ", key.c_str( ) );
                    initial = false;
                }
            }
            else
            {
                auto keys = btree.getKeysInNode( nodeIndex );
                result += std::format( "{}...{}", keys.front( ), keys.back( ) );
            }

            result += "], \t";
        }

        nodes = std::move( kids );

        result += "\n";
    }

    result += std::format( "size='{}' free='", (int)btree.size( ) );
    bool initial = true;
    for ( auto i : btree.getFreeNodes( ) )
        { result += std::format( "{}{}", initial ? "" : ", ", i ); initial = false; }
    result += "' \n";
    return result;
}

