/**
 * Autogenerated by Thrift Compiler (0.8.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef Scribe_H
#define Scribe_H

#include <TProcessor.h>
#include "scribe_types.h"

namespace rdd {

class ScribeIf {
 public:
  virtual ~ScribeIf() {}
  virtual ResultCode Log(const std::vector<LogEntry> & messages) = 0;
};

class ScribeIfFactory {
 public:
  typedef ScribeIf Handler;

  virtual ~ScribeIfFactory() {}

  virtual ScribeIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) = 0;
  virtual void releaseHandler(ScribeIf* /* handler */) = 0;
};

class ScribeIfSingletonFactory : virtual public ScribeIfFactory {
 public:
  ScribeIfSingletonFactory(const boost::shared_ptr<ScribeIf>& iface) : iface_(iface) {}
  virtual ~ScribeIfSingletonFactory() {}

  virtual ScribeIf* getHandler(const ::apache::thrift::TConnectionInfo&) {
    return iface_.get();
  }
  virtual void releaseHandler(ScribeIf* /* handler */) {}

 protected:
  boost::shared_ptr<ScribeIf> iface_;
};

class ScribeNull : virtual public ScribeIf {
 public:
  virtual ~ScribeNull() {}
  ResultCode Log(const std::vector<LogEntry> & /* messages */) {
    ResultCode _return = (ResultCode)0;
    return _return;
  }
};

typedef struct _Scribe_Log_args__isset {
  _Scribe_Log_args__isset() : messages(false) {}
  bool messages;
} _Scribe_Log_args__isset;

class Scribe_Log_args {
 public:

  Scribe_Log_args() {
  }

  virtual ~Scribe_Log_args() throw() {}

  std::vector<LogEntry>  messages;

  _Scribe_Log_args__isset __isset;

  void __set_messages(const std::vector<LogEntry> & val) {
    messages = val;
  }

  bool operator == (const Scribe_Log_args & rhs) const
  {
    if (!(messages == rhs.messages))
      return false;
    return true;
  }
  bool operator != (const Scribe_Log_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Scribe_Log_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class Scribe_Log_pargs {
 public:


  virtual ~Scribe_Log_pargs() throw() {}

  const std::vector<LogEntry> * messages;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Scribe_Log_result__isset {
  _Scribe_Log_result__isset() : success(false) {}
  bool success;
} _Scribe_Log_result__isset;

class Scribe_Log_result {
 public:

  Scribe_Log_result() : success((ResultCode)0) {
  }

  virtual ~Scribe_Log_result() throw() {}

  ResultCode success;

  _Scribe_Log_result__isset __isset;

  void __set_success(const ResultCode val) {
    success = val;
  }

  bool operator == (const Scribe_Log_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const Scribe_Log_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Scribe_Log_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Scribe_Log_presult__isset {
  _Scribe_Log_presult__isset() : success(false) {}
  bool success;
} _Scribe_Log_presult__isset;

class Scribe_Log_presult {
 public:


  virtual ~Scribe_Log_presult() throw() {}

  ResultCode* success;

  _Scribe_Log_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

class ScribeClient : virtual public ScribeIf {
 public:
  ScribeClient(boost::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) :
    piprot_(prot),
    poprot_(prot) {
    iprot_ = prot.get();
    oprot_ = prot.get();
  }
  ScribeClient(boost::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, boost::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) :
    piprot_(iprot),
    poprot_(oprot) {
    iprot_ = iprot.get();
    oprot_ = oprot.get();
  }
  boost::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return piprot_;
  }
  boost::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return poprot_;
  }
  ResultCode Log(const std::vector<LogEntry> & messages);
  void send_Log(const std::vector<LogEntry> & messages);
  ResultCode recv_Log();
 protected:
  boost::shared_ptr< ::apache::thrift::protocol::TProtocol> piprot_;
  boost::shared_ptr< ::apache::thrift::protocol::TProtocol> poprot_;
  ::apache::thrift::protocol::TProtocol* iprot_;
  ::apache::thrift::protocol::TProtocol* oprot_;
};

class ScribeProcessor : public ::apache::thrift::TProcessor {
 protected:
  boost::shared_ptr<ScribeIf> iface_;
  virtual bool process_fn(apache::thrift::protocol::TProtocol* iprot, apache::thrift::protocol::TProtocol* oprot, std::string& fname, int32_t seqid, void* callContext);
 private:
  std::map<std::string, void (ScribeProcessor::*)(int32_t, apache::thrift::protocol::TProtocol*, apache::thrift::protocol::TProtocol*, void*)> processMap_;
  void process_Log(int32_t seqid, apache::thrift::protocol::TProtocol* iprot, apache::thrift::protocol::TProtocol* oprot, void* callContext);
 public:
  ScribeProcessor(boost::shared_ptr<ScribeIf> iface) :
    iface_(iface) {
    processMap_["Log"] = &ScribeProcessor::process_Log;
  }

  virtual bool process(boost::shared_ptr<apache::thrift::protocol::TProtocol> piprot, boost::shared_ptr<apache::thrift::protocol::TProtocol> poprot, void* callContext);
  virtual ~ScribeProcessor() {}
};

class ScribeProcessorFactory : public ::apache::thrift::TProcessorFactory {
 public:
  ScribeProcessorFactory(const ::boost::shared_ptr< ScribeIfFactory >& handlerFactory) :
      handlerFactory_(handlerFactory) {}

  ::boost::shared_ptr< ::apache::thrift::TProcessor > getProcessor(const ::apache::thrift::TConnectionInfo& connInfo);

 protected:
  ::boost::shared_ptr< ScribeIfFactory > handlerFactory_;
};

class ScribeMultiface : virtual public ScribeIf {
 public:
  ScribeMultiface(std::vector<boost::shared_ptr<ScribeIf> >& ifaces) : ifaces_(ifaces) {
  }
  virtual ~ScribeMultiface() {}
 protected:
  std::vector<boost::shared_ptr<ScribeIf> > ifaces_;
  ScribeMultiface() {}
  void add(boost::shared_ptr<ScribeIf> iface) {
    ifaces_.push_back(iface);
  }
 public:
  ResultCode Log(const std::vector<LogEntry> & messages) {
    size_t sz = ifaces_.size();
    for (size_t i = 0; i < sz; ++i) {
      if (i == sz - 1) {
        return ifaces_[i]->Log(messages);
      } else {
        ifaces_[i]->Log(messages);
      }
    }
  }

};

} // namespace

#endif