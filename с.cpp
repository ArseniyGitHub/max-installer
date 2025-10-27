#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#define cu8(u8str)  (char*)(void*)u8str

int main() {
	system("chcp 65001");
	long double n1, n2;
	char op;
	printf(cu8(u8"Это калькулятор, ничего лишнего"));
	while (true) {
		printf(cu8(u8"\nВведите 1-ое число: "));
		std::cin >> n1;

		printf(cu8(u8"Введите операцию: "));
		std::cin >> op;

		printf(cu8(u8"Введите 2-ое число: "));
		std::cin >> n2;

		printf(cu8(u8"Результат: "));
		switch (op) {
		case '+': std::cout << (n1 + n2); break;
		case '-': std::cout << (n1 - n2); break;
		case '*': std::cout << (n1 * n2); break;
		case '/': std::cout << (n1 / n2); break;
		default: printf(cu8(u8"Ошибка: Незвестная операция!"));
		}
	}
}