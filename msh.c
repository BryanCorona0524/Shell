// The MIT License (MIT)
// 
// Copyright (c) 2016 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports four arguments

#define MAX_HIST 15             // History will show the 15 most recent commands

int main()
{


  char * command_string = (char*) malloc( MAX_COMMAND_SIZE );

  //history will be an array of 15 strings with a max string length of 255
  char history_list[MAX_HIST][MAX_COMMAND_SIZE];
  //filling the array with nothing since some elements may be assigned random values
  for(int i=0;i<MAX_HIST;i++)
      {
        strcpy(history_list[i],"");
        //printf("%d: %s\n",i,history_list[i]);
      }
  //history count will be used to record the number of commands in the history_list
  int hist_count=0;
  //this will hold the pids to the corresponding and will show -1 if its a builtin/not found command
  int pid_hist[MAX_HIST];
  int pid_count=0;

  while( 1 )
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (command_string, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      token[i] = NULL;
    }

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *argument_ptr = NULL;                                         
                                                           
    char *working_string  = strdup( command_string );

    // we are going to move the working_string pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *head_ptr = working_string;

    //if ! is used put the chosen history string into working string
    if(strchr(working_string,'!')!=NULL)
    {
      //printf("working string: *%s*\n",working_string);
      //index 1 should hold the number so we get the char and convert it into an int
      int index=atoi(&working_string[1]);
      //if the index is not between 0-14 or if there is nothing at the index, print an error
      if(index>=0 && index<=14)
      {
        if(strcmp(history_list[index],"")==0)
        {
        printf("Command not in history.\n");
        continue;
        }   
        else
        {
          //copying a previous command to working_string and letting it be added to history and get tokenized
          strcpy(working_string,history_list[index]);
        }
      }
      else
      {
        printf("Command not in history.\n");
        continue;
      }
    }

    //check if the input is not empty or if !n is used
    if(strcmp(working_string,"\n")!=0 && strchr(working_string,'!')==NULL)
    {
      if(hist_count<MAX_HIST)
      {
        strcpy(history_list[hist_count],working_string);
        hist_count++;
      }
      //if the array is already full, move the strings down 1 (if ls was in index 6, move down to 5)
      //after moving, put the most recent command on index 14
      else
      {
        int row,column;
        for(row=0;row<MAX_HIST;row++)
        {
          for(column=0;column<MAX_COMMAND_SIZE;column++)
          {
            history_list[row][column]=history_list[row+1][column];
          }
        }
        strcpy(history_list[MAX_HIST-1],working_string);
      }
    }

    // Tokenize the input strings with whitespace used as the delimiter
    while ( ( (argument_ptr = strsep(&working_string, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( argument_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this for loop and replace with your shell functionality
    /*
    int token_index  = 0;
    for( token_index = 0; token_index < token_count; token_index ++ ) 
    {
      printf("token[%d] = %s\n", token_index, token[token_index] );  
    }*/
    
    //auto set pid value to -1 and change if it does have a real pid when forking
    if(pid_count<MAX_HIST)
    {
        pid_hist[pid_count]=-1;
        pid_count++;
    }
    else
    {
      //pid_count=0; suggestion by professor but i dont think it will quite work out to match history
      //doing the same thing as history but now leaving the last element to be -1 or be changed when forking
      int row;
      for(row=0;row<MAX_HIST;row++)
      {
        pid_hist[row]=pid_hist[row+1];
      }
      pid_hist[MAX_HIST-1]=-1;
    }

    //takes care of blanks
    if(token[0]==NULL)
    {
      //do nothing
      //this will continue the shell if the user enters nothing
      pid_count--;
    }
    //compares string to exit/quit in order to close the program if told
    else if(strcmp(token[0],"exit")==0 || strcmp(token[0],"quit")==0)
    {
      return 0;
    }
    //if string is history, print the list by using hist_count as the max index
    else if(strcmp(token[0],"history")==0 && token[1]==NULL)
    {
      for(int i=0;i<hist_count;i++)
      {
        printf("%d: %s",i,history_list[i]);
      }
    }
    else if(strcmp(token[0],"history")==0 && strcmp(token[1],"-p")==0)
    {
      //testing pid history WILL BE MOVED LATER
      for(int i=0;i<hist_count;i++)
      {
      printf("%d: [%d] %s",i,pid_hist[i],history_list[i]);
      }
    }
    //takes care of cd [directory] and cd .. by using chdir. will also give an error if the directory is not found
    else if(strcmp(token[0],"cd")==0 && token[1]!=NULL)
    {
      //chdir returns 0 if sucessful and -1 if not
      if(chdir(token[1])==-1)
      {
        printf("Directory not found.\n");
      }
    }
    else
    {
      //in order to use exec, we can fork and let the child do exec and if correct, execute the command or give an invalid command message 
      pid_t pid = fork( );
      //child
      if( pid == 0 )
      {
        int ret = execvp( token[0], &token[0] );  
        if( ret == -1 )
        {
          printf("%s: Command not found.\n",token[0]);
          return 0;
        }
      }
      //parent
      else 
      {
        //reason for pid_count-1 is because pid_count was incremented before so we have to decrement and then save pid
        if(pid_count<MAX_HIST)
        {
          pid_hist[pid_count-1]=pid;
        }
        //else replace most recent -1 to given pid
        else
        {
          pid_hist[MAX_HIST-1]=pid;
        }
        //printf("\npid count %d and pid: %d\n",pid_count-1,pid_hist[pid_count-1]); //testing
        int status;
        wait( & status );
      }
    }

    // Cleanup allocated memory
    for( int i = 0; i < MAX_NUM_ARGUMENTS; i++ )
    {
      if( token[i] != NULL )
      {
        free( token[i] );
      }
    }

    free( head_ptr );

  }
  free( command_string );

  return 0;
  // e2520ca2-76f3-90d6-0242ac120003
}
