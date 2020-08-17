/*************************************************************************************
 * Author: Mae LaPresta
 * Title: keygen.c
 * Description: Generates a randomized key of a user inputted length, made up of the
 *              26 capital letters and the space character.
 *************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void getRandKeyChar (char *);

int main(int argc, const char * argv[]) {
    
    if (argc<=1){
        printf("ERROR: No keylength given.");
        exit(1);
    }
    srand(time(0));                                   
    char *key;
    int keyLength=atoi(argv[1]);                       
    key=malloc(sizeof(char)*(keyLength+1));             
    int i;
    
    for (i=0; i<keyLength; i++){
        getRandKeyChar(&key[i]);
    }
    key[keyLength]='\n';
    printf("%s", key);
    return 0;
}


/**** getRandKeyChar******************************************************************
* Description: Takes a char * and fills it with a random character, the random
*              character must either be a capitalized letter or space.
*   Parameter: Pointer to a character
*    Modified: character
*      Return: none
*************************************************************************************/

void getRandKeyChar (char *storeChar){
    int lower=0;                                        
    int upper=26;                                       
    int randomNumber=(rand()%(upper-lower+1))+lower;    
    if (randomNumber==upper)
        *storeChar=(char)' ';                           //store space in parameter character input
    else
        *storeChar=(char)(randomNumber+65);             //or else store the ASCII value of A-Z
                                                            //in parameter character input
}
