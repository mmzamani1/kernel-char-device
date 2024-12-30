cmd_/home/coding/OS/ex1/myQueue.mod := printf '%s\n'   myQueue.o | awk '!x[$$0]++ { print("/home/coding/OS/ex1/"$$0) }' > /home/coding/OS/ex1/myQueue.mod
