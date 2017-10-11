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

#include "main/service.hpp"

#include <boost/assert.hpp>

using namespace iroha;
using namespace iroha::ametsuchi;
using namespace iroha::simulator;
using namespace iroha::validation;
using namespace iroha::network;
using namespace iroha::model;
using namespace iroha::synchronizer;
using namespace iroha::torii;
using namespace iroha::model::converters;
using namespace iroha::consensus::yac;

Application::Application(std::unique_ptr<Config> config)
    : config_(std::move(config)), log_(logger::log("irohad")) {
  log_->info("created");
  initStorage();
}

Application::~Application() {
  if (internal_server) {
    internal_server->Shutdown();
  }
  if (torii_server) {
    torii_server->shutdown();
  }
  if (server_thread.joinable()) {
    server_thread.join();
  }
}

void Application::init() {
  initProtoFactories();
  initPeerQuery();
  initPeer();
  initCryptoProvider();
  initValidators();
  initOrderingGate();
  initSimulator();
  initBlockLoader();
  initConsensusGate();
  initSynchronizer();
  initPeerCommunicationService();

  // Torii
  initTransactionCommandService();
  initQueryService();
}

void Application::initStorage() {
  storage = StorageImpl::create(
      config_->redis(), config_->postgres(), config_->blockStorage());

  BOOST_ASSERT(storage != nullptr);

  log_->info("[Init] => storage", logger::logBool(storage));
}

void Application::initProtoFactories() {
  pb_tx_factory = std::make_shared<PbTransactionFactory>();
  pb_query_factory = std::make_shared<PbQueryFactory>();
  pb_query_response_factory = std::make_shared<PbQueryResponseFactory>();

  BOOST_ASSERT(pb_tx_factory != nullptr);

  log_->info("[Init] => converters");
}

void Application::initPeer() {
  auto peers = wsv->getLedgerPeers().value();

  model::Peer p;
  p.pubkey = config_->cryptography().keypair().pubkey;
  p.address = config_->torii().listenAddress();

  log_->info("[Init] => peer address is {}", peer.address);
}

void Application::initCryptoProvider() {
  crypto_verifier = std::make_shared<ModelCryptoProviderImpl>(
      config_->cryptography().keypair());

  log_->info("[Init] => crypto provider");
}

void Application::initValidators() {
  stateless_validator =
      std::make_shared<StatelessValidatorImpl>(crypto_verifier);
  stateful_validator = std::make_shared<StatefulValidatorImpl>();
  chain_validator = std::make_shared<ChainValidatorImpl>();

  log_->info("[Init] => validators");
}

void Application::initPeerQuery() {
  wsv = std::make_shared<ametsuchi::PeerQueryWsv>(storage->getWsvQuery());
  BOOST_ASSERT(wsv != nullptr);

  log_->info("[Init] => peer query");
}

void Application::initOrderingGate() {
  // maximum transactions in proposal
  const auto max_transactions_in_proposal = 10u;

  // delay before emitting new proposal
  const auto delay_for_new_proposal = 5000u;

  ordering_gate = ordering_init.initOrderingGate(
      wsv, max_transactions_in_proposal, delay_for_new_proposal);

  log_->info("[Init] => init ordering gate - [{}]",
             logger::logBool(ordering_gate));
}

void Application::initSimulator() {
  simulator = std::make_shared<Simulator>(ordering_gate,
                                          stateful_validator,
                                          storage,
                                          storage->getBlockQuery(),
                                          crypto_verifier);

  log_->info("[Init] => init simulator");
}

void Application::initBlockLoader() {
  block_loader = loader_init.initBlockLoader(
      wsv, storage->getBlockQuery(), crypto_verifier);

  log_->info("[Init] => block loader");
}

void Application::initConsensusGate() {
  consensus_gate = yac_init.initConsensusGate(
      peer.address,
      wsv,
      simulator,
      block_loader,
      /*TODO(@warchant): temp solution. Will be changed with Keypair object.*/
      config_->cryptography().keypair());

  log_->info("[Init] => consensus gate");
}

void Application::initSynchronizer() {
  synchronizer = std::make_shared<SynchronizerImpl>(
      consensus_gate, chain_validator, storage, block_loader);

  log_->info("[Init] => synchronizer");
}

void Application::initPeerCommunicationService() {
  pcs = std::make_shared<PeerCommunicationServiceImpl>(ordering_gate,
                                                       synchronizer);

  pcs->on_proposal().subscribe(
      [this](auto) { log_->info("~~~~~~~~~| PROPOSAL ^_^ |~~~~~~~~~ "); });

  pcs->on_commit().subscribe(
      [this](auto) { log_->info("~~~~~~~~~| COMMIT =^._.^= |~~~~~~~~~ "); });

  log_->info("[Init] => pcs");
}

void Application::initTransactionCommandService() {
  auto tx_processor =
      std::make_shared<TransactionProcessorImpl>(pcs, stateless_validator);

  command_service =
      std::make_unique<::torii::CommandService>(pb_tx_factory, tx_processor);

  log_->info("[Init] => command service");
}

void Application::initQueryService() {
  auto query_proccessing_factory = std::make_unique<QueryProcessingFactory>(
      storage->getWsvQuery(), storage->getBlockQuery());

  auto query_processor = std::make_shared<QueryProcessorImpl>(
      std::move(query_proccessing_factory), stateless_validator);

  query_service = std::make_unique<::torii::QueryService>(
      pb_query_factory, pb_query_response_factory, query_processor);

  log_->info("[Init] => query service");
}

void Application::run() {
  torii_server =
      std::make_unique<ServerRunner>(config_->torii().listenAddress());

  grpc::ServerBuilder builder;
  int *port = 0;
  builder.AddListeningPort(config_->torii().listenAddress(),
                           grpc::InsecureServerCredentials(),
                           port);
  BOOST_ASSERT(port != nullptr);

  builder.RegisterService(ordering_init.ordering_gate_transport.get());
  builder.RegisterService(ordering_init.ordering_service_transport.get());
  builder.RegisterService(yac_init.consensus_network.get());
  builder.RegisterService(loader_init.service.get());

  internal_server = builder.BuildAndStart();
  server_thread = std::thread([this] {
    torii_server->run(std::move(command_service), std::move(query_service));
  });

  log_->info("===> iroha initialized. torii is on port {}", *port);

  torii_server->waitForServersReady();
  internal_server->Wait();
}

const Config &Application::config() const { return *config_; }
