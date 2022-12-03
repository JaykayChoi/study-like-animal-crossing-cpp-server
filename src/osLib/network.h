#pragma once

#include "../global.h"

class ITCPConnection;
class IListenServer;

// Single TCP connection interface.
class ITCPConnection
{
    friend class Network;

public:
    class Callback
    {
    public:
        virtual ~Callback() { }

        /**
         * ITCPConnection 가 생성될 때 호출 됨.
         * 콜백은 나중에 사용하기 위해 ITCPConnection instance 을 저장할 수 있지만
           OnError(), OnRemoteClosed() 또는 Close() 바로 뒤에 제거해야 됨.
         */
        virtual void OnConnCreated(std::shared_ptr<ITCPConnection> conn) = 0;

        // Remote peer 에서 들어오는 데이터가 있을 때 호출 됨.
        virtual void OnReceivedData(const char* data, size_t length) = 0;

        /**
         * Remote 가 연결을 닫을 때 호출 됨.
         * 연결은 유지되나 에러 없이 데이터는 전달 안 됨.
         */
        virtual void OnRemoteClosed() = 0;

        // 연결 시 에러 발생될 경우 호출 됨.
        virtual void OnError(int errorCode, const std::string& errorMsg) = 0;
    };

    virtual ~ITCPConnection() { }

    /**
     * Remote peer 에 데이터를 전송하기 위한 queue 에 데이터를 삽입.
     * 성공하면 true 반환.
     */
    virtual bool Send(const void* data, size_t length) = 0;

    bool Send(const std::string& data) { return Send(data.data(), data.size()); }

    virtual std::string GetLocalIP() const = 0;

    virtual int GetLocalPort() const = 0;

    virtual std::string GetRemoteIP() const = 0;

    virtual int GetRemotePort() const = 0;

    /**
     * Close connection gracefully.
     * 대기열에 있는 모든 outgoing data 를 보낸 후 FIN packet 을 보냄.
     * Remote 가 연결을 닫을 때까지 들어오는 모든 데이터를 받음.
     */
    virtual void Shutdown() = 0;

    /**
     * 연결을 끊는다.
     * RST packet 을 보내고 대기열의 모든 데이터가 손실 됨.
     */
    virtual void Close() = 0;

    std::shared_ptr<Callback> GetCallback() const { return callback_; }

    // TODO TLS
protected:
    std::shared_ptr<Callback> callback_;

    ITCPConnection(std::shared_ptr<Callback> callback)
        : callback_(std::move(callback))
    {
    }
};

// Listen server interface
class IListenServer
{
    friend class Network;

public:
    virtual ~IListenServer() { }

    /**
     * 서버를 중지하고 더 이상 들어오는 연결이 허용되지 않음.
     * 모든 연결이 shut down (ITCPConn::Shutdown()) 된다.
     */
    virtual void Close() = 0;

    virtual bool IsListening() const = 0;
};

class Network
{
public:
    // 서버로 들어오는 연결을 수신 할 때 사용되는 콜백
    class ListenCallback
    {
    public:
        virtual ~ListenCallback() { }

        /**
         * Listen()으로 생성 된 TCP 서버가 새로운 수신 연결을 받을 때 호출 됨.
         * nullptr이 반환되면 연결이 즉시 삭제 됨.
         * 그렇지 않으면 새 TCPConn 인스턴스가 생성되고 OnAccepted()가 호출 됨.
         */
        virtual std::shared_ptr<ITCPConnection::Callback> OnIncomingConnection(
            const std::string& remoteIPAddress, int remotePort)
            = 0;

        /**
         * Listen()으로 만든 TCP 서버가 들어오는 연결에 대한 TCPConn 을 만들 때 호출 됨.
         * 통신에 사용할 수있는 새로 생성 된 connection을 제공한다.
         * OnIncomingConnection() 이 성공된 직후에 호출 됨.
         */
        virtual void OnAccepted(ITCPConnection& conn) = 0;

        // 소켓이 지정된 포트에서 listen 이 실패할 때 호출 됨.
        virtual void OnError(int errorCode, const std::string& errorMsg) = 0;
    };

    /**
     * Listen.
     * 각 수신 연결에 대해 OnAccepted 콜백을 호출.
     * 각 연결에 대해 지정된 TCPConn::Callback 이있는 TCPConn가 생성 됨.
     * 작업 상태를 쿼리하고 서버를 닫는 데 사용할 수있는 ListenServer을 반환.
     */
    static std::shared_ptr<IListenServer> Listen(
        int port, std::shared_ptr<ListenCallback> listenCallback);
};
