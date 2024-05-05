###### 測試semaphore的lock機制  
在hello.c driver裡，hello_read()會取得semaphore，但是不會釋放，需要透過hello_write()來釋放  
而user1.c負責一直read，user2.c負責write()一次，所以流程是user1.c每次read一次就會block，需要user2.c去write()釋放semaphore  
啊 read的資料是空的沒關係。 