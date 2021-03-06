/*
 * Copyright (C) 2017, Yeolar
 */

#include <gflags/gflags.h>

#include "raster/concurrency/CPUThreadPoolExecutor.h"
#include "raster/net/NetUtil.h"
#include "raster/protocol/thrift/SyncClient.h"
#include "raster/util/Algorithm.h"
#include "raster/util/Logging.h"
#include "raster/util/Portability.h"
#include "gen-cpp/Parallel.h"

static const char* VERSION = "1.1.0";

DEFINE_string(addr, "127.0.0.1:8000", "HOST:PORT");
DEFINE_int32(threads, 8, "concurrent threads");
DEFINE_int32(count, 100, "request count");

using namespace rdd;
using namespace rdd::parallel;

bool request(const ClientOption& opt) {
  Query req;
  req.__set_traceid("rddt");
  req.__set_query("query");
  Result res;

  TSyncClient<ParallelClient> client(opt);
  if (!client.connect()) {
    return false;
  }
  try {
    client.fetch(&ParallelClient::run, res, req);
    if (res.code != 0) {
      return false;
    }
  }
  catch (...) {
    return false;
  }
  return true;
}

int main(int argc, char* argv[]) {
  gflags::SetVersionString(VERSION);
  gflags::SetUsageMessage("Usage : ./parallel-bench");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  CPUThreadPoolExecutor pool(FLAGS_threads);
  std::atomic<size_t> count(0);
  std::vector<uint64_t> costs(FLAGS_count);

  pool.subscribeToTaskStats(
      [&](ThreadPoolExecutor::TaskStats stats) {
        costs[count++] = stats.runTime;
      });

  ClientOption opt;
  opt.peer.setFromIpPort(FLAGS_addr);
  opt.timeout.ctimeout = 10000000;
  opt.timeout.rtimeout = 10000000;
  opt.timeout.wtimeout = 10000000;

  for (int i = 0; i < FLAGS_count; i++) {
    pool.add(std::bind(request, opt));
  }

  while (pool.getPoolStats().pendingTaskCount > 0) {
    sleep(1);
    ACCRLOG(INFO) << "handled: " << count;
  }
  pool.join();

  ACCRLOG(INFO) << "FINISH";
  ACCRLOG(INFO) << "total: " << count;

  if (count > 0) {
    std::sort(costs.begin(), costs.end());
    uint64_t cost10 = costs[count    /10];
    uint64_t cost50 = costs[count * 5/10];
    uint64_t cost90 = costs[count * 9/10];
    uint64_t cost_sum = sum(costs);
    uint64_t cost_avg = cost_sum / count;

    ACCRLOG(INFO) << " cost10: " << cost10   / 1000.0 << " ms";
    ACCRLOG(INFO) << " cost50: " << cost50   / 1000.0 << " ms";
    ACCRLOG(INFO) << " cost90: " << cost90   / 1000.0 << " ms";
    ACCRLOG(INFO) << "avgcost: " << cost_avg / 1000.0 << " ms";
    ACCRLOG(INFO) << "    qps: " << 1000000. / cost_avg * FLAGS_threads;
  }

  gflags::ShutDownCommandLineFlags();
  return 0;
}
