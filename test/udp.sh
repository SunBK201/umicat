# servr:
ncat --udp --listen --keep-open --exec "/bin/cat" -p 12345
# client: echo "hello world" | nc -u 127.0.0.1 10201
