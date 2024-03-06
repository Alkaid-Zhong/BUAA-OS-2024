#include <stdio.h>

int check(int n) {

	if (1 <= n && n <= 9) {
		return 1;
	} else if (10 <= n && n <= 99) {
		return (n / 10) == (n % 10);
	} else if (100 <= n && n <= 999) {
		return (n / 100) == (n % 10);
	} else if (1000 <= n && n <= 9999) {
		return ( ( (n / 100) % 10) == ( (n / 10) % 10) ) && ( (n / 1000) == (n % 10) );
	}
	return 0;

}

int main() {
	int n;
	scanf("%d", &n);

	if (check(n)) {
		printf("Y\n");
	} else {
		printf("N\n");
	}
	return 0;
}
