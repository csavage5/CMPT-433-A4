Directions for building drivers


-----------------------
Option 1: Cross-Compile
-----------------------
- Follow driver creation guide to setup cross-compile of kernel (via RobertCNelson's bb-kernel repo)
- from Makefile run
	make all:   Builds
	make clean: Cleans
	


------------------------
Option 2: Native Compile
------------------------
- Copy code and native makefile to NFS folder for compilation on target
  On host, run:
    $ make -f Makefile_native deploy
  
    (the Makefile_native is copied and renamed to Makefile)
  
- On the target:
	# cd /mnt/remote/drivers
	# make
	
	# modinfo echo.ko
	# insmod echo.ko
	 
- May need to do a 'make clean' on both host and target before doing a deploy
  (file write permissions etc).