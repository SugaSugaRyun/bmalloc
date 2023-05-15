bmalloc
====
buddy memory allocation을 이용한 c언어로 구현된 메모리 할당기 라이브러리다.

Install and Build
-----
To clone the repository you should have Git installed. Just run:

    $ git clone https://github.com/SugaSugaRyun/bmalloc.git

To build the library with some test program, run `make`. 

    $ make  

Or if you want to compile with other program, you have to include following option

    $ -fpack-struct

To delete *.o files and excutable file(test1~4), run
`make clean`.

    $ make clean

How to use
----
You can use this library like amalloc with these functions  
-bmalloc(size_t s): s must be in range 1~4087  
-brealloc(void * p, size_t s)  
-bfree(void * p) 