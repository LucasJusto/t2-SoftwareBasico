#
# Makefile para Linux e macOS
#

SOURCE  = main.c lib/SOIL/image_DXT.c lib/SOIL/image_helper.c lib/SOIL/SOIL.c lib/SOIL/stb_image_aug.c
OBJECTS = $(SOURCE:.c=.o)

UNAME = `uname`

all: $(TARGET)
	-@make $(UNAME)

Darwin: $(OBJECTS)
	gcc $(OBJECTS) -Wno-deprecated -framework OpenGL -framework GLUT -framework Cocoa -lm -o seamcarving

Linux: $(OBJECTS)
	gcc $(OBJECTS) -lGL -lGLU -lglut -lm -o seamcarving

clean:
	@ rm -f $(OBJECTS) seamcarving
