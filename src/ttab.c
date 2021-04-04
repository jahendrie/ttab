/*******************************************************************************
 * ttab.c   |   v 0.94  |   GPL v 3     |   2021-04-03
 * James Hendrie        |   hendrie dot james at gmail dot com
 *
 *      ttab is a simple adding program; you use it to add or subtract one
 *      number after another.  It shows a running log in the terminal window,
 *      and also has a built-in history and limited 'undo' functionality.  It
 *      can also sum up numbers piped to it.  See the README or man file for
 *      more information.
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TTAB_VERSION "0.94"

#define DATE_STRING_LEN 25
#define NUM_STRING_LEN 64
#define MAX_STRING_LEN 80

double total;
double entered;
char line[ (MAX_STRING_LEN) ];
char *saveLocation;
char mode;

int counter;


/*******************************************************************************
 *                          FUNCTION PROTOTYPES
*******************************************************************************/

void print_prompt(void);
void print_usage(void);
void print_commands(void);
void print_options(void);
void print_help(void);
void clean_up(void);
void print_version_info(void);
void truncate_zeroes( double total );
void sum_log(FILE *fp);
void sum_log_stdin(void);
void print_log(FILE *fp);
void save_file(char *saveLocation);
void undo_prev(void);
void mem_error(const char *description);
void add_to_undo(double *current, char cc);
void do_math(double *current);
void clear_register(double *current);
struct action* create_node(void);
double* get_entered(double *current);
char* get_date_string(char quickSaving);

/****************    -------  END PROTOTYPES  ------    ***********************/


struct action {
    double number;              //  Number added / subtracted / whatever
    double runningTotal;        //  Running total after operation

    char date[DATE_STRING_LEN]; //  Date at which operation was performed

    /*
     * Single-character comment code.
     *
     * 'R' = register reset
     * 'L' = clear log
     * 'A' = clear all (register and total)
     *
     * 'w' = write file
     * 'l' = load file
     *
     * 'a' = number added
     * 's' = number subtracted
     * 'u' = undo operation
     *
     * Any other character code(s) will result in no comment being added (0)
     *
     */
    char commentCode;

    struct action *next;    //  Our next node pointer
    struct action *prev;    //  Our previous node pointer
};


/*  Create empty list pointers */
struct action *undo;
struct action *history;


void print_usage(void)
{
    printf("Usage:  ttab [OPTION]\n");
}

void print_commands(void)
{
    printf("\nCOMMANDS\n");
    printf("\th\t\tPrint this help text\n");
    printf("\ts FILENAME\tSave log to FILENAME\n");
    printf("\ts or /\t\tQuicksave to previously specified filename (or to\n");
    printf("\t\t\tttab_yyyy-mm-dd_hh-mm-ss.log if no filename was\n");
    printf("\t\t\tprevious specified)\n");

    printf("\tl or *\t\tShow running log\n");
    printf("\tc\t\tClear register\n");
    printf("\tu\t\tUndo previous operation\n");
    printf("\t-\t\tPerform arithmetic opposite to previous operation once\n");
    printf("\t+ or ENTER\tRepeat previous operation once\n");
    printf("\tN..\t\tRepeat previous operation N times\n");
}


void print_options(void)
{
    printf("OPTIONS\n");
    printf("\t-h or --help:\tPrint this help text\n");
    printf("\t--version:\tPrint version and author info\n");
    printf("\t-\t\tRead from stdin\n");
}

void print_help(void)
{
    print_usage();
    print_options();
    print_commands();

    printf("\n\tThis program can also sum files with 1 number per line or\n");
    printf("\tttab logs (including negative and floating point numbers).\n");
    printf("\tThis can be given as an argument or piped in using the '-'\n");
    printf("\toption.\n");

}

void print_version_info(void)
{
    printf("ttab version %s\n", (TTAB_VERSION) );
    printf("James Hendrie ( hendrie dot james at gmail dot com )\n");
}


void mem_error(const char *description)
{
    fprintf(stderr, "ERROR:  Out of memory.\n");
    fprintf(stderr, "%s\n", description);
    exit(8);
}


struct action* create_node(void)
{
    struct action *temp = NULL;

    temp = malloc( sizeof(struct action) );
    if( temp == NULL )
        mem_error("function:  create_node");

