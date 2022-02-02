/**********************************************************************
cs1713p4Driver.c
Purpose:
    This program reads course information and a command file.   It 
    processes the commands against the course information.
    This is the driver code for the routines written by the students.
Command Parameters:
    ./register -s courseFileName -c commandFileName
Input:
    Course   Stream input file which contains many records defining courses:
                 szCourseId szRoom  szDays  szTimes     iAvailSeats dFee 
                 10s        3s      3s      5s          4d          10lf 

    Command  This is different from the previous assignment.  The file contains 
             text in the form of commands (one command per text line):  
                 STUDENT BEGIN szStudentId cGender szBirthDate szFullName 
                     specifies the beginning of student request and includes 
                     all the identification information from program 3.
                 STUDENT INFO szMajor,szEmail,dGpa,cInternationalStudent ( Y or N )
                     specifies the address for a student (separated by commas)
                 STUDENT REQUEST szCourseId
                    specifies a single course request.  Steps:
                     >	Print the course ID and requested number of seats
                     >	Lookup the course ID, If not found, print a warning (but do not terminate your program) and return.
                     >	If found, try to satisfy the request.  
                        If not enough seats,  print a warning and return.
                     >	Print the fees.
                     >	Accumulate the total fees for this student
                 STUDENT COMPLETE
                     specifies the completion of the list of course requests 
                     for a student.
                 COURSE INCREASE szCourseId
                     increase the available seats for a course by the specified quantity.
                 COURSE SHOW szCourseId    
                     requests a display of a particular course.  Show all of its information.

Results:
    Prints the Courses prior to sorting
    Prints the Course after sorting.
    Processes the commands (see above) and shows any errors.
    Prints the resulting Courses
Returns:
    0  normal
    -1 invalid command line syntax
    -2 show usage only
    -3 error during processing, see stderr for more information

Notes:
    ./register -?       will provide the usage information.  In some shells,
                you will have to type ./register -\?

**********************************************************************/
// If compiling using visual studio, tell the compiler not to give its warnings
// about the safety of scanf and printf
#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cs1713p5.h"

// prototypes for this file
void processCommandSwitches(int argc, char *argv[], char **ppszCourseFileName
    , char **ppszCommandFileName);
void processCommands(CourseNode *pRoot, char *pszCommandFileName);
int getSimpleToken(char szInput[], int *piBufferPosition, char szToken[]);

#define MAX_TOKEN_SIZE 20

int main(int argc, char *argv[])
{
    CourseNode *pRoot = NULL;
    char        *pszCourseFileName = NULL;       // Course file name
    char        *pszCommandFileName = NULL;      // Command file name

    processCommandSwitches(argc, argv, &pszCourseFileName, &pszCommandFileName);

    // get the course, print it, sort it, and print the sorted
    pRoot = getCourses(pszCourseFileName);
    printCourses("Initial Courses", pRoot);
    
    // process the command input file
    processCommands(pRoot, pszCommandFileName);

    // print the course after processing the commandfile
    printCourses("Resulting Courses", pRoot);
    printf("\n");
    return 0;
}

