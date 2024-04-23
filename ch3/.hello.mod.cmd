savedcmd_/home/kuo/Desktop/LDD/ch3/hello.mod := printf '%s\n'   hello.o | awk '!x[$$0]++ { print("/home/kuo/Desktop/LDD/ch3/"$$0) }' > /home/kuo/Desktop/LDD/ch3/hello.mod
