#include "sdk/vdb/IVehicleDataBrokerClient.h"

#include <memory>
#include <string>
#include <vector>

#ifndef DUO_BROKERCLIENT_H
#define DUO_BROKERCLIENT_H

namespace velocitas::duo {

class BrokerAsyncGrpcFacade;

class BrokerClient : public IVehicleDataBrokerClient {
public:
    explicit BrokerClient(const std::string& vdbAddress, const std::string& vdbServiceName);
    explicit BrokerClient(const std::string& vdbserviceName);

    ~BrokerClient() override;

    BrokerClient(const BrokerClient&)            = delete;
    BrokerClient(BrokerClient&&)                 = delete;
    BrokerClient& operator=(const BrokerClient&) = delete;
    BrokerClient& operator=(BrokerClient&&)      = delete;

    AsyncResultPtr_t<DataPointReply>
    getDatapoints(const std::vector<std::string>& datapoints) override;

    AsyncResultPtr_t<SetErrorMap_t>
    setDatapoints(const std::vector<std::unique_ptr<DataPointValue>>& datapoints) override;

    AsyncSubscriptionPtr_t<DataPointReply> subscribe(const std::string& query) override;

private:
    std::shared_ptr<BrokerAsyncGrpcFacade> m_asyncBrokerFacade;
};

} // namespace velocitas::duo

#endif // DUO_BROKERCLIENT_H
