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
#include <sys/wait.h>
#include <iostream>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

//global variables
const int PORT = 60154;
const int BACKLOG = 20;
const int MAXDATASIZE = 40000;
const int MAXCLIENTS = 15;

int connectToClients( int s, int &new_s, struct sockaddr_in my_addr, int i );
int setUpSocket( int &s, struct sockaddr_in &my_addr );

void sendData( int new_s, int i );
int receiveFromClient( char * directory, int new_s, int i );
int sendToClient( char * directory, int new_s, int i );
int sendFileToClient( char * directory, int new_s, dirent *direntp );
int sendFileNameToClient( int new_s, dirent *direntp );
int sendFileContentsToClient( int new_s, int input );

int main() 
{
    int s, new_s;
    struct sockaddr_in my_addr;
    int pids[MAXCLIENTS];
    if ( setUpSocket(s, my_addr) == -1 )
        exit(1);

    int i;
    for ( i = 0; i < MAXCLIENTS; ++i )
    {
        //listen for connection
        if ( listen(s, BACKLOG) == -1 )
        {
            cerr << "Listen server failed, exiting." << endl;
            return -1;
        } 

        int pid;
        pid = fork();
        switch(pid)
        {
            case -1:
                cerr << "Fork failed" << endl;
                _exit(1);
            case 0:
                // cout << "Listen server succeeded." << endl;
                if ( connectToClients( s, new_s, my_addr, i ) == -1 ) 
                    _exit(0);
                sendData(new_s, i);
                close(new_s);
                _exit(0);
            default:
                pids[i] = pid;
                break;
        }
    }

    for ( int j = 0; j < i; ++j )
    {
        int stat;
        waitpid( pids[j], &stat, 0 );
    }
    close(s);


    return 0;
}

int connectToClients( int s, int &new_s, struct sockaddr_in my_addr, int i )
{
    //accept connection to client
    int sun_size = sizeof(struct sockaddr_un);
    if( (new_s = accept(s, (struct sockaddr *)&my_addr, (socklen_t *)&sun_size)) == -1)
    {
        cerr << "Accept server failed, exiting." << endl;
        return -1;
    } 
    // cout << "Accept server succeeded." << endl;
    cout << "Connected to client " << i << endl;
    return 0;
}

//create socket and and bind it
int setUpSocket( int &s, struct sockaddr_in &my_addr )
{
    //create AF_INET socket to liten for incoming connections
    if( (s = socket(AF_INET, SOCK_STREAM, 0)) ==-1)
    {
        cerr << "Socket server failed, exiting." << endl;
        return -1;
    } 
    // cout << "Socket server has been created." << endl;

    memset( &(my_addr),0,sizeof(my_addr) );
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind the socket
    for ( int i = 0; i < 10 && bind(s, (struct sockaddr *)&my_addr, sizeof(struct sockaddr) ) == -1; ++i )
    {
        if ( i == 9 )
        {
            cerr << "Bind server failed, exiting." << endl;
            close(s);
            return -1;
        }
        cout << "Bind server failed, retrying." << endl;
        sleep(1);
    }
    // cout << "Bind server succeeded." << endl;
    
    return 0;
}

//send and receive data from and to the client
void sendData( int new_s, int i )
{
    //receive the name of the directory
    char * directory = new char[MAXDATASIZE];
    if ( receiveFromClient( directory, new_s, i ) == -1 )
        return;

    sleep(1);

    if ( sendToClient(directory, new_s, i) == -1 )
        return;
    delete [] directory;     //deallocate memory

    return;
}

//receive directory path from client
int receiveFromClient( char * directory, int new_s, int i ) 
{
    //read the directory that the client wants
    int bytes_read;
    if ( (bytes_read=read( new_s, directory, MAXDATASIZE-1 )) < 0 ) 
    {
        cerr << "Read from client " << i << " error, exiting." << endl;
        return -1;
    }
    directory[bytes_read] = '\0';
    // cout << endl << bytes_read << " bytes were read from client." << endl;
    // cout << "Server read " << directory << " from client." << endl << endl;
    cout << "Sending client " << i << " copy of " << directory << endl;

    return 0;
}

//send each file to the client
int sendToClient( char * directory, int new_s, int i ) 
{
    string directory_str = directory;
    DIR *dirp;

    //print if directory can't be opened
    if (!(dirp = opendir(directory))) {
        cerr << "Error(" << errno << ") opening " << directory << endl;
        return -1; 
    }

    //goes through each file in the directory
    dirent *direntp;
    while ( (direntp = readdir(dirp)) ) {

        //skip hidden files
        if ( direntp->d_name[0] == '.' )
            continue;

        //use stat here to find subdirectories
        struct stat buf;
        if ( stat((directory_str+"/"+direntp->d_name).c_str(), &buf) != -1)
        {
            if ( buf.st_mode & S_IFDIR )        //skip directories
                continue;
            if ( !(buf.st_mode & S_IROTH) )     //skip files that I don't
                continue;                       //have access to
        }
        
        cout << "  Sending client " << i << " " << (directory_str+"/"+direntp->d_name) << endl;
        if ( sendFileToClient(directory, new_s, direntp) == -1 )
            return -1;

        // break;
    }

    //close each directory
    closedir(dirp);

    //send end of directory

    return 0;
}

//sends a single file to the client
int sendFileToClient( char * directory, int new_s, dirent *direntp )
{
    string directory_str = directory;

    if ( sendFileNameToClient(new_s,direntp) == -1 )
        return -1;

    //wait for client to read the file name
    sleep(1);

    //open file descriptor for file
    int input = open( (directory_str+"/"+direntp->d_name).c_str(), O_RDONLY );
    if ( input == -1 )
    {
        cerr << "Error opening " << (directory_str+"/"+direntp->d_name) << endl;
        return -1;
    }
    // cout << "Server opened " << (directory_str+"/"+direntp->d_name) << endl;

    if ( sendFileContentsToClient(new_s, input) == -1 )
        return -1;

    // cout << endl;

    sleep(1);

    close(input);
    return 0;
}

//send the name of the file to the client
int sendFileNameToClient( int new_s, dirent *direntp )
{
    //send the name of the file to the client
    if ( write( new_s, direntp->d_name, strlen(direntp->d_name) ) == -1 )
    {
        cerr << "Writing name of file failed" << endl;
        return -1;
    }
    // cout << strlen(direntp->d_name) << " bytes were written to client." << endl;
    // cout << "Server wrote " << direntp->d_name << " to client." << endl;

    return 0;
}

int sendFileContentsToClient( int new_s, int input )
{
    //read the file contents in buf
    char buf[MAXDATASIZE];
    int bytes_read;
    if ( (bytes_read = read( input, buf, MAXDATASIZE-1 )) == -1 )
    {
        cerr << "Error reading file" << endl;
        return -1;
    }
    buf[bytes_read] = '\0';
    // cout << "Read from file" << endl;

    //write the file contents to the client
    if ( write( new_s, buf, strlen(buf) ) == -1 )
    {
        cerr << "Error writing to client" << endl;
        return -1;
    }

    if ( write( new_s, " ", strlen(" ") ) == -1 )
    {
        cerr << "Error writing to client" << endl;
        return -1;
    }
    // cout << "Wrote to socket" << endl << endl;
    return 0;
}
