// Copyright (c) 2017 Kai Luo <gluokai@gmail.com>. All rights reserved.
#include <chrono>
#include <thread>

#include "kl/logger.h"
#include "kl/netdev.h"
#include "kl/testkit.h"
#include "kl/udp.h"
#include "pcap++.h"

class T {};

TEST(T, DefaultDevice) {
  auto device = kale::pcap::DefaultDevice();
  ASSERT(device);
  KL_DEBUG("default device %s", (*device).c_str());
}

TEST(kale::pcap::Sniffer, Constructor, "lo") {}

TEST(T, CompileAndInstall) {
  auto device = kale::pcap::DefaultDevice();
  ASSERT(device);
  kale::pcap::Sniffer sniffer((*device).c_str());
  auto compile = sniffer.CompileAndInstall("udp and portrange 50000-65535");
  ASSERT(compile);
  auto fail_compile =
      sniffer.CompileAndInstall("udp and portrange 50000-65536");
  ASSERT(!fail_compile);
}

TEST(T, UDPDump) {
  const std::string message("wtf~imfao~rofl");
  const char *ifname = "lo";
  const uint16_t port = 4000;
  kale::pcap::Sniffer sniffer("lo");
  auto compile = sniffer.CompileAndInstall(
      kl::FormatString("udp and port %u", port).c_str());
  ASSERT(compile);
  auto send_thread = std::thread([ifname, port, message] {
    auto addr = kl::netdev::GetAddress(ifname);
    ASSERT(addr);
    KL_DEBUG("address %s", (*addr).c_str());
    auto sock = kl::udp::Socket();
    ASSERT(sock);
    kl::env::Defer defer([fd = *sock] { ::close(fd); });
    auto send = kl::inet::Sendto(*sock, message.c_str(), message.size(), 0,
                                 (*addr).c_str(), port);
    KL_DEBUG("packet sent");
    ASSERT(send);
  });
  struct pcap_pkthdr header;
  KL_DEBUG("waiting for packet");
  auto start = std::chrono::high_resolution_clock::now();
  // FIXME(Kai Luo): sniffer.NextPacket costs too much time
  const uint8_t *packet = sniffer.NextPacket(&header);
  auto now = std::chrono::high_resolution_clock::now();
  std::chrono::duration<float> diff = now - start;
  KL_DEBUG("sniffer.NextPacket costs %fs", diff.count());
  KL_DEBUG("packet size: %u", header.len);
  ASSERT(header.len >= message.size());
  ASSERT(std::string(packet + header.len - message.size(),
                     packet + header.len) == message);
  send_thread.join();
}

void LoopAndBreakLoopHandler(uint8_t *user, const struct pcap_pkthdr *header,
                             const uint8_t *packet) {
  KL_DEBUG("packet size: %u", header->len);
}

TEST(T, LoopAndBreakLoop) {
  const std::string message("wtf~imfao~rofl");
  const char *ifname = "lo";
  const uint16_t port = 4000;
  kale::pcap::Sniffer sniffer("lo");
  auto compile = sniffer.CompileAndInstall(
      kl::FormatString("udp and port %u", port).c_str());
  ASSERT(compile);
  auto send_thread = std::thread([ifname, port, &message, &sniffer] {
    auto addr = kl::netdev::GetAddress(ifname);
    ASSERT(addr);
    KL_DEBUG("address %s", (*addr).c_str());
    auto sock = kl::udp::Socket();
    ASSERT(sock);
    kl::env::Defer defer([fd = *sock] { ::close(fd); });
    auto send = kl::inet::Sendto(*sock, message.c_str(), message.size(), 0,
                                 (*addr).c_str(), port);
    KL_DEBUG("packet sent");
    ASSERT(send);
    sniffer.BreakLoop();
    KL_DEBUG("break loop called");
  });
  KL_DEBUG("entering loop");
  auto callback = [&message](const struct pcap_pkthdr *header,
                             const uint8_t *packet) {
    KL_DEBUG("packet size: %u", header->len);
    ASSERT(header->len >= message.size());
    ASSERT(std::string(packet + header->len - message.size(),
                       packet + header->len) == message);
  };
  KL_DEBUG("waiting for packet");
  auto start = std::chrono::high_resolution_clock::now();
  // FIXME(Kai Luo): sniffer.Loop costs too much time
  auto loop = sniffer.Loop(-1, callback);
  auto now = std::chrono::high_resolution_clock::now();
  std::chrono::duration<float> diff = now - start;
  KL_DEBUG("sniffer.Loop costs %fs", diff.count());
  ASSERT(loop);
  KL_DEBUG("loop exited");
  send_thread.join();
}

int main() { return KL_TEST(); }
