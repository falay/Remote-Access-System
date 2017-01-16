#ifndef PIPE_HPP
#define PIPE_HPP

#include <vector>
#include <string>

using std::vector ;
using std::string ;





struct Pipe
{
	int Socket[2] ;
	int counter ;

	Pipe(int c) : counter(c) {}
} ;


class PipePool
{
	public:
		int pipeReader() ;
		Pipe pipeWriter(string&,int) ;
		void pipeCountDowner() ;
		void destroyWritePipe() ;	
		void destroyReadPipe() ;
		
	private:
		vector<Pipe> vecPipe ;
} ;




#endif