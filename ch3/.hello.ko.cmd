savedcmd_/home/kuo/Desktop/LDD/ch3/hello.ko := ld -r -m elf_x86_64 -z noexecstack --build-id=sha1  -T scripts/module.lds -o /home/kuo/Desktop/LDD/ch3/hello.ko /home/kuo/Desktop/LDD/ch3/hello.o /home/kuo/Desktop/LDD/ch3/hello.mod.o;  make -f ./arch/x86/Makefile.postlink /home/kuo/Desktop/LDD/ch3/hello.ko