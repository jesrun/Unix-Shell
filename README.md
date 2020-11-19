# Unix-Shell
This program uses C as a shell interface that accepts user commands from the command line and executes the commands. The program supports input and output redirection and pipes, and it involves using system calls that can be executed on any Linux, Unix, or macOS system.

## Usage
### Compile the program
```
$ gcc shell.c
```

### Run the program
```
$ ./a.out
```

### Exit the program
```
osh>exit
```

### Execute the most recent command
```
osh>!!
```

## Execution Examples
Display the file program.c on the terminal using the Unix cat command: `osh>cat program.c`  
Redirect output to a file: `osh>ls > output.txt`  
Redirect input from a file: `osh>sort < input.txt`  
Communicate using a pipe: `osh>ls -l | less`   
Push a process to run in the background: `osh>cat progam.c &`
