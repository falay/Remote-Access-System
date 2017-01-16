Project Name: Remote Access System (ras)

It's a rsh-like access systems,
called remote access systems, including both client and server.  
In this system, the server designates a directory, 
say ras/; then, clients can only run executable programs inside the directory, ras/.  

/////////////////////////////////////////////////////////////////////////////////////////////////////
Input spec:

1.	The length of a single-line input will not exceed 15000 characters.
	There may be huge number of commands in a single-line input.
	Each command will not exceed 256 characters.

2.	There must be one or more spaces between commands and symbols (or arguments.),
	but no spaces between pipe and numbers.

	e.g. cat hello.txt | number
	     cat hello.txt |1 !1 number

3.	There will not be any '/' character in demo input.

4.	Pipe ("|") will not come with "printenv" and "setenv."

5.	Use '% ' as the command line prompt.

/////////////////////////////////////////////////////////////////////////////////////////////////////
About server:

1.	The welcome message MUST been shown as follows:
****************************************
** Welcome to the information server. **
****************************************

2.	Close the connection between the server and the client immediately when the server receive "exit".

3.	Note that the forked process of server MUST be killed when the connection to the client is closed.
	Otherwise, there may be lots zombie processes.

/////////////////////////////////////////////////////////////////////////////////////////////////////
About parsing:

1.	If there is command not found, print as follows:
	Unknown command: [command].

e.g. 	% ctt
		Unknown command: [ctt].

/////////////////////////////////////////////////////////////////////////////////////////////////////
About a numbered-pipe

1.	|N means the stdout of last process should be piped to the first legal process of next Nth command, where 1 <= N <= 1000.

2.	!N means the stderr of last process should be piped to the first legal process of next Nth command, where 1 <= N <= 1000.

e.g.
	1. % A | B |2
	2. % C | D |1
	3. % E | F
	This means the first command last process "B" stdout will pipe to the third command first process "E",
	       and the second command last process "D" stdout will pipe to the third command first porcess "E".

Note:
	process means one instruction like "ls", "number", "cat hello.txt"...
	command means the combination of server process in one line
	e.g.
		"% cat hello.txt | number | number !2" is one command with three porcess "cat hello.txt", "number", "number"

3.	|N and !N only appear at the end of line, and they can appear at the same time.

e.g.
	% cat hello.txt | number | number !2		(O)
	% cat hello.txt | number | number |3		(O)
	% cat hello.txt | number | number !2 |3		(O)
	% cat hello.txt | number | number |3 !2		(O)
	% cat hello.txt |1 number |1 number |1		(X,|N will only appear at the end of line)

4.	If there is any error in a input line, the line number should not count.

e.g.
	% ls |1
	% ctt               <= unknown command, process number is not counted
	Unknown command: [ctt].
	% number
	1	bin/
	2	test.html
	
4.	In our testing data, we will use pipe or numbered-pipe after unknown command like "ls | ctt | cat" or "ls | ctt |1". 
	In this case, process will stop when running unknown command, so the command or numbered-pipe after unknown command will not execute and numbered-pipe will not be counted.

e.g.
	% ls | ctt | cat
	Unknown command: [ctt].
	% number
	This will pipe "ls" stdout to next process "ctt", since "ctt" is error process, it will do nothing.
	And the next line command "number" will not receive any input.
	
5.	If mutiple commands output to the same pipe, the output should be ordered, not mixed.

e.g.
	% ls |3
	% removetag test.html |2
	% ls |1
	% cat   <= cat will not execute and numbered-pipe will not be counted.
	bin/
	test.html
	
	Test
	This is a test program
	for ras.

	bin/
	test.html
