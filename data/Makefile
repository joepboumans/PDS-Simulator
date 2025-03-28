CXX=g++
RM=rm -f
LDLIBS=-lpcap
SRCS=pcap_parser.cpp
OBJS=$(SRCS)
SRCS_IPV6=remove_ipv6.cpp
OBJS_IPV6=$(SRCS_IPV6)

all: pcap_parser remove_ipv6

pcap_parser: $(OBJS)
		$(CXX) -o pcap_parser $(OBJS) $(LDLIBS)

remove_ipv6: $(OBJS_IPV6)
		$(CXX) -o remove_ipv6 $(OBJS_IPV6) $(LDLIBS)

%.o: %.cpp 
		${CXX} ${CPPFLAGS} -c $<

clean:
		$(RM) $(OBJS) $(OBJS_IPV6)

distclean: clean
		$(RM) pcap_parser remove_ipv6
