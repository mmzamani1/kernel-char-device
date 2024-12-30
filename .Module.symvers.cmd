cmd_/home/coding/OS/ex1/Module.symvers :=  sed 's/ko$$/o/'  /home/coding/OS/ex1/modules.order | scripts/mod/modpost -m -a -E   -o /home/coding/OS/ex1/Module.symvers -e -i Module.symvers -T - 
