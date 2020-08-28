EXE:=qcloud_test

EXTRA_CFLAGS += -O2 -Wall -g $(CFLAGS)

LDFLAGS := -lcrypto -lssl -lcurl

SRCSC = $(wildcard *.c)
OBJS = $(patsubst %.c, %.o, $(SRCSC))


all: $(EXE) 

$(EXE): $(OBJS)
	$(CC) $(EXTRA_CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

clean:
	-@rm -f *~ *.o $(EXE) *.d

$(OBJS): %.o : %.c
	$(CC) $(EXTRA_CFLAGS) -c $< -o $@

