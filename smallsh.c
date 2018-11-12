/***************************************
 * Name: Andrew Snow
 * Program 3
 * CS 344
 **************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

//Functions
void command(); 
void handler(int);

//Global Variables
int exitStatus;
int forground = 0;
int quit;


int main(){
    
    quit = 0;

    while(quit == 0){
        command();
    }

    return 0;
}

void handler(int sig_num){
    signal(SIGTSTP, handler);
    if (forground > 0){
        forground = 0;
        printf("Exiting foreground-only mode");
    } else {
        forground = 1;
        printf("Entering foreground only mode (& is now ignored)");
    }
}

void command(){
    
    char *input;
    char *argument[512]; //512 arguments exepted
    char *inFile = NULL;
    char *outFile = NULL;
    size_t inputSize = 32;//change to proper size
    int background = 0;
    int count = 0;
    int i = 0;
    int fd;
    int childPros;

    //Set up Signals to ignore
    struct sigaction sig;
    sig.sa_handler = SIG_IGN;
    sigaction(SIGINT, &sig, NULL);

    signal(SIGTSTP, handler);

    /*********************
     * Read in command
     * *****************/

    //Allocate Space for input
    input = (char *)malloc(inputSize * sizeof(char));

    printf(": ");
    fflush(stdout);

    //Read Input from user and figure out what to do with it
    if (!getline(&input, &inputSize, stdin)){     
        
        //error
        printf("Error Reading stdin");
        fflush(stdout);
    }

    //Remove tailing new line
    if (input[strlen(input) - 1] == '\n'){
        input[strlen(input) - 1] = 0;
    }



    /******************************************
     * Break up the arguments using strtok
     * Check for ">", "<", "&", and "$$" chars
     ******************************************/
    argument[0] = strtok(input, " ");
   
    while (argument[count] != NULL){
       
        count++;
        argument[count] = strtok(NULL, " ");
       
        //Check for NULL to prevent segfault
        if (argument[count] != NULL){
            
            //Check for ">", and set outFile accordingly
            //outFile is later used for fd
            if (!strcmp(argument[count], ">")){
            
                //Check that file follows after
                argument[count] = strtok(NULL, " ");
                
                if (argument[count] != NULL){
                    outFile = argument[count];
                     
                } else {

                    //error
                    printf("No file following after >\n");
                    exitStatus = 1;
                                
                }
                
                count--;

            //Check for "<" and set inFile accordingly
            //inFile is later used for fd
            } else if (!strcmp(argument[count], "<")){
                    
                //Check that file follows after
                argument[count] = strtok(NULL, " ");
                
                if (argument[count] != NULL){
                    inFile = argument[count];

                } else {

                    //error
                    printf("No file following after <\n");
                    exitStatus = 1;   
                
                }

                count--;

            //Check for "$$"
            //prints out current shell pid
            } else if (!strcmp(argument[count], "$$")){
                printf("%d\n", getpid());
                argument[0] = NULL;


            //Check for "&" at the end of arguments
            //Set background to true
            } else if (!strcmp(argument[count], "&")){
               
                //Varify that its the last argument
                argument[count] = strtok(NULL, " ");

                if (argument[count] != NULL){
                    //Do nothing
                } else {
                    if (forground == 0){
                        background = 1;
                    }
                }
            }
        }
    } 



    /********************************************
     *Check first argument for spicific commands
     *Exit, Status, CD, #
     * *****************************************/

    //Check first argument
    //Note: if it is the only argument then a '\n' will tail it
    if (argument[0] == NULL){
        //Do nothing
    
    } else if (!strcmp(argument[0], "exit")){
        
        //kill all other processes
        exit(0);
        quit = 1;
        
    } else if (!strcmp(argument[0], "cd")){ 
        
        //Check and change directory to next argument
        //Otherwise change directory to HOME enviroment variable
        if (argument[1] != NULL){
            if (chdir(argument[1]) != -1){
            
                chdir(argument[1]);
            
            } else {

                printf("Directory does not exist\n"); 
                fflush(stdout);
                exitStatus = 1;
            }

        } else {
            chdir(getenv("HOME"));
        }

        
    } else if (!strcmp(argument[0], "status")){

        //Print previous terminating signal or exit status
        if (exitStatus == 0){         
            printf("exit value: %d\n", exitStatus);
            fflush(stdout);

        } else if (WIFEXITED(exitStatus)){

            printf("exit value: %d\n", WIFEXITED(exitStatus));
            fflush(stdout);

        } else {
            printf("terminated by signal %d\n", WTERMSIG(exitStatus));
            fflush(stdout);
        }

    } else if (*argument[0] == '#'){ 
        //This does nothing
    
    } else {



    /******************************
     *Execute Command
     * ***************************/

        childPros = fork();

        switch (childPros){

            case -1:
            
                perror("Hull breach\n");
                exitStatus = 1;

                break;

            case 0:

            /********************
             * Child Case
             * *****************/
           
            //Set up signals
            if (background == 0){
                sig.sa_handler = SIG_DFL;
                sig.sa_flags = 0;
                sigaction(SIGINT, &sig, NULL);
            }


            /*********************************
             * Redirection of input and output
            **********************************/
            
                //Redirect using "<" char
                if (inFile != NULL){
                                
                    //Open file
                    fd = open(inFile, O_RDONLY);

                    //Check file open
                    if (fd < 0){
                    
                        printf("Invalid file %s\n", inFile);
                        fflush(stdout);
                        exitStatus = 1;
                        exit(1);

                    }

                    //Redirect to File
                    if (dup2(fd, 0) < 0){
                    
                        close(fd);
                        printf("dup2 error\n");
                        exitStatus = 1;
                        exit(1);
                    }

                    close(fd);
                }

                //Redirect using ">" char
                if (outFile != NULL){
                
                    //Open file
                    fd = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

                    //Check file open
                    if (fd < 0){
                    
                        printf("Invalid File %s\n", outFile);
                        fflush(stdout);
                        exitStatus = 1;
                        exit(1);

                    }

                    if (dup2(fd, fileno(stdout)) < 0){
                        
                        close(fd);
                        printf("dup2 error\n");
                        exitStatus = 1;
                        exit(1);
                    }
                
                    close(fd); 
                }    
            

                if (background == 1) {

                    fd = open("/dev/null", O_RDONLY);

                    if (fd < 0){

                        printf("Failed to open\n");
                        fflush(stdout);
                        exitStatus = 1;
                        exit(1);

                    }
                    
                    if (dup2(fd, 0) < 0){

                        close(fd);
                        printf("dup2 error\n");
                        exitStatus = 1;
                        exit(1);
                    }
        
                    close(fd);
                }


                //Execute the command and following arguments            
                if (execvp(argument[0], argument)){
                
                    printf("Exec Failure\n");
                    fflush(stdout);
                    exitStatus = 1;
                    exit(1);
                }    
            
                break;
           

            default:

            /***********
             * Parent
             * ********/
            
                //Check for background process
                if (background == 1){
                    //Print background pid
                    printf("background pid is %d\n", childPros); 
                    fflush(stdout);

                } else {

                    //need parent to wait for child to finsih
                    waitpid(childPros, &exitStatus, 0);
                
                }
            
            }
             
             
             //Check background processes (Reference from martamae's github)
            childPros = waitpid(-1, &exitStatus, WNOHANG);
            
            while (childPros > 0) {

                printf("background pid complete: %d\n", childPros);

                if (WIFEXITED(exitStatus)) {

                    printf("exit status %d\n", WEXITSTATUS(exitStatus));
                    fflush(stdout);
                
                } else { 

                    printf("terminating sygnal: %d\n", exitStatus);
                    fflush(stdout);
                }

                childPros = waitpid(-1, &exitStatus, WNOHANG);
            }
    }

    //Before ending free allocated space
    free(input);
      
    
}





