AM_CPPFLAGS = -I$(srcdir)/../util $(CXX11_FLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

noinst_LIBRARIES = libpacket.a

libpacket_a_SOURCES = packetshell.hh packetshell.cc queued_packet.hh \
                      abstract_packet_queue.hh dropping_packet_queue.hh dropping_packet_queue.cc infinite_packet_queue.hh \
                      dual_dropping_packet_queue.cc dual_dropping_packet_queue.hh \
                      drop_tail_packet_queue.hh drop_head_packet_queue.hh \
                      codel_packet_queue.cc codel_packet_queue.hh \
                      pie_packet_queue.cc pie_packet_queue.hh \
                      cellular_packet_queue.cc cellular_packet_queue.hh \
                      rcp_packet_queue.cc rcp_packet_queue.hh \
                      vcp_packet_queue.cc vcp_packet_queue.hh \
                      xcp_packet_queue.cc xcp_packet_queue.hh \
                      weighted_packet_queue.cc weighted_packet_queue.hh \
                      bindworkaround.hh
