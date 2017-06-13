/*
Copyright 2017 Soramitsu Co., Ltd.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "command_service.hpp"
#include <validation/stateless/validator.hpp>
#include <ordering/quque.hpp>

namespace connection {
    namespace api {

        using namespace iroha::protocol;

        grpc::Status CommandService::Torii(grpc::ClientContext* context,
                                           const Transaction& request,
                                           ToriiResponse* response) {

            // TODO: Use this to get client's ip and port.
            (void) context;

            if(validator::stateless::validate(request)){
                ordering::queue::append(request);
                // TODO: Return tracking log number (hash)
                *response = ToriiResponse();
            }else{
                // TODO: Return validation failed message
                *response = ToriiResponse();
            }
            return grpc::Status::OK;
        }

    }  // namespace api
}  // namespace connection
