cc -std=gnu99 -g -Wall -Wextra \
    `pkg-config --cflags xcb freetype2` \
    src/main.c \
    `pkg-config --libs xcb freetype2` \
    -o br
