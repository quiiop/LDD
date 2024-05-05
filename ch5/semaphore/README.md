###### 測試semaphore的lock機制  
user1.c 會不斷的寫1111111和讀，user2.c 會不斷的寫2222222和讀，hello driver會用semaphore管控，如果你在user1、2的終端  
看到打印和自己寫入不同的數值，這個是正常的，因為我設計read or write完後就會釋放semaphore，所以假設user1 write 1111111  
有可能user2 取得semaphore user2 write 2222222，之後又被user1取得semaphore，user1 執行read就讀到2222222。 