/**
 * Copyright Soramitsu Co., Ltd. 2017 All Rights Reserved.
 * http://soramitsu.co.jp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IROHA_CONFIG_MOCK_HPP_
#define IROHA_CONFIG_MOCK_HPP_

#include <gmock/gmock.h>
#include "main/config/config.hpp"
#include "util/string.hpp"
#include "crypto/crypto.hpp"

using namespace std::literals::string_literals;
using iroha::string::util::from_string;

template <typename T>
T parseEnv(const char *name, T default_) {
  auto v = std::getenv(name);
  return v && strlen(v) > 0 ? from_string<T>(v) : default_;
}

class FakeConfig : public iroha::config::Config {
 public:
  FakeConfig() { load(); }

  void load() override {
    this->pg_.host = parseEnv("IROHA_POSTGRES_HOST", "172.17.0.2"s);
    this->pg_.port = parseEnv("IROHA_POSTGRES_PORT", 5432);
    this->pg_.username = parseEnv("IROHA_POSTGRES_USER", "iroha"s);
    this->pg_.password = parseEnv("IROHA_POSTGRES_PASSWORD", "helloworld"s);

    this->redis_.host = parseEnv("IROHA_REDIS_HOST", "172.17.0.3"s);
    this->redis_.port = parseEnv("IROHA_REDIS_PORT", 6379);

    this->db_.path = parseEnv("IROHA_DATABASE_PATH", "/tmp/block_store"s);

    this->torii_.host = parseEnv("IROHA_TORII_HOST", "0.0.0.0"s);
    this->torii_.port = parseEnv("IROHA_TORII_PORT", 50051);

    iroha::keypair_t kp = iroha::create_keypair(iroha::create_seed("host"));
    this->crypto_.key = kp.privkey.to_string();
    this->crypto_.certificate = kp.pubkey.to_string();

//    this->options_.genesis_block = ?;

    this->loaded_ = true;
  }
};

#endif  //  IROHA_CONFIG_MOCK_HPP_
