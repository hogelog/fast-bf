# fast-bf

Where is fastest brainf**k intepreter?

It's fast-bf!

## Isssues
- Is fast-bf really fastest?
 - If you find any faster bf interpreter, [please throw New Issue](http://goo.gl/Xpidz)

## Require
- g++
- xbyak (used by bf-jit)

## Build
    $ make

## Description
### bf-vm-opt
optimized vm implementation

- [direct threading](http://en.wikipedia.org/wiki/Threaded_code#Direct_threading)
- optimize some pattern

### bf-jit
jit compiler (x86) implementation

- no optimization


## Sample
- hello.bf ([http://www.kmonos.net/alang/etc/brainfuck.php](http://www.kmonos.net/alang/etc/brainfuck.php))
- mandelbrot.b, mandelbrot-huge.b, mandelbrot-titannic.b ([http://esoteric.sange.fi/brainfuck/utils/mandelbrot/](http://esoteric.sange.fi/bra
infuck/utils/mandelbrot/))
- long.b ([http://mazonka.com/brainf/](http://mazonka.com/brainf/))
