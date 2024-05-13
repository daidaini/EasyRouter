#pragma once

/*对恒生模块中的连接类做适配修改
 */

#include "GatewayDefine.h"
#include "StringFunc.h"
#include "ConnPool.h"

// t2sdk
#include "t2sdk_interface.h"

namespace HST2
{
    constexpr const char s_T2ConfigPath[] = "./t2sdk.ini";
    constexpr const int s_ConnectTimeout = 20000; // ms
    constexpr const int s_QueryRecordNum = 200;
    constexpr const char s_PositionStr[] = "position_str";

    //<FieldName, FieldValue>
    using HsRequestParam = std::pair<std::string, std::string>;

    struct RequstInfo
    {
        int FunID = 0;
        std::string FuncName;
        int SysNode = 0;

        void Assign(int funcid, const std::string &funcname, int sysnode)
        {
            FunID = funcid;
            FuncName = funcname;
            SysNode = sysnode;
        }
    };

    CConfigInterface *GenerateConfiger();

    class Connection final
    {
    public:
        Connection();
        ~Connection();

        // 创建连接
        bool Create();
        // 检测连接是否有效
        bool GetSocketValid() const;

        // 发送请求并接收数据
        GateErrorStruct MID_SendToData();

        // 移动到下一条记录（遍历第一次不后移）
        // 返回false 表示没有下一条
        bool MID_MoveNextRec();

        /*添加参数*/
        void PrepareRequest(int hsFunID, const char *hsFuncName, int hsSysnodeId);
        void MID_AddString(std::string &&field, const std::string &value);
        void MID_SetString(std::string &&field, const std::string &value);
        void MID_AddChar(std::string &&field, char value);
        void MID_AddInt(std::string &&field, int value);

        /*解析记录*/
        int MID_GetInt(const char *field) const;
        char MID_GetChar(const char *field) const;
        std::string MID_GetString(const char *field) const;
        double MID_GetDouble(const char *field) const;

        int CurrUnpackerRecCnt() const
        {
            return m_CurrUnpackerRecCnt;
        }

    private:
        //<ip, port>
        std::pair<std::string, int16_t> GetAddrFromConfig();

        void NewIF2Packer();

        GateErrorStruct ParseRspPkg(int code);

        std::string GetPackerErrorInfo() const;

        // 打印一条应答记录
        void LogOneRspRecord() const;
        // 打印请求日志
        void LogRequest() const;

        bool IsNeedNextPack() const;

    private:
        // 配置对象，各个连接对象共享，不用管理释放
        CConfigInterface *m_HsConfiger = nullptr;
        // 连接对象
        CConnectionInterface *m_HsConnectionPtr = nullptr;
        // 打包器
        IF2Packer *m_IF2Packer = nullptr;
        // 打包器用
        char m_HsPackerMemory[10240]{};
        // 解包器
        IF2UnPacker *m_IF2UnPacker = nullptr;

        bool m_IsConnected = false;

        // 请求包体参数
        std::map<std::string, std::string> m_RequestParams;
        RequstInfo m_CurrReqInfo;

        // 当前解包器包含的记录的索引，用以判断解包器中记录是否解析完毕
        int m_CurrUnpackerRecIndex = 0;
        // 当前解包器包含的记录总数
        int m_CurrUnpackerRecCnt = 0;
    };
}