    /*  Initialize everything to 0 or NULL */
    temp->number = 0;
    temp->runningTotal = 0;

    /* OMITTED (because I don't need to set them here:
     * char date[25]
     * char commentCode
     */

    temp->next = NULL;
    temp->prev = NULL;

    return(temp);
}


void print_prompt(void)
{
    /*
     * For aesthetic reasons, we want to put the cursor a certain distance away
     * from the display register.  Basically, if there are 5 chars or less, we
     * tab to the right twice.  Any more, we tab only once.
     */
    if( total < 10000 && total > -1000 )
        printf("[%g]:\t\t", total);
    else
        printf("[%g]:\t", total);
}


/*
 * This function just gets rid of the zeroes at the end of the number, since
 * we really don't need all those nasty extras.
 */
void truncate_zeroes( double total )
{
    //  Make our string and print the total to it
    char numString[ (NUM_STRING_LEN) ];
    snprintf( numString, (NUM_STRING_LEN), "%lf", total );

    //  Find out how long it actually is
    int len = strnlen( numString, (NUM_STRING_LEN));

    //  Index into it and go backward until we hit a non-zero number or period
    int idx = len - 1;
    int ch = numString[ idx ];
    while( idx >= 0 && ch == '0' && ( ch != '.' && ch != '\0' ) )
    {
        ch = numString[ idx ];
        --idx;
    }

    /*
     * Here we trial-and-error our way to success while pretending we perfectly
     * understand how indexing affects printing
     */
    if( ch == '.' )
        numString[ idx + 1] = '\0';
    else
        numString[ idx + 2 ] = '\0';

    //  Print it out
    printf( "%s\n", numString );

}

/*
 * This function exists so that people can just pipe numbers to the program
 * and have it add them up
 */
void sum_log_stdin(void)
{
    /*  Make a copy of the line but exclude comments */
    char good[ (MAX_STRING_LEN) ];
    for( int i = 0, ch = line[ i ]; i < strlen( line ); ++i )
    {
        if( ch == '#' )
            break;
        good[ i ] = line[ i ];
    }


    char *theNumber = NULL;     //  Used for strtok

    while( fgets(good, 80, stdin) != NULL )
    {
        /*  Check for ttab logs */
        if( strcmp(good, "----------------------------------------\n") == 0 )
        {
            /*
             * If it's a ttab log, send it to the standard sum_log function and
             * treat it like a normal file.  To be completely honest, I'm a bit
             * surprised that this actually works
             */
            sum_log(stdin);

            exit(0);    //  Exit the program
        }

        /*
         * We'll use strtok to tokenize the string, number by number, using the
         * space character as a delimiter
         */
        theNumber = strtok(good, " ");
        while( theNumber != NULL )
        {
            total += atof(theNumber);       //  ADD 'ER UP BABY
            theNumber = strtok(NULL, " ");  //  Re-run strtok, grab next number
        }
    }

    /*  Print our total */
    truncate_zeroes( total );
    //printf("%lf\n", total);
}


void sum_log(FILE *fp)
{
    char ttabLogMode = 0;           //  Special mode for ttab logs
    char *numberLocation = NULL;    //  Pointer to string containing number
    char ch = 0;                    //  Used for ttab mode
    counter = 0;                    //  ''
    total = 0;                      //  Our total

    /*  Get lines from file pointer */
    while( fgets(line, 80, fp) != NULL )
    {
        counter = 0;    //  Reset counter

        /*  Make a copy of the line but exclude comments */
        char good[ (MAX_STRING_LEN) ];
        for( int i = 0, ch = line[ i ]; i < strlen( line ); ++i )
        {
            if( ch == '#' )
                break;
            good[ i ] = line[ i ];
        }

        /*  If we run into a ttab log, go into ttab log mode */
        if( strcmp( good, "TTAB LOG\n") == 0 )
        {
            ttabLogMode = 1;
        }

        /*  If we're in ttab log mode */
        if( ttabLogMode == 1 )
        {
            numberLocation = NULL;  //  Null out number location pointer

            ch = good[counter];     //  Initialize ch

            /*  While we're getting characters that are potentially useful */
            while( ch != '\n' && ch != '\0' && ch != EOF )
            {
                ch = good[counter];     //  Reset ch to its position in the line
                
                if( ch == '\t' )        //  If we find a tab character
                {
                    /* 
                     * If the next character after the tab is a plus or a minus,
                     * we've finally found our number
                     */
                    if( good[counter + 1] == '+' || good[counter + 1] == '-' )
                    {
                        /*
                         * Here we assign the address of the position in the
                         * string where we've found the number to numberLocation
                         */
                        numberLocation = &good[counter+1];
                        break;
                    }
                }

                ++counter;      //  Increment the counter to keep things a-movin
            }

            /*  If we found a usable number (finally), add it to total */
            if( numberLocation != NULL )
            {
                total += atof(numberLocation);
            }

        }   /*  -------     END ttab log mode   ----- */

        else    //  If we're doing a normal file / stdin
        {
            /*  Add whatever's on the line to the total */
            total += atof(good);
        }
    }

    /*  Print total to stdout */
    truncate_zeroes( total );
    //printf("%lf\n", total);
}