/******************** processCommands **************************************
    void processCommands(Course courseM[], int iCourseCnt
        , char *pszCommandFileName)
Purpose:
    Reads the Comand file to process commands.  There are two major
    types of commands:  STUDENT and COURSE.
Parameters:
    I/O Course courseM[]              array of course
    I   int   iCourseCnt              number of elments in courseM[]
    I   char  **ppszCommandFileName   command file name
Notes:
    This opens and closes the Command file.
**************************************************************************/
void processCommands(CourseNode *pRoot, char *pszCommandFileName)
{
    FILE *pfileCommand;                     // stream Input for Student Student data
    // variables for command processing
    char szInputBuffer[100];                // input buffer for a single text line
    char szCommand[MAX_TOKEN_SIZE+1];       // command
    char szSubCommand[MAX_TOKEN_SIZE+1];    // subcommand
    int bGotToken;                          // TRUE if getSimpleToken got a token
    int iBufferPosition;                    // This is used by getSimpleToken to 
                                            // track parsing position within input buffer
 
    // variables for student information (except Course Request)
    Student student;
    double dStudentRequestTotalCost = 0.0;

    // open the Command stream data file
    if (pszCommandFileName == NULL)
        exitError(ERR_MISSING_SWITCH, "-c");

    pfileCommand = fopen(pszCommandFileName, "r");
    if (pfileCommand == NULL)
        exitError(ERR_COMMAND_FILENAME, pszCommandFileName);
    
    /* get command data until EOF
    ** fgets returns null when EOF is reached.
    */
    while (fgets(szInputBuffer, 100, pfileCommand) != NULL)
    {
        printf("%s", szInputBuffer);
        iBufferPosition = 0;                // reset buffer position

        // get the command
        bGotToken = getSimpleToken(szInputBuffer, &iBufferPosition, szCommand);

        // see what the command is
        if (bGotToken && strcmp(szCommand, "STUDENT") == 0)
        {   // STUDENT command
            // get the sub comand
            bGotToken = getSimpleToken(szInputBuffer, &iBufferPosition, szSubCommand);
            if (bGotToken)
                processStudentCommand(pRoot, szSubCommand
                    , &szInputBuffer[iBufferPosition]           // address past the subcommand
                    , &student
                    , &dStudentRequestTotalCost);
            else exitError(ERR_STUDENT_SUB_COMMAND, " missing subcommand");
        }
        else  if (bGotToken && strcmp(szCommand, "COURSE") == 0)
        {   // COURSE command
            bGotToken = getSimpleToken(szInputBuffer, &iBufferPosition, szSubCommand);
            if (bGotToken)
                processCourseCommand(pRoot, szSubCommand
                    , &szInputBuffer[iBufferPosition]);
            else exitError(ERR_COURSE_SUB_COMMAND, " missing subcommand");
        }
        else 
            exitError(ERR_INVALID_COMMAND, szCommand);
    }
    fclose(pfileCommand);
}

 /******************** getSimpleToken **************************************
 int getSimpleToken(char szInput[], int *piBufferPosition, char szToken[])
 Purpose:
    Returns the next token in a string.  The delimiter for the token is a 
    space, a newline or the end of the string.  To help with a 
    subsequent call, it also returns the next position in the buffer.
Parameters:
    I   char szInput[]          input buffer to be parsed
    I/O int *piBufferPosition   Position to begin looking for the next token.
                                This is also used to return the next 
                                position to look for a token (which will
                                follow the delimiter).
    O   char szToken[]          Returned token.  
Returns:
    Functionally:
        TRUE - a token is returned
        FALSE - no more tokens
    iBufferPosition parm - the next position for parsing
    szToken parm - the returned token.  If not found, it will be an empty string.
Notes:
    - If the token is larger than the szToken parm, we return a truncated value.
**************************************************************************/

int getSimpleToken(char szInput[], int *piBufferPosition, char szToken[]) 
{
    int iDelimPos;                      // found position of delim
    int iCopy;                          // number of characters to copy
    char szDelims[20] = " \n";          // delimiters
    
    // check for past the end of the string
    if (*piBufferPosition >= (int) strlen(szInput))
    {
        szToken[0] = '\0';              // mark it empty
        return FALSE;                   // no more tokens
    }

    // get the position of the first delim, relative to the iBufferPosition
    iDelimPos = strcspn(&szInput[*piBufferPosition], szDelims);

    // see if we have more characters than target token, if so, trunc
    if (iDelimPos > MAX_TOKEN_SIZE)
        iCopy = MAX_TOKEN_SIZE;             // truncated size
    else
        iCopy = iDelimPos;
    
    // copy the token into the target token variable
    memcpy(szToken, &szInput[*piBufferPosition], iCopy);
    szToken[iCopy] = '\0';              // null terminate

    // advance the position
    *piBufferPosition += iDelimPos + 1;  
    return TRUE;
}


/******************** printFormattedTreeHelper **************************************
    void printFormattedTreeHelper(CourseNode *p, char szPrefix[], int bIsTail)
Purpose:
    prints a binary tree formatted to show structure, this is the helper function
    called by the printFormattedTree function.
Parameters:
    I   CourseNode *p           Current node
    I   char szPrefix[]         String to be printed before current nodes value
                                should be at least 200 characters
    I   int bIsTail             indicates if the current node is the last in its
                                branch
Notes:
    Should be called with an empty string of at least 200 characters and
    and a value of true for bIsTail.

    This is a recursive function.
**************************************************************************/
void printFormattedTreeHelper(CourseNode *p, char szPrefix[], int bIsTail)
{
    if (p == NULL)
        return;

    // if we're at the end of  the current branch
    if (bIsTail == TRUE)
    {
        printf("%s └── %s\n", szPrefix, p->course.szCourseId);
        sprintf(szPrefix, "%s%s", szPrefix, "    ");
    }
    else
    {
        printf("%s ├── %s\n", szPrefix, p->course.szCourseId);
        sprintf(szPrefix, "%s%s", szPrefix, " │   ");
    }
    
    // save off current position
    int position = strnlen(szPrefix, 200);
    printFormattedTreeHelper(p->pRight, szPrefix, p->pLeft == NULL);

    // allows cutting off info from upper branches
    szPrefix[position] = '\0'; // reset output to current position

    printFormattedTreeHelper(p->pLeft, szPrefix, TRUE);
}

