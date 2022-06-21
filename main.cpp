#include <cassert>
#include <conio.h>
#include <chrono>
#include <filesystem>

#include "btree.h"


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


void put( BTree & btree, std::string keys )
{
    for ( int i = 0; i < keys.size( ); i++ )
        { btree.put( keys.substr( i, 1 ), 0 ); }

}


int main(int argc, char** argv)
{
    try 
    {
        std::string filename = "test.btree";
        std::filesystem::remove( filename );

        BTree btree{ filename, 128, 2 };
        //BTree btree{ filename, 128, 128 };

        uint32_t seed = 0; //(uint32_t)time( nullptr );
        srand( seed );

        int atoz = 'z' + 1 - 'a';

        std::string lastOp;
        for ( int i = 0; i < 1000000; i++ )
        {
            std::string op;

            int n1 = ( rand( ) % ( atoz ) ) + 'a';
            //int n2 = ( rand( ) % ( atoz ) ) + 'a';
            //int n3 = ( rand( ) % ( atoz ) ) + 'a';
            std::string k{ (char)n1 };
            //std::string k{ (char)n1, (char)n2, (char)n3 };
            op += std::format( "#{} >> '{}'", i, k );
            
            auto value = btree.remove( k );
            if ( value.has_value( ) )
            { 
                op += " removed <<\n";
            }
            else
            { 
                btree.put( k, i ); 
                op += " added <<\n";
            }

            if ( (i % 10000) == 0 )
            {
                op += printBTree( btree ) + "\n";

                std::string time = std::format("{}\n",std::chrono::system_clock::now( ));
                printf( time.c_str( ) );
                printf( op.c_str( ) );
            }
            //op += printBTree( btree ) + "\n";
            lastOp = op;
        }
        std::string time = std::format( "{}\n", std::chrono::system_clock::now( ) );
        printf( time.c_str( ) );
        printf( lastOp.c_str( ) );
    }
    catch ( std::exception & e )
    {
        printf( e.what( ) );
    }
}