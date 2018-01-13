/*
 * Copyright 2017 Yeolar
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "raster/ssl/OpenSSLHash.h"
#include "raster/io/IOBuf.h"
#include <gtest/gtest.h>

using namespace rdd;

TEST(OpenSSLHashTest, sha256) {
  IOBuf buf;
  buf.prependChain(IOBuf::wrapBuffer(ByteRange(StringPiece("foo"))));
  buf.prependChain(IOBuf::wrapBuffer(ByteRange(StringPiece("bar"))));
  EXPECT_EQ(3, buf.countChainElements());
  EXPECT_EQ(6, buf.computeChainDataLength());

  auto expected = std::vector<uint8_t>(32);
  auto combined = ByteRange(StringPiece("foobar"));
  SHA256(combined.data(), combined.size(), expected.data());

  auto out = std::vector<uint8_t>(32);
  OpenSSLHash::sha256(range(out), buf);
  EXPECT_EQ(expected, out);
}

TEST(OpenSSLHashTest, sha256_hashcopy) {
  std::array<uint8_t, 32> expected, actual;

  OpenSSLHash::Digest digest;
  digest.hash_init(EVP_sha256());
  digest.hash_update(ByteRange(StringPiece("foobar")));

  OpenSSLHash::Digest copy(digest);

  digest.hash_final(range(expected));
  copy.hash_final(range(actual));

  EXPECT_EQ(expected, actual);
}

TEST(OpenSSLHashTest, sha256_hashcopy_intermediate) {
  std::array<uint8_t, 32> expected, actual;

  OpenSSLHash::Digest digest;
  digest.hash_init(EVP_sha256());
  digest.hash_update(ByteRange(StringPiece("foo")));

  OpenSSLHash::Digest copy(digest);

  digest.hash_update(ByteRange(StringPiece("bar")));
  copy.hash_update(ByteRange(StringPiece("bar")));

  digest.hash_final(range(expected));
  copy.hash_final(range(actual));

  EXPECT_EQ(expected, actual);
}

TEST(OpenSSLHashTest, hmac_sha256) {
  auto key = ByteRange(StringPiece("qwerty"));

  IOBuf buf;
  buf.prependChain(IOBuf::wrapBuffer(ByteRange(StringPiece("foo"))));
  buf.prependChain(IOBuf::wrapBuffer(ByteRange(StringPiece("bar"))));
  EXPECT_EQ(3, buf.countChainElements());
  EXPECT_EQ(6, buf.computeChainDataLength());

  auto expected = std::vector<uint8_t>(32);
  auto combined = ByteRange(StringPiece("foobar"));
  HMAC(
      EVP_sha256(),
      key.data(),
      int(key.size()),
      combined.data(),
      combined.size(),
      expected.data(),
      nullptr);

  auto out = std::vector<uint8_t>(32);
  OpenSSLHash::hmac_sha256(range(out), key, buf);
  EXPECT_EQ(expected, out);
}
