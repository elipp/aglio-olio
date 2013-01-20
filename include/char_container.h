#ifndef CHAR_CONTAINER_H
#define CHAR_CONTAINER_H

typedef struct char_container {

        char *position;
        char *data;
        char *last;
        int linelength();

        long size;

        char_container(char *data, long size) : data(data), size(size)
        {
                position = data;
                last = &data[size-1];
        }

        inline void gline(char * const buffer);
        inline void jumpto_nextline();
	inline void jumpto_offset(int &offset);

} char_container;

int char_container::linelength()
{
        char *tmp_iter = position;

        while( *tmp_iter != '\n')
        {
                tmp_iter++;
        }

        tmp_iter++;             // in order to point to the last character

        return (tmp_iter - position);
}

inline void char_container::gline(char * const buffer) {

        int i = 0;

        for (; i < linelength(); i++)
        {
                buffer[i] = *(position + i);
        }

        buffer[i] = '\0';       // this appears to have fixed ze excess newline characters - issue
}

inline void char_container::jumpto_nextline()
{
        position += linelength();
}

inline void char_container::jumpto_offset(int &offset)
{
	position = data + offset;
}

#endif