/******************** printFormattedTree **************************************
    void printFormattedTree(CourseNode *pRoot)
Purpose:
    prints a binary tree formatted to show structure.

Parameters:
    I   CourseNode *pRoot           Root of the tree to be printed

Notes:
    This function uses the printFormattedTreeHelper function.
**************************************************************************/
void printFormattedTree(CourseNode *pRoot)
{
    char szPrefix[200];
    memset(szPrefix, 0, 200);
    printFormattedTreeHelper(pRoot, szPrefix, 1);
}

/******************** processCommandSwitches *****************************
    void processCommandSwitches(int argc, char *argv[]
        , char **ppszCourseFileName
        , char **ppszCommandFileName)
Purpose:
    Checks the syntax of command line arguments and returns the filenames.
    If any switches are unknown, it exits with an error.
Parameters:
    I   int argc                        count of command line arguments
    I   char *argv[]                    array of command line arguments
    O   char **ppszCourseFileName    course file name
    O   char **ppszCommandFileName      command file name
Notes:
    If a -? switch is passed, the usage is printed and the program exits
    with USAGE_ONLY.
    If a syntax error is encountered (e.g., unknown switch), the program
    prints a message to stderr and exits with ERR_COMMAND_LINE_SYNTAX.
**************************************************************************/

void processCommandSwitches(int argc, char *argv[], char **ppszCourseFileName
    , char **ppszCommandFileName)
{
    int i;
    int rc = 0;
    int bShowCommandHelp = FALSE;

    for (i = 1; i < argc; i++)
    {
        // check for a switch
        if (argv[i][0] != '-')
            exitUsage(i, ERR_EXPECTED_SWITCH, argv[i]);
        // determine which switch it is
        switch (argv[i][1])
        {
        case 's':                   // Course File Name
            if (++i >= argc)
                exitUsage(i, ERR_MISSING_ARGUMENT, argv[i - 1]);
            // check for too long of a file anme
            else
                *ppszCourseFileName = argv[i];
            break;
        case 'c':                   // Student Student File Name
            if (++i >= argc)
                exitUsage(i, ERR_MISSING_ARGUMENT, argv[i - 1]);
            else
                *ppszCommandFileName = argv[i];
            break;
        case '?':
            exitUsage(USAGE_ONLY, "", "");
            break;
        default:
            exitUsage(i, ERR_EXPECTED_SWITCH, argv[i]);
        }
    }
}

/******************** exitError *****************************
    void exitError(char *pszMessage, char *pszDiagnosticInfo)
Purpose:
    Prints an error message and diagnostic to stderr.  Exits with
    ERROR_PROCESSING.
Parameters:
    I char *pszMessage              error message to print
    I char *pszDiagnosticInfo       supplemental diagnostic information
Notes:
    This routine causes the program to exit.
**************************************************************************/
void exitError(char *pszMessage, char *pszDiagnosticInfo)
{
    fprintf(stderr, "Error: %s %s\n"
        , pszMessage
        , pszDiagnosticInfo);
    exit(ERROR_PROCESSING);
}

/******************** exitUsage *****************************
    void exitUsage(int iArg, char *pszMessage, char *pszDiagnosticInfo)
Purpose:
    If this is an argument error (iArg >= 0), it prints a formatted message
    showing which argument was in error, the specified message, and
    supplemental diagnostic information.  It also shows the usage. It exits
    with ERR_COMMAND_LINE_SYNTAX.

    If this is just asking for usage (iArg will be -1), the usage is shown.
    It exits with USAGE_ONLY.
Parameters:
    I int iArg                      command argument subscript
    I char *pszMessage              error message to print
    I char *pszDiagnosticInfo       supplemental diagnostic information
Notes:
    This routine causes the program to exit.
**************************************************************************/

void exitUsage(int iArg, char *pszMessage, char *pszDiagnosticInfo)
{
    if (iArg >= 0)
        fprintf(stderr, "Error: bad argument #%d.  %s %s\n"
        , iArg
        , pszMessage
        , pszDiagnosticInfo);
    fprintf(stderr, "p3 -f courseFileName -c commandFileName\n");
    if (iArg >= 0)
        exit(-1);
    else
        exit(-2);
}