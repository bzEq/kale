// Copyright (c) 2017 Kai Luo <gluokai@gmail.com>. All rights reserved.
#ifndef KALE_TUN_H_
#define KALE_TUN_H_
#include <string>
#include <tuple>

#include "kl/error.h"

namespace kale {

extern const char *kTunDevRoot;

// RETURN: fd
kl::Result<int> AllocateTun(const char *ifname);
kl::Result<int> AllocateTunInterface(const char *ifname, const char *host,
                                     const char *mask);
std::string RandomTunName();

}  // namespace kale
#endif
