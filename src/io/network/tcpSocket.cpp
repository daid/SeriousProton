#include <io/network/tcpSocket.h>
#include <logging.h>

#ifdef __WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
static constexpr int flags = 0;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
static constexpr int flags = MSG_NOSIGNAL;
#endif


extern "C" {
    struct X509;
    struct X509_STORE;
    struct SSL_CTX;
    struct SSL_METHOD;
    struct SSL;

    static X509_STORE* (*X509_STORE_new)();
    static X509* (*d2i_X509)(X509**, const unsigned char **, long);
    static int (*X509_STORE_add_cert)(X509_STORE*, X509*);
    static void (*X509_free)(X509*);
    static SSL_CTX* (*SSL_CTX_new)(const SSL_METHOD*);
    static const SSL_METHOD* (*TLSv1_2_client_method)();
    static long (*SSL_CTX_set_options)(SSL_CTX*, long);
    static int (*SSL_CTX_set_default_verify_paths)(SSL_CTX*);
    static void (*SSL_CTX_set_cert_store)(SSL_CTX*, X509_STORE*);
    static SSL* (*SSL_new)(SSL_CTX*);
    static int (*SSL_set_fd)(SSL *ssl, int fd);
    static int (*SSL_connect)(SSL *ssl);
    static long (*SSL_get_verify_result)(const SSL *ssl);
    static int (*SSL_read)(SSL *ssl, void *buf, int num);
    static int (*SSL_write)(SSL *ssl, const void *buf, int num);
    static void (*SSL_free)(SSL *ssl);
    
    static SSL_CTX* ssl_context;
}

# define SSL_OP_NO_SSLv2                                 0x01000000L
# define SSL_OP_NO_SSLv3                                 0x02000000L
# define SSL_OP_NO_TLSv1                                 0x04000000L
# define SSL_OP_NO_TLSv1_2                               0x08000000L
# define SSL_OP_NO_TLSv1_1                               0x10000000L

#include "dynamicLibrary.h"

static std::unique_ptr<DynamicLibrary> libcrypto;
static std::unique_ptr<DynamicLibrary> libssl;

