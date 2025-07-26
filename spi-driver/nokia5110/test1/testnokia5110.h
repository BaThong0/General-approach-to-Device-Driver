#ifndef __TESTNOKIA5110_H__
#define __TESTNOKIA5110_H__
struct position {
    uint8_t x;
    uint8_t y;
};

#define NEXT_LINE   _IO('a', 1)
#define GOTO_XY     _IOW('a', 2, struct position*)
#define CLEAR       _IO('a', 3)
#define PRINT_IMAGE _IOW('a', 4, uint8_t* data)
#endif // __TESTNOKIA5110_H__