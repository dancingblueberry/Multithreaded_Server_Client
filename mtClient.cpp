/*
* Course: CS 100 Fall 2013 *
* First Name: Amanda
* Last Name: Berryhill
* Username: aberr005
* email address: aberr005@ucr.edu
*
* Assignment: 9
* I hereby certify that the contents of this file represent
* my own original individual work. Nowhere herein is there
* code from any outside resources such as another individual,
* a website, or publishings unless specifically designated as
* permissible by the instructor or TA. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include <pthread.h>

using namespace std;

const int PORT = 60154;
const int BACKLOG = 10;
const int MAXDATASIZE = 40000;
const int THREAD_COUNT_MAX = 10;
pthread_t THREAD_IDS[THREAD_COUNT_MAX];
char * DIRECTORY_NAMES[] = 
{
    "threadfiles0",
    "threadfiles1",
    "threadfiles2",
    "threadfiles3",
    "threadfiles4",
    "threadfiles5",
    "threadfiles6",
    "threadfiles7",
    "threadfiles8",
    "threadfiles9"
};

struct thread_args
{
    int thread_count;
    int argc;
    char **argv;
};

void * client( void * params );
int setUpSocket( int &s, struct sockaddr_in &my_addr, char *argv[], int i );

int sendData( int s, int argc, char *argv[], int i );
int sendToServer( int s, int num, char *argv[], int i );
int receiveFromServer( int s, char * directory, int i );

int main( int argc, char **argv ) 
{
    if ( argc < 3 ) 
    {
        cerr << "Client: 2 arguments needed, exiting." << endl;
        exit(1);
    }
    
    thread_args thread_args_array[THREAD_COUNT_MAX];
    for ( int i = 0; i < THREAD_COUNT_MAX; ++i )
    {
        thread_args_array[i].argc = argc;
        thread_args_array[i].argv = argv;
        thread_args_array[i].thread_count = i;
        pthread_create( &THREAD_IDS[i], NULL, client, &thread_args_array[i] );
    }   
    for ( int i = 0; i < THREAD_COUNT_MAX; ++i )
        pthread_join( THREAD_IDS[i], NULL );

    return 0;
}

void * client( void * params )
{
    struct thread_args * arguments = (struct thread_args *)params;
    int i = arguments->thread_count;
    int argc = arguments->argc;
    char **argv = arguments->argv;

    // cout << "Client " << i << " is here!" << endl;
    
    int s;
    struct sockaddr_in my_addr;
    if ( setUpSocket(s, my_addr, argv, i) == -1 )
        return NULL;

    // cout << "Now client " << i << " is there!" << endl;
    
    sendData(s, argc, argv, i);
    close(s);
    
    return NULL;
}

int setUpSocket( int &s, struct sockaddr_in &my_addr, char *argv[], int i ) 
{
    //create AF_INET socket to listen for incoming connections
    if( (s = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        cerr << "Socket client " << i << " failed, exiting." << endl;
        return -1;
    }
    // cout << "Socket client has been created." << endl;

    memset( &(my_addr),0,sizeof(my_addr) );
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    struct hostent * hostname;
    if ( (hostname = gethostbyname( argv[1] )) == NULL ) 
    {
        cerr << "Invalid hostname, exiting." << endl;
        return -1;
    } 
    // cout << "Hostname: " << argv[1] << endl;
    
    for ( int t = 0; 
          t < 10 && connect(s, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)); 
          ++t )
    {
        if ( t == 9 )
        {
            cerr << "Connect client " << i << " failed, exiting." << endl;
            return -1;
        }
        cout << "Connect client " << i << " failed, retrying." << endl;
    } 
    // cout << "Connect client succeeded." << endl;

    return 0;
}

int sendData( int s, int argc, char *argv[], int i ) 
{
    if ( sendToServer( s, 2, argv, i ) == -1 )
        return -1;

    //create the directory 
    char mkdir[] = "mkdir ./";
    strcat( mkdir, DIRECTORY_NAMES[i] );
    system( mkdir );
    
    cout << "Client " << i << " created directory: " << DIRECTORY_NAMES[i] << endl;

    if ( receiveFromServer(s, DIRECTORY_NAMES[i], i ) == -1 )
        return -1;

    return 0;
}

int sendToServer( int s, int num, char *argv[], int i ) 
{
    if ( write(s,argv[num],strlen(argv[num])) == -1 )
    {
        cerr << "Writing to server failed, exiting." << endl;
        return -1;
    }
    // cout << endl << strlen(argv[num]) << " bytes were written to server." << endl;
    // cout << "Client wrote " << argv[num] << " to server." << endl << endl;
    cout << "Client " << i << ": send request for " << argv[num] << endl;

    return 0;
}

int receiveFromServer( int s, char * directory, int i )
{
    char name[MAXDATASIZE];
    int bytes_read = 0;

    while ( (bytes_read = read(s, name, MAXDATASIZE-1 )) > 0 ) 
    {
        name[bytes_read] = '\0';
        // cout << bytes_read << " bytes were read from server." << endl;
        // cout << "Client read " << name << " from server." << endl;
        cout << "  Client " << i << ": receiving " << name << endl;

        //get the name of the directory
        char * directory_name = new char[100];
        int j = 0;
        for( int k = 0; directory[k]; ++k )
            if ( directory[k] == '/' )
                j = k+1;
        strcpy( directory_name, directory+j );

        sleep(1);

        strcat( directory_name, "/" );
        strcat( directory_name, name );
        int output = open( directory_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
        if ( output == -1 )
        {
            cerr << "Client " << i << ": " << directory_name << " could not be opened" << endl;
            char buf[MAXDATASIZE];
            return -1;
        }
        // cout << "Client opened " << directory_name << endl;

        char buf[MAXDATASIZE];
        if ( (bytes_read = read(s, buf, MAXDATASIZE-1 )) == -1 )
        {
            cerr << "Client " << i << ": Reading from server failed, exiting" << endl;
            return -1;
        }
        buf[bytes_read] = '\0';
        // cout << bytes_read << " bytes were read from server." << endl;
        // cout << "Client read:\n" << buf << endl;

        if ( write(output, buf, strlen(buf)) == -1 )
        {
            cerr << "Client " << i << ": " << directory_name 
                 << " could not be written" << endl;
            return -1;
        }

        delete [] directory_name;
        close(output);
        // cout << endl;

        sleep(1);
    }
    if ( bytes_read == -1 )
    {
        cerr << "Client " << i << ": Reading from server failed, exiting" << endl;
        return -1;
    }

    return 0;
}
