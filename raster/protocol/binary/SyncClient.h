/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include "raster/net/NetUtil.h"
#include "raster/protocol/binary/SyncTransport.h"
#include "raster/util/Logging.h"

namespace rdd {

class BinarySyncClient {
public:
  BinarySyncClient(const std::string& host, int port)
    : peer_(host, port) {
    init();
  }
  BinarySyncClient(const ClientOption& option)
    : peer_(option.peer) {
    init();
  }
  virtual ~BinarySyncClient() {
    close();
  }

  void close() {
    if (transport_->isOpen()) {
      transport_->close();
    }
  }

  bool connect() {
    try {
      transport_->open();
    }
    catch (std::exception& e) {
      RDDLOG(ERROR) << "BinarySyncClient: connect " << peer_
        << " failed, " << e.what();
      return false;
    }
    RDDLOG(DEBUG) << "connect peer[" << peer_ << "]";
    return true;
  }

  bool connected() const {
    return transport_->isOpen();
  }

  bool fetch(ByteRange& _return, const ByteRange& request) {
    try {
      transport_->send(request);
      transport_->recv(_return);
    }
    catch (std::exception& e) {
      RDDLOG(ERROR) << "BinarySyncClient: fetch " << peer_
        << " failed, " << e.what();
      return false;
    }
    return true;
  }

private:
  void init() {
    transport_.reset(new BinarySyncTransport(peer_));
    RDDLOG(DEBUG) << "SyncClient: " << peer_;
  }

  Peer peer_;
  std::unique_ptr<BinarySyncTransport> transport_;
};

} // namespace rdd
