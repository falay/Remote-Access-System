#ifndef RAS_HPP
#define RAS_HPP


#include "Pipe.hpp"
using std::string ;
using std::vector ;

enum
{
	STDIN  ,
	STDOUT ,
	STDERR 
} ;


class RAS
{
	public:
		RAS(int) ;
		int passiveSock(int) ;
		void shell(int) ;
		void processor(int,string) ;
		char** formatChanger(string) ;
		void cmdExecutor(int,string) ;
		bool builtInCmd(string) ;
		void builtInHandler(int,string) ;
		int redirectHandler(string&,int) ;
		vector<string> parser(string) ;
		bool socketCloser(int, int, int) ;
		bool binFinder(string) ;
		
	private:
		PipePool pool ;		
} ;


void spaceEraser(string&) ;
void err_mesg(const char*) ;



#endif