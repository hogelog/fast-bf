# fast-bf

Where is fastest brainf**k intepreter?

It's fast-bf!

But [rdebath/Brainfuck](https://github.com/rdebath/Brainfuck) is more faster!

## Isssues

## Require
- g++
- xbyak (used by bf-jit)

## Build
    $ make

## Usage
    $ ./bf-opt-jit sample/mandelbrot.b

## Description
### bf-vm-opt
optimized vm implementation

- [direct threading](http://en.wikipedia.org/wiki/Threaded_code#Direct_threading)
- optimize some pattern

### bf-jit
jit compiler (x86) implementation

- no optimization

### bf-jit-opt
optimized x86 jit compiler implementation

- fastest in these interpreters


## Sample
- hello.bf ([http://www.kmonos.net/alang/etc/brainfuck.php](http://www.kmonos.net/alang/etc/brainfuck.php))
- mandelbrot.b, mandelbrot-huge.b, mandelbrot-titannic.b ([http://esoteric.sange.fi/brainfuck/utils/mandelbrot/](http://esoteric.sange.fi/bra
infuck/utils/mandelbrot/))
- long.b ([http://mazonka.com/brainf/](http://mazonka.com/brainf/))

## Authors
- [hogelog](https://github.com/hogelog)
- [todesking](https://github.com/todesking)
 - more optimization
 - useful commandline arguments

## See Also
- [fastest-bf-interpreter@slideshare](http://www.slideshare.net/hogelog/fastest-bf-interpreter)