static void initializeLibSSL()
{
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

#ifdef __WIN32
    libcrypto = DynamicLibrary::open("libcrypto-1_1.dll");
    libssl = DynamicLibrary::open("libssl-1_1.dll");
#else
    libcrypto = DynamicLibrary::open("libcrypto.so.1.1");
    libssl = DynamicLibrary::open("libssl.so.1.1");
#endif
    if (!libcrypto || !libssl)
        return;

    X509_STORE_new = libcrypto->getFunction<X509_STORE*(*)()>("X509_STORE_new");
    d2i_X509 = libcrypto->getFunction<X509* (*)(X509**, const unsigned char **, long)>("d2i_X509");
    X509_STORE_add_cert = libcrypto->getFunction<int (*)(X509_STORE*, X509*)>("X509_STORE_add_cert");
    X509_free = libcrypto->getFunction<void (*)(X509*)>("X509_free");

    SSL_CTX_new = libssl->getFunction<SSL_CTX*(*)(const SSL_METHOD*)>("SSL_CTX_new");
    TLSv1_2_client_method = libssl->getFunction<const SSL_METHOD* (*)()>("TLSv1_2_client_method");
    SSL_CTX_set_options = libssl->getFunction<long (*)(SSL_CTX*, long)>("SSL_CTX_set_options");
    SSL_CTX_set_default_verify_paths = libssl->getFunction<int (*)(SSL_CTX*)>("SSL_CTX_set_default_verify_paths");
    SSL_CTX_set_cert_store = libssl->getFunction<void (*)(SSL_CTX*, X509_STORE*)>("SSL_CTX_set_cert_store");
    SSL_new = libssl->getFunction<SSL* (*)(SSL_CTX*)>("SSL_new");
    SSL_set_fd = libssl->getFunction<int (*)(SSL *ssl, int fd)>("SSL_set_fd");
    SSL_connect = libssl->getFunction<int (*)(SSL *ssl)>("SSL_connect");
    SSL_get_verify_result = libssl->getFunction<long (*)(const SSL *ssl)>("SSL_get_verify_result");
    SSL_read = libssl->getFunction<int (*)(SSL *ssl, void *buf, int num)>("SSL_read");
    SSL_write = libssl->getFunction<int (*)(SSL *ssl, const void *buf, int num)>("SSL_write");
    SSL_free = libssl->getFunction<void (*)(SSL *ssl)>("SSL_free");

#ifdef __WIN32
    HCERTSTORE hStore;
    PCCERT_CONTEXT pContext = NULL;
    X509 *x509;
    X509_STORE *store = X509_STORE_new();

    hStore = CertOpenSystemStore(0, "ROOT");
    while((pContext = CertEnumCertificatesInStore(hStore, pContext)) != nullptr)
    {
        const unsigned char* c = pContext->pbCertEncoded;
        x509 = d2i_X509(nullptr, &c, pContext->cbCertEncoded);
        if (x509)
        {
            X509_STORE_add_cert(store, x509);
            X509_free(x509);
        }
    }
    CertFreeCertificateContext(pContext);
    CertCloseStore(hStore, 0);
#endif

    ssl_context = SSL_CTX_new(TLSv1_2_client_method());
    SSL_CTX_set_options(ssl_context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
#ifdef __WIN32
    SSL_CTX_set_cert_store(ssl_context, store);
#else
    SSL_CTX_set_default_verify_paths(ssl_context);
#endif
}


namespace sp {
namespace io {
namespace network {


TcpSocket::TcpSocket()
: ssl_handle(nullptr)
{
}

TcpSocket::TcpSocket(TcpSocket&& socket)
{
    *this = std::move(socket);
}

TcpSocket::~TcpSocket()
{
    close();
}

TcpSocket& TcpSocket::operator=(TcpSocket&& socket)
{
    close();

    handle = socket.handle;
    ssl_handle = socket.ssl_handle;
    send_queue = std::move(socket.send_queue);
    blocking = socket.blocking;
    receive_buffer = std::move(socket.receive_buffer);
    received_size = socket.received_size;

    socket.handle = -1;
    socket.send_queue.clear();
    socket.receive_buffer.clear();
    socket.received_size = 0;
    socket.ssl_handle = nullptr;

    return *this;
}

bool TcpSocket::connect(const Address& host, int port)
{
    if (isConnected())
        close();

    for(const auto& addr_info : host.addr_info)
    {
        handle = ::socket(addr_info.family, SOCK_STREAM, 0);
        if (handle < 0)
            return false;
        if (addr_info.family == AF_INET && sizeof(struct sockaddr_in) == addr_info.addr.length())
        {
            struct sockaddr_in server_addr;
            memset(&server_addr, 0, sizeof(server_addr));
            memcpy(&server_addr, addr_info.addr.data(), addr_info.addr.length());
            server_addr.sin_port = htons(port);
            if (::connect(handle, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr)) == 0)
            {
                setBlocking(blocking);
                return true;
            }
        }
        if (addr_info.family == AF_INET6 && sizeof(struct sockaddr_in6) == addr_info.addr.length())
        {
            struct sockaddr_in6 server_addr;
            memset(&server_addr, 0, sizeof(server_addr));
            memcpy(&server_addr, addr_info.addr.data(), addr_info.addr.length());
            server_addr.sin6_port = htons(port);
            if (::connect(handle, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr)) == 0)
            {
                setBlocking(blocking);
                return true;
            }
        }
        close();
    }
    return false;
}

bool TcpSocket::connectSSL(const Address& host, int port)
{
    if (!connect(host, port))
        return false;
    initializeLibSSL();
    if (!SSL_new)
    {
        LOG(Warning, "Failed to connect SSL socket due to missing libssl/libcrypto v1.1");
        close();
        return false;
    }
    
    ssl_handle = SSL_new(ssl_context);
    SSL_set_fd(static_cast<SSL*>(ssl_handle), handle);
    if (!SSL_connect(static_cast<SSL*>(ssl_handle)))
    {
        LOG(Warning, "Failed to connect SSL socket due to SSL negotiation failure.");
        close();
        return false;
    }
    if (SSL_get_verify_result(static_cast<SSL*>(ssl_handle)) != 0)
    {
        LOG(Warning, "Failed to connect SSL socket due to certificate verfication failure.");
        close();
        return false;
    }
    return true;
}

void TcpSocket::close()
{
    if (isConnected())
    {
#ifdef __WIN32
        closesocket(handle);
#else
        ::close(handle);
#endif
        handle = -1;
        send_queue.clear();
        if (ssl_handle)
            SSL_free(static_cast<SSL*>(ssl_handle));
        ssl_handle = nullptr;
    }
}

bool TcpSocket::isConnected()
{
    return handle != -1;
}

void TcpSocket::send(const void* data, size_t size)
{
    if (!isConnected())
        return;
    if (sendSendQueue())
    {
        send_queue += std::string(static_cast<const char*>(data), size);
        return;
    }

    for(size_t done = 0; done < size; )
    {
        int result;
        if (ssl_handle)
            result = SSL_write(static_cast<SSL*>(ssl_handle), static_cast<const char*>(data) + done, size - done);
        else
            result = ::send(handle, static_cast<const char*>(data) + done, size - done, flags);
        if (result < 0)
        {
            if (!isLastErrorNonBlocking())
                close();
            else
                send_queue += std::string(static_cast<const char*>(data) + done, size - done);
            return;
        }
        done += result;
    }
}

size_t TcpSocket::receive(void* data, size_t size)
{
    sendSendQueue();
    
    if (!isConnected())
        return 0;
    
    int result;
    if (ssl_handle)
        result = SSL_read(static_cast<SSL*>(ssl_handle), static_cast<char*>(data), size);
    else
        result = ::recv(handle, static_cast<char*>(data), size, flags);
    if (result < 0)
    {
        result = 0;
        if (!isLastErrorNonBlocking())
            close();
    }
    return result;
}

void TcpSocket::send(const io::DataBuffer& buffer)
{
    io::DataBuffer packet_size(uint32_t(buffer.getDataSize()));
    send(packet_size.getData(), packet_size.getDataSize());
    send(buffer.getData(), buffer.getDataSize());
}

bool TcpSocket::receive(io::DataBuffer& buffer)
{
    if (!isConnected())
        return 0;
    
    if (receive_buffer.size() == 0)
    {
        uint8_t size_buffer[sizeof(uint32_t)];
        size_t idx = 0;
        while(idx < sizeof(uint32_t))
        {
            //TOFIX: This blocks if we receive less then 4 bytes. Allows denial of service attack.
            int result;
            if (ssl_handle)
                result = SSL_read(static_cast<SSL*>(ssl_handle), reinterpret_cast<char*>(&size_buffer[idx]), sizeof(uint32_t) - idx);
            else
                result = ::recv(handle, reinterpret_cast<char*>(&size_buffer[idx]), sizeof(uint32_t) - idx, flags);
            if (result < 0)
            {
                result = 0;
                if (!isLastErrorNonBlocking())
                {
                    close();
                    return false;
                }
            }
            if (result == 0 && idx == 0)
                return false;
            idx += result;
        }
        uint32_t size = *reinterpret_cast<uint32_t*>(size_buffer);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        size = __builtin_bswap32(size);
#endif
        receive_buffer.resize(size);
        received_size = 0;
    }

    while(true)
    {
        int result;
        if (ssl_handle)
            result = SSL_read(static_cast<SSL*>(ssl_handle), reinterpret_cast<char*>(&receive_buffer[received_size]), receive_buffer.size() - received_size);
        else
            result = ::recv(handle, reinterpret_cast<char*>(&receive_buffer[received_size]), receive_buffer.size() - received_size, flags);
        if (result < 0)
        {
            result = 0;
            if (!isLastErrorNonBlocking())
            {
                close();
                return false;
            }
        }
        received_size += result;
        if (received_size == receive_buffer.size())
        {
            buffer = std::move(receive_buffer);
            received_size = 0;
            return true;
        }
        if (result < 1)
            break;
    }
    
    return false;
}

bool TcpSocket::sendSendQueue()
{
    if (send_queue.size() < 1)
        return false;
    
    int result;
    do
    {
        if (ssl_handle)
            result = SSL_write(static_cast<SSL*>(ssl_handle), static_cast<const char*>(send_queue.data()), send_queue.size());
        else
            result = ::send(handle, static_cast<const char*>(send_queue.data()), send_queue.size(), flags);
        if (result < 0)
        {
            result = 0;
            if (!isLastErrorNonBlocking())
                close();
        }
        if (result > 0)
            send_queue = send_queue.substr(result);
    } while(result > 0 && send_queue.size() > 0);
    
    return send_queue.size() > 0;
}

}//namespace network
}//namespace io
}//namespace sp
