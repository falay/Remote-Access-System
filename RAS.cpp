#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <dirent.h>
#include <sstream>
#include <vector>
#include "RAS.hpp"


#define BUFSIZE 1024
#define QLEN    5
#define PERM	0666

using namespace std ;


RAS::RAS(int port)
{
	int slaveSock, pid ;
	int masterSock = passiveSock( port ) ;
	struct sockaddr_in clientAddr, serverAddr ;
	
	while( true )
	{
		socklen_t clientLen = sizeof(clientAddr) ;
		
		if( (slaveSock = accept(masterSock, (struct sockaddr*)&clientAddr, &clientLen)) < 0 )
			err_mesg("accept error") ;
		
		if( (pid = fork()) < 0 )
			err_mesg("fork error") ;
		
		else if( pid == 0 )
		{
			close( masterSock ) ;
			shell( slaveSock ) ;
			exit(0) ;
		}	
		
		else
		{		
			close( slaveSock ) ;
			wait(0) ;
		}	
		
	}	
}


void RAS::shell(int Socket)
{
	chdir( "./ras" ) ;
	setenv( "PATH", "bin:.", 1 ) ;
	
	char buffer[BUFSIZE] ;
	int revLen ; 
	string welcome = "****************************************\n** Welcome to the information server. **\n****************************************\n% " ;	
	write( Socket, welcome.c_str(), welcome.length() ) ;
	
	while( true )
	{	
		if( (revLen = read( Socket, buffer, BUFSIZE )) < 0 )
			err_mesg("read error") ;
		else if( revLen == 0 )
			return ;	
		else
		{
			string revComd(buffer, revLen) ;
			revComd = revComd.substr(0, revComd.find_first_of("\r")) ;
			
			if( revComd == "exit" )
				return ;
			
			else if( builtInCmd( revComd ) )
				builtInHandler( Socket, revComd ) ;
			
			else 
			{	
				if( !revComd.empty() )
				{		
					/* Unknown command */
					if( !binFinder( revComd ) )
					{
						string invalidCmd = "Unknown command: [" + revComd + "].\n" ;
						write( Socket, invalidCmd.c_str(), invalidCmd.length() ) ;	
					}	
					else	
					{	
						processor( Socket, revComd ) ;
						pool.pipeCountDowner() ;
					}
				}	
			}		
		}	
		
		write( Socket, "% ", 2 ) ;
	}
	
}






void RAS::cmdExecutor(int Socket, string Cmd)
{
	int pid, pos, inSock, OutSock, stderrSock = Socket ;
	Pipe outPipe(-1), stderrPipe(-1) ; 
	
	
	if( (inSock = pool.pipeReader()) < 0 ) 
		err_mesg("Invalid usage of pipe") ;
	
	/* stderr */
	if( (pos = Cmd.find_first_of("!")) != string::npos )
	{	
		stderrPipe = pool.pipeWriter( Cmd, pos ) ;
		stderrSock = stderrPipe.Socket[1] ;
	}	
	
	/* redirect and pipe */
	if( (pos = Cmd.find_first_of(">") ) != string::npos )
		OutSock = redirectHandler( Cmd, pos ) ;
	
	else if( (pos = Cmd.find_first_of("|") ) != string::npos )
	{	
		outPipe = pool.pipeWriter( Cmd, pos ) ;
		OutSock = outPipe.Socket[1] ;
	}	
		
	else
		OutSock = Socket ;
	
	spaceEraser( Cmd ) ;
	
	if( (pid = fork()) < 0 )
		err_mesg("fork error") ;
	
	else if( pid == 0 )
	{			
		dup2( inSock, STDIN ) ;
		dup2( OutSock, STDOUT ) ;
		dup2( stderrSock, STDERR ) ;
		
		char** cmd = formatChanger( Cmd ) ;	
		
		if( execvp( cmd[0], cmd ) < 0 )
		{
			string invalidCmd = "Unknown command: [" + Cmd + "].\n" ;
			write( Socket, invalidCmd.c_str(), invalidCmd.length() ) ;
			exit(0) ;
		}
		
		exit(EXIT_SUCCESS);
	}
		
	else
	{
		wait(0) ;
				
		/* Closing read pipe */
		if( inSock != STDIN )		
			close( inSock ) ;
	
		
		/* Not number pipe */
		if( outPipe.counter == 0 )
		{
			if( socketCloser(OutSock, STDOUT, Socket) )
				close( OutSock ) ;
		}
		else 	
			pool.destroyWritePipe() ;
			
		
		/* stderr */
		if( socketCloser(stderrSock, STDERR, Socket) )
			close( stderrSock ) ;

	}
}


bool RAS::socketCloser(int outSock, int typeFD, int clientSock)
{
	return outSock != typeFD && outSock != clientSock ;
}



void RAS::processor(int Socket, string cmd)
{
	vector<string> parsedCmd = parser( cmd ) ;
		
	for(string cmd : parsedCmd)
		cmdExecutor( Socket, cmd ) ;
}


