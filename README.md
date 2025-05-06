# Terminal Brickout

A simple brick-breaking game playable in terminal

## How to play

- Use the **Left** and **Right arrow keys** to move the paddle.
- Press the **Up arrow key** to launch the ball.
- Break all the bricks without letting the ball fall!

## Requirements

- **Linux**
- **C Compiler** (e.g., `gcc`)
- **ncurses** library
- **UTF-8** capable terminal (for full emoji rendering)

***
<br/>

> If your terminal does not support emoji or renders them incorrectly, compile using the ASCII fallback by adding the -DUSE_ASCII flag:

```bash
gcc -DUSE_ASCII -o brickout main.c -lncurses
```
