gcc -std=gnu99 -g -Wall -Wextra \
    `pkg-config --cflags x11 xft` \
    src/main.c \
    `pkg-config --libs x11 xft` \
    -o br