/*
	The commands are parsed as follows:
	
	cat | cat | cat |5
	=> [cat |0][cat |0][cat |5]
	to indicate it's a number pipe with 0
		
	cat | ls
	=> [cat |0][ls]
*/
vector<string> RAS::parser(string cmd)
{
	if( cmd.find_first_of("|") == string::npos )
		return { cmd } ;

	stringstream SS(cmd) ;
	string token ;
	vector<string> parsedToken ;
	
	while( getline(SS, token, '|') )
	{	
		if( !parsedToken.empty() )
		{
			if( isdigit(token[0]) )
			{	
				parsedToken.back() += token ;
				break ;
			}	
			else
				parsedToken.back() += "0" ;
		}
		parsedToken.push_back( token + "|" ) ;
	}
	
	if( parsedToken.back().back() == '|' )
		parsedToken.back().pop_back() ;
	
	return parsedToken ;	
}


/* Handle for the ">" redirect case */
int RAS::redirectHandler(string& cmd, int pos)
{
	int startPos = cmd.find_first_not_of(" ", pos+1) ;
	string file = cmd.substr(startPos, string::npos) ;
	
	int outputSocket ; 
	if( (outputSocket = open( file.c_str(), O_WRONLY|O_CREAT, PERM )) < 0 )
		err_mesg("open error") ;
	
	cmd = cmd.substr(0, pos) ;
	while( cmd.back() == ' ' )
		cmd.pop_back() ;
	
	return outputSocket ;
}


// "cat test.html" -> ["cat"]["test.html"]
char** RAS::formatChanger(string Cmd)
{
	stringstream SS(Cmd) ;
	string token ;
	vector<string> parseCmd ;
	
	while( getline(SS, token, ' ') )
		parseCmd.push_back( token ) ;

	char** cmd = new char* [ parseCmd.size() ] ;	
	for(int i=0; i<parseCmd.size(); i++)
	{
		cmd[i] = new char [parseCmd[i].length()] ;
		strcpy( cmd[i], parseCmd[i].c_str() ) ;
	}	
	
	cmd[ parseCmd.size() ] = NULL ;
	
	return cmd ;
} 



bool RAS::binFinder(string cmd)
{
	DIR* dirPointer ;
	struct dirent* entry ;
	bool isFound = false ;
	
	if( (dirPointer = opendir("./bin")) == NULL )
		err_mesg("Open bin error") ;

	while( (entry = readdir(dirPointer)) != NULL )
	{
		if( cmd.find( entry->d_name ) != string::npos )
		{	
			isFound = true ;
			break ;
		}	
	}		

	closedir( dirPointer ) ;
	
	return isFound ;
}



int RAS::passiveSock(int port)
{
	struct protoent* ppe ;
	struct sockaddr_in serverAddr ;
	int masterSock ;
	
	if( (ppe = getprotobyname("tcp")) == 0 )
		err_mesg("getprotobyname error") ;
	
	if( (masterSock = socket(AF_INET, SOCK_STREAM, ppe->p_proto)) < 0 )
		err_mesg("socket error") ;
	
	bzero((char*)&serverAddr, sizeof(serverAddr)) ;
	serverAddr.sin_family 		= AF_INET ;
	serverAddr.sin_addr.s_addr 	= htonl(INADDR_ANY) ;
	serverAddr.sin_port 		= htons(port) ;
	if( bind(masterSock, (struct sockaddr*)& serverAddr, sizeof(serverAddr)) < 0 )
		err_mesg("bind error") ;
	
	listen(masterSock, QLEN) ;
	
	return masterSock ;	
}



bool RAS::builtInCmd(string cmd) 
{
	return cmd.find("printenv") != string::npos || cmd.find("setenv") != string::npos ;
}


void RAS::builtInHandler(int Socket, string cmd) 
{
	if( cmd.find("printenv") != string::npos )
	{
		int pos ;
		if( (pos = cmd.find_first_not_of(" ", 8)) != string::npos )
		{
			string envVar = cmd.substr(pos, string::npos) ;
			
			string envVarContent( getenv(envVar.c_str()) ) ;
			envVarContent += "\n" ;
			write( Socket, envVarContent.c_str(), envVarContent.length() ) ;
		}
		else
		{
			extern char** environ ;
			string allVar = "" ;
			for(int i=0; environ[i]!=(char*)0; i++)
				allVar += string(environ[i]) + "\n" ;
			
			write( Socket, allVar.c_str(), allVar.length() ) ;
		}	
	}
	
	else
	{
		stringstream SS(cmd) ;
		string token, envVar, paramter ;
		int counter = 1 ;
		
		while( getline(SS, token, ' ') )
		{
			if( counter == 2 )
				envVar = token ;
			else if( counter == 3 )
				paramter = token ;
			
			counter ++ ;
		}	
		
		setenv( envVar.c_str(), paramter.c_str(), 1 ) ;
	}		
}



void spaceEraser(string& str)
{
	int pos = str.find_first_not_of(" ") ;
	str = str.substr(pos, string::npos) ;
	
	while( str.back() == ' ' )
		str.pop_back() ;
}



void err_mesg(const char* mesg)
{
	fprintf(stderr, "%s\n", mesg) ;
	exit(0) ;
}
