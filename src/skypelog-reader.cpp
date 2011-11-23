/* http://www.ndmteam.com/skype-chat-logs-dissection/

6C 33 33 6C __ __ __ __ __ __ __ __ __ __ __ __ __ __
E0 03 23 -- -- -- ...   2F 34 -- -- -- ...   3B
         ^          ^
         +----------+- name1  +----------+- name2

1. Every record starts with byte sequence 0x6C 0x33 0x33 0x6C (l33l in ASCII)
2. Next 14 bytes are there with unknown (at least for me) purpose
3. 0xE0 0x03 – marker for the beginning of chat members field
   first chat member username is prefixed with 0x23 (# in ASCII)
   two chat members are separated with 0x2F (/ in ASCII)
   the second chat member username is prefixed with 0x34 ($ in ASCII)
   the list of chat members ends with 0x3B (; in ASCII)

   Remark: I still have some problems with correct interpretation of this field for records with more then two chat members

4. The bytes after 0x3B to the next described number are with unknown content
5. 0xE5 0x03 – marker for the beginning of 6 bytes sequence, representing the message timestamp. The numbers in all chat logs are stored in little-endian format. The fifth and the sixth byte seems to be constant in all the records - 0x04 0x03. The sixth byte is not used in the actual timestamp calculations (for now ... may be it'll be used in further moment). Bytes 1st to 5th represent message timestamp in standard Unix format.Normally only 4 bytes of information are needed to store Unix timestamp. That's why first I thought that bytes 5th and 6th are not used at all. But after some calculations it came clear that first 4 bytes did not represent the actual time since 1/1/1970. It came clear also that the most significant bit in every of the first 4 bytes is always 1. That's why it seems logically to me to conclude that those bits are sign bits and that they shouldn't be used in actual timestamp calculations. Striping those most significant bits from every of the first 4 bytes and combining the rest of the bits it was received 28bit combination. For the standard Unix time representation 32 bits of information are needed, so we just 'lend' last 4 bits from 5th byte. This 32 bit combination gave the Unix timestamp of the chat message
6. 0xE8 0x03 – marker for the beginning of the sender username field. The field ends with zero byte 0x00
7. 1.0xEC 0x03 – marker for the beginning of the sender screen name field. The field ends with zero byte 0x00
8. 1.0xFC 0x03 – marker for the beginning of the message field. The field ends with zero byte 0x00

*/


// C-Library
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <csetjmp>
#include <cstdlib>

// C++-Library
#include <iostream>
#include <algorithm>

// POSIX-Library
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "skypelog-reader.hpp"



template<std::size_t N>
constexpr std::pair<const char*,const char*> make_rangepair(const char (&a)[N])
{ return {a, a+N-1}; }

enum {
    SEC_START_REC = 0,
    SEC_START_CHAT,
    SEC_START_SENDER,
    SEC_MEM_SEP,
    SEC_START_RECIPIENTS,
    SEC_END_MEMBS,
    SEC_START_TIME,
    SEC_START_MSG,
    SEC_NUM_SECTIONS
};


const std::pair<const char*, const char*> sections[] = {
    make_rangepair("\x6C\x33\x33\x6C"),  /* start_rec  */
    /* // skip 14 */
    make_rangepair("\xE0\x03"),  /* start_chat */
    make_rangepair("\x23"),  /* start_sender */
    make_rangepair("\x2F"),  /* mem_sep */
    make_rangepair("\x34"),  /* start_recipients */
    make_rangepair("\x3B"),  /* end_membs */
    make_rangepair("\xE5\x03"),  /* start_time */
    make_rangepair("\xFC\x03"),  /* start_msg */
};

const char prog[] = "skype-dbb-read";

time_t read_time(char* start)
{
    time_t t;
    char *t_ptr = reinterpret_cast<char*>(&t);

    // drop msb of every byte, join the remaining bits together
    t_ptr[0] = ((start[1] << 7) & 0x80) | ((start[0] >> 0) & 0x7F);
    t_ptr[1] = ((start[2] << 6) & 0xC0) | ((start[1] >> 1) & 0x3F);
    t_ptr[2] = ((start[3] << 5) & 0xE0) | ((start[2] >> 2) & 0x1F);
    t_ptr[3] = ((start[4] << 4) & 0xF0) | ((start[3] >> 3) & 0x0F);

    return t;
}


char *find_section(char *start, char *data, size_t len, int n)
{
    std::size_t seclen = sections[n].second - sections[n].first;
    char *pos = (char *) memmem(
        start, len - (start - data),
        sections[n].first, seclen
    );

    if(!pos) return NULL;

    return pos + seclen;
}

void parse_data(char *data, size_t len)
{
	char *ptr;
	char *save;
	time_t time;
#define TIMESTR_MAX 18
	char timestr[TIMESTR_MAX];

	ptr = data;

	/* bail if we don't find a section, only warning if it's not the inital section */
#define FIND_SECTION(ptr, data, len, en) \
	do{ \
		ptr = find_section(ptr, data, len, en); \
		if(!ptr){ \
			if(en != 0) \
				fprintf(stderr, "%s: warning: couldn't find section %d\n", prog, en); \
			return; \
		} \
	}while(0)

    do{
        char *sender, *recipients, *msg;

FIND_SECTION(ptr, data, len, SEC_START_REC);
		ptr += 14;
		FIND_SECTION(ptr, data, len, SEC_START_CHAT);
		FIND_SECTION(ptr, data, len, SEC_START_SENDER);
		save = ptr;
		FIND_SECTION(ptr, data, len, SEC_MEM_SEP);
		ptr[-1] = '\0';
		sender = save;

		ptr++; /* = FIND_SECTION(ptr - 1, data, len, 4); */
		save = ptr;
		FIND_SECTION(ptr, data, len, SEC_END_MEMBS);
		ptr[-1] = '\0';
		recipients = save;

		FIND_SECTION(ptr, data, len, SEC_START_TIME);
		time = read_time(ptr);
		ptr += 6;

		FIND_SECTION(ptr, data, len, SEC_START_MSG);
		save = ptr;
		ptr = reinterpret_cast<char *>(
            memchr(ptr, 0, len - (ptr - data)));
		msg = save;

        strftime(timestr, TIMESTR_MAX, "%Y-%m-%d.%H%M%S", localtime(&time));
        printf("%s: %s <-> %s: %s\n", timestr, sender, recipients, msg);
    } while(1);
}

int process_file(const char *fname)
{
    int fd = open(fname, O_RDONLY);
    if (fd == -1)
    {
        perror("Failed to open file for reading");
        return 1;
    }

    off_t filesize = lseek(fd, 0, SEEK_END);

    void *buf = mmap(0, filesize, PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (buf == MAP_FAILED)
    {
        perror("Failed to memory-map file");
        close(fd);
        return 1;
    }


	parse_data(static_cast<char *>(buf), filesize);

    munmap(buf, filesize);
    close(fd);

	return 0;
}



