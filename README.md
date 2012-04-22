# fast-bf

fast brainf*ck intepreter

## require
- g++
- xbyak (used by bf-jit)

## build
    $ make

## description
### bf-vm-opt
optimized vm implementation

- [direct threading](http://en.wikipedia.org/wiki/Threaded_code#Direct_threading)
- optimize some pattern

### bf-jit
jit compiler (x86) implementation

- no optimization


## sample
- hello.bf ([http://www.kmonos.net/alang/etc/brainfuck.php](http://www.kmonos.net/alang/etc/brainfuck.php))
- mandelbrot.b, mandelbrot-huge.b, mandelbrot-titannic.b ([http://esoteric.sange.fi/brainfuck/utils/mandelbrot/](http://esoteric.sange.fi/brainfuck/utils/mandelbrot/))
