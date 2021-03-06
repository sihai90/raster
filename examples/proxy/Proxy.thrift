// Copyright (C) 2017, Yeolar

namespace cpp rdd.proxy

struct Query {
    1: required string traceid;
    2: optional string query;
    3: optional string forward;
}

enum ResultCode {
    OK, // 0

    // WARNING      <1000

    // ERROR        >1000
    E_SOURCE__UNTRUSTED = 1001,
    E_BACKEND_FAILURE = 1002,
}

struct Result {
    1: required string traceid;
    2: optional ResultCode code;
}

service Proxy {
    Result run(1: Query query);
}
