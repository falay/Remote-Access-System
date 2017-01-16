#include <stdio.h>
#include "RAS.hpp"


int main(int argc, char* argv[])
{
	if( argc == 2 )
		RAS server( atoi(argv[1]) ) ;

	else
		fprintf(stderr, "Usage: %s [port number]\n", argv[0]) ;
}