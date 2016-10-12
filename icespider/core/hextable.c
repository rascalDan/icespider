// GCC doesn't support this sort of initialization in C++, only plain C.
// https://gcc.gnu.org/onlinedocs/gcc-5.4.0/gcc/Designated-Inits.html

const long hextable[] = {
	[0 ... '0' - 1] = -1,
	['9' + 1 ... 'A' - 1] = -1,
	['G' ... 'a' - 1] = -1,
	['g' ... 255] = -1,
	['0'] = 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
	['A'] = 10, 11, 12, 13, 14, 15,
	['a'] = 10, 11, 12, 13, 14, 15
};

