CXX=g++
RM=rm -f
LDLIBS=-lpcap
SRCS=pcap_parser.cpp
OBJS=$(SRCS)

all: pcap_parser

pcap_parser: $(OBJS)
		$(CXX) -o pcap_parser $(OBJS) $(LDLIBS)

%.o: %.cpp 
		${CXX} ${CPPFLAGS} -c $<

clean:
		$(RM) $(OBJS)

distclean: clean
		$(RM) pcap_parser
