#include <stdlib.h>
#include <stdio.h>


/* This program reads 2 values of standard input
 * 10 character string and an integer 
 *
 * if the integer is -1, program prints full string to stdout
 * if the integer is between 0 - 9, program prints only the corresponding digit from the string to stdout
 * in thse cases program returns 0
 *
 * if the integer is less than -1 or greater than 9 the program prints "ERROR" and return 1
 */

int main(){
    
    char phone[11];
    int num;
    scanf("%11s %d", phone, &num);
    
    if (num == -1) {
	printf("%s\n", phone);
	return 0;	
    } else if (num >= 0 && num <= 9) {
	printf("%c\n", phone[num]);
	return 0;
    } else {
	printf("ERROR\n");
	return -1;
    }
    printf("%s, %d are the inputs\n", phone, num);

    return 0;
}
