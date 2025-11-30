#include "BrokerAsyncGrpcFacade.h"

#include "sdk/Logger.h"
#include "sdk/grpc/GrpcCall.h"
#include "sdk/vdb/grpc/duo/TypeConverter.h"

#include <grpcpp/channel.h>
#include <utility>

using google::protobuf::ListValue;
using google::protobuf::Struct;
using google::protobuf::Value;

namespace velocitas::duo {

BrokerAsyncGrpcFacade::BrokerAsyncGrpcFacade(const std::shared_ptr<grpc::Channel>& channel)
    : m_stub{::duo::ShadowService::NewStub(channel)}
    , m_job_stub{::duo::JobService::NewStub(channel)} {}

/// 不要になった関数
::duo::GetResponse ConvertGetResponseToDatapointsReply(const ::duo::GetResponse& response) {
    return response;
}

void BrokerAsyncGrpcFacade::GetDatapoints(
    const std::vector<std::string>&                      datapoints,
    std::function<void(const ::duo::GetResponse& reply)> replyHandler,
    std::function<void(const grpc::Status& status)>      errorHandler) {
    auto callData =
        std::make_shared<GrpcSingleResponseCall<::duo::GetReportRequest, ::duo::GetResponse>>();

    //----------------------------------------------------------------------
    // get対象データパス群を callData に詰め込む
    //----------------------------------------------------------------------
    // std::for_each(datapoints.begin(), datapoints.end(), [&callData](const auto& dataPoint) {
    //     callData->m_request.add_datapoints(dataPoint);
    // });
    //----------------------------------------------------------------------
    // 現時点のduo::GetReportRequest()は
    // 複数データの一括リクエストに対応していないので、
    // 引数で受けたパス群の最初のパスのみget()する実装にする。
    //----------------------------------------------------------------------
    if (datapoints.empty()) {
        logger().error("GetDatapoints called without any datapoint paths.");
        return;
    }

    std::string path = DuoTypeConverter::toDuoPath(datapoints.front());
    logger().info(path);
    callData->m_request.set_thing(
        "vss"); // duoではthingが選べるが、今のVelocitasはthingを選択する構造でないので、仮にvssに固定する。
    callData->m_request.set_path(path);

    //----------------------------------------------------------------------
    // grpc::ClientContext のcontext AddMetadata() を実行してもらう
    // (gRPCのメタデータのリストにget対象を加える
    //  gRPCにおける「メタデータ（metadata）」とは、
    //  RPC呼び出しに付随して送受信される追加情報（ヘッダー）のことを指す。)
    //----------------------------------------------------------------------
    applyContextModifier(*callData);

    //----------------------------------------------------------------------
    // 通信結果を処理するコールバック関数を定義する
    //----------------------------------------------------------------------
    const auto grpcResultHandler = [callData, replyHandler, errorHandler](grpc::Status status) {
        try {
            if (status.ok()) {
                replyHandler(callData->m_response);
            } else {
                errorHandler(status);
            };
        } catch (std::exception& e) {
            logger().error("GRPC: Exception occurred during \"GetDatapoints\": {}", e.what());
        }
        callData->m_isComplete = true;
    };

    //----------------------------------------------------------------------
    // 通信したいデータのリストにcallDataを登録する
    //----------------------------------------------------------------------
    addActiveCall(callData);

    //----------------------------------------------------------------------
    // 登録済みの通信を実施し、引数のコールバック関数で処理する。
    //----------------------------------------------------------------------
    m_stub->async()->GetReport(&callData->m_context, &callData->m_request, &callData->m_response,
                               grpcResultHandler);
}

void BrokerAsyncGrpcFacade::SetDatapoints(
    const std::map<std::string, Value>&                        datapoints_map,
    std::function<void(const ::duo::CreateJobResponse& reply)> replyHandler,
    std::function<void(const grpc::Status& status)>            errorHandler) {

    auto callData = std::make_shared<
        GrpcSingleResponseCall<::duo::CreateJobRequest, ::duo::CreateJobResponse>>();

    // 先頭のデータを取得し、duo用に変換 (複数対応は残件)
    if (datapoints_map.empty()) {
        logger().warn("SetDatapoints called without payload.");
        return;
    }

    const auto& firstEntry = *datapoints_map.begin();
    // Job documentのtargetフィールドはドット区切り形式が必要なのでそのまま使用
    const std::string& path = firstEntry.first;

    logger().info(path);

    callData->m_request.set_thing("vss");
    Struct document;
    auto&  fields = *document.mutable_fields();
    fields["action"].set_string_value("set");
    fields["target"].set_string_value(path);
    fields["value"].CopyFrom(firstEntry.second);

    callData->m_request.mutable_document()->CopyFrom(document);

    applyContextModifier(*callData);

    // 応答を処理するハンドラー
    const auto grpcResultHandler = [callData, replyHandler, errorHandler](grpc::Status status) {
        try {
            if (status.ok()) {
                replyHandler(callData->m_response);
            } else {
                errorHandler(status);
            };
        } catch (std::exception& e) {
            logger().error("GRPC: Exception occurred during \"SetDatapoints\": {}", e.what());
        }
        callData->m_isComplete = true;
    };

    addActiveCall(callData);

    m_job_stub->async()->CreateJob(&callData->m_context, &callData->m_request,
                                   &callData->m_response, grpcResultHandler);
}

void BrokerAsyncGrpcFacade::Subscribe(
    const std::vector<std::string>&                         targets,
    std::function<void(const ::duo::ListenResponse& reply)> streamHandler,
    std::function<void(const grpc::Status& status)>         errorHandler) {

    auto callData = std::make_shared<
        GrpcStreamingResponseCall<::duo::ListenReportRequest, ::duo::ListenResponse>>();

    auto& request = callData->getRequest();
    request.set_thing("vss");
    request.set_needs_initial_value(true);
    if (!targets.empty()) {
        request.add_filters(
            DuoTypeConverter::toDuoPath(targets.front())); //仮で先頭のみの対応とする
    }

    applyContextModifier(*callData);
    addActiveCall(callData);

    callData->onData([streamHandler = std::move(streamHandler)](const auto& response) {
        streamHandler(response);
    });

    callData->onFinish([callData, errorHandler = std::move(errorHandler)](const auto& status) {
        if (!status.ok()) {
            errorHandler(status);
        }
        callData->m_isComplete = true;
    });

    m_stub->async()->ListenReport(&callData->m_context, &request, &callData->getReactor());

    callData->startCall();
}

} // namespace velocitas::duo
