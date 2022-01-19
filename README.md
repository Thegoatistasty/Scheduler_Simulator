# OS2021_Hw3
* [Hw3 requirements](https://docs.google.com/presentation/d/1UFuPUwd17Hogh5Vp8GZbnrLRAddGvC1j/edit#slide=id.p3)
## Objective
* Understand how to implement a user-level thread scheduler
* Understand how to implement signal handler
* Understand how to realize asy./deferred thread cancellation 
## Compile
    make simulator
## Execute
    ./simulator
## See thread info
    Ctrl+Z
## Input file format
To initialize a new thread(modified in init_threads.json), you must assign 
* name
* entry function(defined in function_libary.c)
* priority(H, M or L)
* cancel mode(1 for deferred cancellation type, 0 for asynchronous cancellation type)  
##### Format
    {   
            "name" : "f1",  
		"entry function" : "Function1",  
		"priority": "M",  
		"cancel mode": "1"  
    }  
