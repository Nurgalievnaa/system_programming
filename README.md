# Debugfs operations

What does it do?


The kernel module creates three files in debugfs:
    /sys/kernel/debug/fortytwo/jiffies
    /sys/kernel/debug/fortytwo/user1
    
 In file user1 is changed only by root, but has access read by anyone
 The file jiffies is only readable by any user and we can use it only share information 
    


