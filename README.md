# iklang

A stack based language written in C. Compiles to x86-64 assembly. Please note that this language is only designed to run on my machine. That means that certain features may be broken on yours. While i don't expect this to work on windows out of the box, you could also have issues on linux systems.

## building
```sh
cd iklang
make clean && make build
```

## example
```sh
touch foo.ikl
echo "1 1 + dump" > foo.ikl
.build/iklc foo.ikl
./out 
# >> 2
```

## contributors
This project is heavily inspired by the fabolous works of [Alexey Kutepov](https://rexim.github.io/). Keep that in mind if you see parallels in paradigms and code in general.

## contributing
Feel free to create pull requests or fork and distribute code as you want. Please consider mentioning me when you do. 

## developement note
This project uses AI-assisted static analysis and refactoring. I am open and verbose about using AI, and AI only comes to usage to:
- Fix Bugs: instead of throughly wasting hours of searching for critical bugs in the codebase, AI has been specifically trained to do so. This not only saves time but ensures more bugs to be found and resolved. 
- Clean up syntax: it often happens that there occur inconsistencies between applied paradigms. 
