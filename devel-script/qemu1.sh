 sudo qemu-system-x86_64  -no-kvm -smp 1 -m 15G -fda ./qdisk.img -localtime -M pc -nographic -curses -monitor telnet:127.0.0.1:1234,server,nowait 