void print_log(FILE *fp)
{
    /*  First, check which file stream we're using */
    FILE *fs = fp;
    if( fs == NULL )
        fs = stdout;

    struct action *temp = undo;

    fprintf(fs, "\n");

    while( temp != NULL )
    {
        if( temp->commentCode != 0 )
        {
            fprintf(fs, "%s", temp->date);
            switch( temp->commentCode )
            {
                case 'a':
                    fprintf(fs, "\t+%g\n", temp->number);
                    break;
                case 's':
                    fprintf(fs, "\t%g\n", temp->number);
                    break;
                case 'u':
                    fprintf(fs, "\tUNDO\n");
                    fprintf(fs, "%s\t%g\n", temp->date, temp->number);
                    break;
                case 'R':
                    fprintf(fs, "\tREGISTER CLEARED\n");
                    fprintf(fs, "%s\t%g\n", temp->date, temp->number);
                    break;
                default:
                    fprintf(fs, "I DON'T KNOW WHAT I'M DOING\n");
                    break;
            }
            fprintf(fs, "%s\tTotal:  %g\n\n", temp->date, temp->runningTotal);
        }

        temp = temp->next;
    }
}


void save_file(char *saveLocation)
{
    FILE *fp = NULL;
    fp = fopen(saveLocation, "w+");
    if( fp == NULL )
    {
        if( saveLocation[0] == '~' )
        {
            char *userHome = getenv("HOME");
            char newLocation[strlen(saveLocation) + strlen(userHome) + 1];

            sprintf(newLocation, "%s/%s", userHome, saveLocation+2);

            fp = fopen(newLocation, "w+");
            if( fp == NULL )
            {
                fprintf(stderr, "\nERROR:  Cannot open file for writing:  %s\n\n",
                    newLocation);
            }
        }
        else
        {
            fprintf(stderr, "\nERROR:  Cannot open file for writing:  %s\n\n",
                    saveLocation);
        }
    }

    if( fp != NULL )
    {
        //  Timestamp
        fprintf(fp, "----------------------------------------\nTTAB LOG\n");
        fprintf(fp,"Created %s\n", get_date_string(0) );
        fprintf(fp, "----------------------------------------\n\n");

        //  Send the file pointer to print_log
        print_log(fp);

        //  Tell the user what's up
        printf("\nLog written to %s\n\n", saveLocation);
    }
}


void clear_register(double *current)
{
    *current = total * -1;
    add_to_undo( current, 'R' );
    total -= total;
}


