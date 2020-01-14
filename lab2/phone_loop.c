#include <stdio.h>
#include <stdlib.h>

int main() {
   char phone[11];
   int num;
   int to_return = 0;
   scanf("%11s", phone);
   while(1) {
	int res = scanf("%d", &num);
	if (res == EOF) break;
	else {
	  if (num == -1) {
	      printf("%s\n", phone);
	  } else if (num >= 0 && num <= 9) {
	      printf("%c\n", phone[num]);
	  } else {
	      printf("ERROR\n");
	      to_return = 1;
	  }
	}
   }
   return to_return;
}
