#include <unistd.h>
#include "Pipe.hpp"
#include <iostream>


int PipePool::pipeReader()
{
	if( vecPipe.empty() )
		return 0 ; /* No pipes -> stdin */
	
	int retFd = 0 ;	
	for(int i=0; i<vecPipe.size(); i++)
	{
		if( vecPipe[i].counter == 0 )
		{
			retFd = vecPipe[i].Socket[0] ;
			vecPipe.erase( vecPipe.begin()+i ) ;	
			break;
		}
	}	
	
	return retFd ; 
}





Pipe PipePool::pipeWriter(string& cmd, int pos) 
{	
	int num = 0 ;

	if( !isdigit( cmd[pos+1] ) )
		return -1 ;
		
	int i ;
	for(i=pos+1; i<cmd.length() && isdigit(cmd[i]); i++)
		num += 10*num + (cmd[i]-'0') ;
		
	cmd.replace( pos, i-pos, "" ) ;	
		
	for(Pipe& curPipe : vecPipe)
	{
		if( curPipe.counter == num )
			return curPipe ;
	}	
	
	Pipe newPipe( num ) ;
	if( pipe( newPipe.Socket ) < 0 )
		return -1 ;
	
	vecPipe.push_back( newPipe ) ;
	
	return newPipe ;	
}


void PipePool::pipeCountDowner()
{
	for(Pipe& curPipe : vecPipe)
		curPipe.counter -- ;
}

void PipePool::destroyReadPipe()
{
	for(int i=0; i<vecPipe.size(); i++)
		if( vecPipe[i].counter == 0 )
		{
			vecPipe.erase( vecPipe.begin()+i ) ;
			break ;
		}	
}


void PipePool::destroyWritePipe()
{
	for(Pipe& curPipe : vecPipe)
		if( curPipe.counter == 1 )
		{	
			close( curPipe.Socket[1] ) ;
			break  ;
		}	
}