double* get_entered(double *current)
{
    double temp = 0;
    char lineFront = 0;
    char lineBack = 0;

    fgets(line, sizeof(line), stdin);

    /*  Check for quit */
    if( line[0] == 'q' || line[0] == 'Q' )
    {
        clean_up();
        exit(0);
    }

    /*  Check if they want to clear the register */
    if( line[0] == 'c' || line[0] == 'C' )
    {
        clear_register(current);
        *current = 0;
        return(current);
    }

    /*  Show running log (history) */
    if( line[0] == 'l' || line[0] == '*' )
    {
        print_log(NULL);
        *current = 0;
        return(current);
    }


    /*  Check if the user wants to see the help */
    if( line[0] == 'h' )
    {
        printf("\nGENERAL USAGE\n");
        printf("\tEnter a number and hit enter.\n");
        print_commands();
        printf("\n");
        *current = 0;
        return(current);
    }

    /*  Entering anything that starts with 'u' counts as undo */
    if( line[0] == 'u' )
    {
        undo_prev();
        *current = 0;
        return(current);
    }

    /*
     * If the user entered a blank line, a plus sign or only two dots, we repeat
     * the previous operation once, unless we'd previously cleared the register
     */
    if( strcmp(line, "+\n") == 0 || strcmp(line, "..\n") == 0 ||
            line[0] == '\n' )
    {
        if( history->commentCode == 'R' )
        {
            *current = 0;
        }
        else
        {
            *current = history->number;
        }
        return(current);
    }

    /*
     * If the user enters an integer followed by two dots, or two dots followed
     * by an integer (n), the previous operation will be repeated n times
     */
    if( line[0] == '.' && line[1] == '.' )  //  FRONT DOTS
    {
        int repeat = 0;
        sscanf( line, "..%d", &repeat );

        *current = ( history->number ) * repeat;

        return(current);
    }
    if( strcmp( &line[strlen(line) - 3 ], "..\n" ) == 0 )    //  REAR DOTS
    {
        int repeat = 0;
        sscanf( line, "%d..", &repeat);

        *current = ( history->number) * repeat;

        return(current);
    }

    /*
     * If the user enters only a minus sign, the number from the previous
     * operation is subtracted from running total.  This is different from the
     * undo function because it is added to the operation history
     */
    if( strcmp(line, "-\n") == 0 )
    {
        *current = (history->number) * -1;

        /*  Tell the user what they just did */
        if( *current > 0 )
            printf("\n+%g\n\n", *current);
        else
            printf("\n%g\n\n", *current);

        return(current);
    }


    /*  User wants to save w/custom filename */
    if( line[0] == 's' && strcmp(line, "s\n") != 0 )
    {
        counter = 0;    //  Reset counter

        /*  While our character isn't null or a newline */
        while( line[counter] != '\n' && line[counter] != '\0' )
        {
            /*  If we find a space, assign that address to saveLocation ptr */
            if( line[counter] == ' ' )
            {
                saveLocation = malloc(256);
                if( saveLocation == NULL )
                {
                    mem_error("function:  get_entered:\n\
                            Cannot assign memory for saveLocation");
                }

                strcpy(saveLocation, &line[counter] + 1);
            }

            ++counter;  //  Increment counter to move along the line of input
        }

        /*  Null out the second-to-last char (newline) */
        saveLocation[ strlen(saveLocation) - 1 ] = '\0';

        /*  Send the location to the save function */
        save_file( saveLocation );
    }


    /*  User wants to quicksave (prev filename or ttab_yyyy-mm-dd_hh-mm-ss */
    if( strcmp(line, "/\n") == 0 || strcmp(line, "s\n") == 0 )
    {
        if( saveLocation == NULL )
        {
            saveLocation = malloc(256);
            if( saveLocation == NULL )
            {
                mem_error("function:  get_entered:\n\
                        Cannot assign memory for saveLocation");
            }

            sprintf(saveLocation, "ttab_%s.log", get_date_string(1));

            save_file(saveLocation);
        }
        else
        {
            save_file(saveLocation);
        }
    }


    /*  Find the number */
    sscanf(line, "%lf", &temp);

    lineFront = line[0];
    lineBack = line[ strlen(line) - 2 ];


    /*  Check for no entry */
    if( temp == 0 )
        *current = 0;
    else
        *current = temp;

    /*  Check to set the mode */
    if( lineFront == '+' || lineBack == '+' )
        mode = '+';
    else if( lineBack == '-' )
        mode = '-';

    return(current);
}


