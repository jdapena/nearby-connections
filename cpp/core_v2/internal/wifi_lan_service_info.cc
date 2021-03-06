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

#include "core_v2/internal/wifi_lan_service_info.h"

#include <inttypes.h>

#include <cstring>
#include <utility>

#include "platform_v2/base/base64_utils.h"
#include "platform_v2/base/base_input_stream.h"
#include "platform_v2/public/logging.h"
#include "absl/strings/str_cat.h"

namespace location {
namespace nearby {
namespace connections {

WifiLanServiceInfo::WifiLanServiceInfo(Version version, Pcp pcp,
                                       absl::string_view endpoint_id,
                                       const ByteArray& service_id_hash,
                                       absl::string_view endpoint_name) {
  if (version != Version::kV1 || endpoint_id.empty() ||
      endpoint_id.length() != kEndpointIdLength ||
      service_id_hash.size() != kServiceIdHashLength) {
    return;
  }
  switch (pcp) {
    case Pcp::kP2pCluster:  // Fall through
    case Pcp::kP2pStar:     // Fall through
    case Pcp::kP2pPointToPoint:
      break;
    default:
      return;
  }

  version_ = version;
  pcp_ = pcp;
  service_id_hash_ = service_id_hash;
  endpoint_id_ = std::string(endpoint_id);
  endpoint_name_ = std::string(endpoint_name);
}

WifiLanServiceInfo::WifiLanServiceInfo(absl::string_view service_info_string) {
  ByteArray service_info_bytes = Base64Utils::Decode(service_info_string);

  if (service_info_bytes.Empty()) {
    NEARBY_LOG(
        INFO,
        "Cannot deserialize WifiLanServiceInfo: failed Base64 decoding of %s",
        std::string(service_info_string).c_str());
    return;
  }

  if (service_info_bytes.size() > kMaxLanServiceNameLength) {
    NEARBY_LOG(INFO,
               "Cannot deserialize WifiLanServiceInfo: expecting max %d raw "
               "bytes, got %" PRIu64,
               kMaxLanServiceNameLength, service_info_bytes.size());
    return;
  }

  if (service_info_bytes.size() < kMinLanServiceNameLength) {
    NEARBY_LOG(INFO,
               "Cannot deserialize WifiLanServiceInfo: expecting min %d raw "
               "bytes, got %" PRIu64,
               kMinLanServiceNameLength, service_info_bytes.size());
    return;
  }

  if (service_info_bytes.size() > kMaxEndpointNameLength) {
    NEARBY_LOG(INFO,
               "Cannot deserialize WifiLanServiceInfo: expecting max %d raw "
               "bytes, got %" PRIu64,
               kMaxEndpointNameLength, service_info_bytes.size());
    return;
  }

  BaseInputStream base_input_stream{service_info_bytes};
  // The first 1 byte is supposed to be the version and pcp.
  auto version_and_pcp_byte = static_cast<char>(base_input_stream.ReadUint8());
  // The upper 3 bits are supposed to be the version.
  version_ =
      static_cast<Version>((version_and_pcp_byte & kVersionBitmask) >> 5);
  if (version_ != Version::kV1) {
    NEARBY_LOG(INFO,
               "Cannot deserialize WifiLanServiceInfo: unsupported Version %d",
               version_);
    return;
  }
  // The lower 5 bits are supposed to be the Pcp.
  pcp_ = static_cast<Pcp>(version_and_pcp_byte & kPcpBitmask);
  switch (pcp_) {
    case Pcp::kP2pCluster:  // Fall through
    case Pcp::kP2pStar:     // Fall through
    case Pcp::kP2pPointToPoint:
      break;
    default:
      NEARBY_LOG(INFO,
                 "Cannot deserialize WifiLanServiceInfo: unsupported V1 PCP %d",
                 pcp_);
  }

  // The next 4 bytes are supposed to be the endpoint_id.
  endpoint_id_ = std::string{base_input_stream.ReadBytes(kEndpointIdLength)};

  // The next 3 bytes are supposed to be the service_id_hash.
  service_id_hash_ = base_input_stream.ReadBytes(kServiceIdHashLength);

  // The next 1 byte are supposed to be the length of the endpoint_name.
  std::uint32_t expected_endpoint_name_length = base_input_stream.ReadUint8();

  // The rest bytes are supposed to be the endpoint_name
  auto endpoint_name_bytes =
      base_input_stream.ReadBytes(expected_endpoint_name_length);
  if (endpoint_name_bytes.Empty() ||
      endpoint_name_bytes.size() != expected_endpoint_name_length) {
    NEARBY_LOG(INFO,
               "Cannot deserialize WifiLanServiceInfo: expected "
               "endpointName to be %d bytes, got %" PRIu64,
               expected_endpoint_name_length, endpoint_name_bytes.size());

    // Clear enpoint_id for validadity.
    endpoint_id_.clear();
    return;
  }
  endpoint_name_ = std::string{endpoint_name_bytes};
}

WifiLanServiceInfo::operator std::string() const {
  if (!IsValid()) {
    return "";
  }

  // The upper 3 bits are the Version.
  auto version_and_pcp_byte = static_cast<char>(
      (static_cast<uint32_t>(Version::kV1) << 5) & kVersionBitmask);
  // The lower 5 bits are the PCP.
  version_and_pcp_byte |=
      static_cast<char>(static_cast<uint32_t>(pcp_) & kPcpBitmask);

  std::string usable_endpoint_name(endpoint_name_);
  if (endpoint_name_.size() > kMaxEndpointNameLength) {
    NEARBY_LOG(
        INFO,
        "While serializing WifiLanServiceInfo, truncating Endpoint Name %s "
        "(%lu bytes) down to %d bytes",
        endpoint_name_.c_str(), endpoint_name_.size(), kMaxEndpointNameLength);
    usable_endpoint_name.erase(kMaxEndpointNameLength);
  }

  // clang-format off
  std::string out = absl::StrCat(std::string(1, version_and_pcp_byte),
                                 endpoint_id_,
                                 std::string(service_id_hash_),
                                 std::string(1, usable_endpoint_name.size()),
                                 usable_endpoint_name);
  // clang-format on

  return Base64Utils::Encode(ByteArray{std::move(out)});
}

}  // namespace connections
}  // namespace nearby
}  // namespace location
