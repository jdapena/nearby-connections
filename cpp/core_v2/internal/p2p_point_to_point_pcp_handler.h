// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CORE_V2_INTERNAL_P2P_POINT_TO_POINT_PCP_HANDLER_H_
#define CORE_V2_INTERNAL_P2P_POINT_TO_POINT_PCP_HANDLER_H_

#include "core_v2/internal/endpoint_channel_manager.h"
#include "core_v2/internal/endpoint_manager.h"
#include "core_v2/internal/mediums/mediums.h"
#include "core_v2/internal/p2p_star_pcp_handler.h"
#include "core_v2/internal/pcp.h"
#include "core_v2/strategy.h"

namespace location {
namespace nearby {
namespace connections {

// Concrete implementation of the PCPHandler for the P2P_POINT_TO_POINT. This
// PCP is for mediums that have limitations on the number of simultaneous
// connections; all mediums in P2P_STAR are valid for P2P_POINT_TO_POINT, but
// not all mediums in P2P_POINT_TO_POINT and valid for P2P_STAR.
//
// Currently, this implementation advertises/discovers over Bluetooth
// and connects over Bluetooth.
class P2pPointToPointPcpHandler : public P2pStarPcpHandler {
 public:
  P2pPointToPointPcpHandler(Mediums& mediums, EndpointManager& endpoint_manager,
                            EndpointChannelManager& channel_manager,
                            Pcp pcp = Pcp::kP2pPointToPoint);

 protected:
  std::vector<proto::connections::Medium> GetConnectionMediumsByPriority()
      override;

  bool CanSendOutgoingConnection(ClientProxy* client) const override;
  bool CanReceiveIncomingConnection(ClientProxy* client) const override;

 private:
  Mediums* mediums_;
};

}  // namespace connections
}  // namespace nearby
}  // namespace location

#endif  // CORE_V2_INTERNAL_P2P_POINT_TO_POINT_PCP_HANDLER_H_
