# cell
cellular automata (left game) - 元胞自动机（生命游戏）

[![preview](http://i2.hdslb.com/bfs/archive/23b2f6bf86f86b4cb7d1afc94da0d1014627e5a5.jpg)](https://player.bilibili.com/player.html?aid=610527769&bvid=BV1184y1K7kr&cid=1040881319&page=1)

## Build / 构建

```
make
```

> If you build failed, please use [makemake](https://github.com/hubenchang0515/makemake) to generate a new `Makefile`.

## Dependency / 依赖

* [SDL2](http://www.libsdl.org/)

## Rule / 规则

1. A hole, which is surrounded by 3 cells exactly, will birth a cell.
2. A cell, which is surrounded by less than 2 cells, will die of loneliness.
3. A cell, which is surrounded by more than 3 cells, will die of crowd.

See: [Cellular automaton - Wikipedia](https://en.wikipedia.org/wiki/Cellular_automaton)

## Usage / 使用

```
Usage: ./cell [FILE]
Create a cell world with FILE ('cell.cfg' in current directory by default).
```

Scroll mouse wheel can zoom the view port. Press mouse left button and move mouse can drag the view port.

滚动鼠标滚轮可以缩放视野，按住鼠标左键移动鼠标可以拖动视野。

## Config / 配置

The default configuration file is cell.cfg in current directory.

默认的配置文件为当前目录下的 cell.cfg 文件。

The first line means the world size. Then every line means a postion of a cell.

第一行表示世界的大小，之后每一行表示一个细胞的坐标。

For example:

```
800 600
0 0 
1 1 
2 2
3 3
4 4
5 5
6 6
```