char* get_date_string(char quickSaving)
{
    /*  Generate time */
    time_t rawTime;
    time(&rawTime);

    /*  Put it into a struct we can use */
    struct tm *theTime;
    theTime = localtime(&rawTime);

    /*  Create a string pointer, allocate memory to it, null it out */
    char *theString = NULL;
    theString = malloc(DATE_STRING_LEN);
    if( theString == NULL )
        mem_error("function:  get_date_string");
    memset(theString, '\0', DATE_STRING_LEN);

    /*  Fill our date string using sprintf */
    if( quickSaving == 1 )
    {
        sprintf(theString, "%02d-%02d-%02d_%02d-%02d-%02d",
                theTime->tm_year+1900, theTime->tm_mon+1, theTime->tm_mday,
                theTime->tm_hour, theTime->tm_min, theTime->tm_sec);
    }
    else
    {
        sprintf(theString, "%02d-%02d-%02d  %02d:%02d:%02d",
                theTime->tm_year+1900, theTime->tm_mon+1, theTime->tm_mday,
                theTime->tm_hour, theTime->tm_min, theTime->tm_sec);
    }

    /*  Return pointer */
    return( theString );
}


void add_to_undo(double *current, char cc)
{
    /*  Create our history node */
    struct action *temp = create_node();

    temp->number = *current;        //  The number added during operation
    temp->runningTotal = total;     //  Total after operation

    /*  Getting our date / time */
    char *dateString = get_date_string(0);
    strncpy(temp->date, dateString, DATE_STRING_LEN);
    free(dateString);
    dateString = NULL;

    /*  Comment code, see struct definition for codes */
    temp->commentCode = cc;

    /*  Swap our nodes around */
    temp->prev = history;
    history->next = temp;
    history = temp;

}


void do_math(double *current)
{
    switch( mode )
    {
        /*
         * EXPLICIT SUBTRACTION
         *  Normally, a number can just be added to the total; when the minus
         *  sign is to the right of the number, though, we have to do it
         *  manually
         */
        case '-':
            total -= *current;
            add_to_undo( current, 's' );
            break;
        default:
            total += *current;
            if( *current < 0 )
                add_to_undo( current, 's' );
            else if( *current > 0 )
                add_to_undo( current, 'a' );
            break;
    }
}


void undo_prev(void)
{
    /*  Check to make sure we have a previous node to fall back to */
    if( history->prev != NULL )
    {
        /*  Reverse previous arithmetic */
        total -= history->number;

        /*  Print the undo string */
        printf("\nUNDO\t( ");
        if( history->number > 0 )
            printf("%g )\n\n", (history->number) * -1 );  //  Print negative no.
        else
            printf("+%g )\n\n", (history->number) * -1 ); //  Print positive no.
        
        history = history->prev;    //  Assign current history to previous node
        free(history->next);        //  Free memory allocated to node
        history->next = NULL;       //  Null out the pointer
    }
}


/*  Free allocated memory, null out pointers */
void clean_up(void)
{
    /*
     * Here we step back through the history, node by node, until we get to the
     * original.  At each node, we free the allocated space and null the pointer
     * from which we just arrived
     */
    while( history->prev != NULL )
    {
        history = history->prev;
        free( history->next );
        history->next = NULL;
    }

    /*  Free the final node (both *history and *undo point to same address ) */
    free(history);

    /*  Finally, null out our pointers */
    history = NULL;
    undo = NULL;
}



int main(int argc, char *argv[])
{

    if( argc > 2 )
    {
        print_usage();
        return(1);
    }
    else if( argc == 2 )
    {
        if( strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0 )
        {
            print_help();
            return(0);
        }

        if( strcmp(argv[1], "-V") == 0 || strcmp(argv[1], "--version") == 0 )
        {
            print_version_info();
            return(0);
        }

        if( strcmp(argv[1], "-") == 0 )
        {
            sum_log_stdin();
        }
        else
        {
            FILE *fp = fopen(argv[1], "r");
            if( fp == NULL )
            {
                fprintf(stderr, "ERROR:  Cannot open file for writing: %s\n",
                        argv[1]);
                printf("\n");
                print_usage();
                printf("\n");
                print_options();
                return(1);
            }

            sum_log(fp);
            fclose(fp);
        }

        return(0);
    }


    /*  Initialize some numbers */
    total = 0;
    entered = 0;
    double *current = &entered;

    /*  Initialize the history nodes */
    undo = create_node();
    history = create_node();
    undo->next = history;
    history->prev = undo;

    /*  Init saveLocation */
    saveLocation = NULL;

    while(1)
    {
        /*  We always reset the mode to addition at the top of the loop */
        mode = '+';
        print_prompt();
        current = get_entered(current);
        do_math(current);
    }


    return(0);
}
