#ifndef DUO_BROKERASYNCGRPCFACADE_H
#define DUO_BROKERASYNCGRPCFACADE_H

#include "sdk/grpc/AsyncGrpcFacade.h"
#include "sdk/grpc/GrpcClient.h"

#include "duo/duo.grpc.pb.h"

#include <google/protobuf/struct.pb.h>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace grpc {
class Channel;
class Status;
} // namespace grpc

namespace velocitas::duo {

class BrokerAsyncGrpcFacade : public AsyncGrpcFacade, GrpcClient {
public:
    explicit BrokerAsyncGrpcFacade(const std::shared_ptr<grpc::Channel>& channel);

    void GetDatapoints(const std::vector<std::string>&                      datapoints,
                       std::function<void(const ::duo::GetResponse& reply)> replyHandler,
                       std::function<void(const grpc::Status& status)>      errorHandler);

    void SetDatapoints(const std::map<std::string, google::protobuf::Value>&      datapoints,
                       std::function<void(const ::duo::CreateJobResponse& reply)> replyHandler,
                       std::function<void(const grpc::Status& status)>            errorHandler);

    void Subscribe(const std::vector<std::string>&                         targets,
                   std::function<void(const ::duo::ListenResponse& reply)> streamHandler,
                   std::function<void(const grpc::Status& status)>         errorHandler);

private:
    std::unique_ptr<::duo::ShadowService::StubInterface> m_stub;
    std::unique_ptr<::duo::JobService::StubInterface>    m_job_stub;
};

} // namespace velocitas::duo

#endif // DUO_BROKERASYNCGRPCFACADE_H
