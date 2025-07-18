#pragma once
#include <cstdint>
#include <string>
#include <memory>
#include "google/protobuf/message.h"

enum ServerMsgType : int8_t {
    None = 0,
    C2S_FLAG = 1,  // �ͻ��˷���������
    S2C_FLAG = 2,  // �����������ͻ���
    WEB_REQ_FLAG = 3,
    WEB_REP_FLAG = 4,
    BROADCAST_FLAG = 5,  // �㲥
    NOTIFY_FLAG = 6,  // ֪ͨĳ������
    S2S_REQ_FLAG = 7,  // ����ĳ������
    S2S_REP_FLAG = 8,  // �ظ�ĳ������
    NOTIFY_GROUP_FLAG = 9,  // ֪ͨĳ�����
    GROUPCAST_FLAG = 10, // ����stype�鲥
    GROUPCAST2_FLAG = 11, // ����sid�鲥
    NOTIFY_TOTAL_FLAG = 12, // ֪ͨ���з���
    PUB_FLAG = 13, // ����ĳ����Ϣ�ķ��񷢲�
    RETRANS_FLAG = 14, // ת��
    USR_REQ_FLAG = 15, // ����ĳ�����
    USR_REP_FLAG = 16, // �ظ�ĳ�����	
};

struct NetPack
{
    int32_t len = 0;
    int32_t seq = 0;
    int32_t msg_id = 0;
    int64_t conn_id = 0;
    int64_t uid = 0;
    int8_t  flag = 0;

    std::string msg;
    NetPack() = default;
    NetPack(const NetPack& request, const google::protobuf::Message* msg, int8_t msg_id = None);
    NetPack(const google::protobuf::Message* msg, int8_t msg_id = None);

    std::shared_ptr<std::string> serialize() const;
    static std::shared_ptr<NetPack> deserialize(int64_t conn_id, const std::string& data);
};
