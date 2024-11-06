#include <io/network/tcpSocket.h>
#include <logging.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
static constexpr int flags = 0;

static inline int send(SOCKET s, const void* msg, size_t len, int flags)
{
    return send(s, static_cast<const char*>(msg), static_cast<int>(len), flags);
}

static inline int recv(SOCKET s, void* buf, size_t len, int flags)
{
    return recv(s, static_cast<char*>(buf), static_cast<int>(len), flags);
}

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <poll.h>
#if defined(__APPLE__)
static constexpr int flags = 0;
#else
static constexpr int flags = MSG_NOSIGNAL;
#endif
static constexpr intptr_t INVALID_SOCKET = -1;
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

#ifndef __ANDROID__
#include "dynamicLibrary.h"

static std::unique_ptr<DynamicLibrary> libcrypto;
static std::unique_ptr<DynamicLibrary> libssl;
#endif

static void initializeLibSSL()
{
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

#ifndef __ANDROID__
#ifdef _WIN32
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

#ifdef _WIN32
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
#ifdef _WIN32
    SSL_CTX_set_cert_store(ssl_context, store);
#else
    SSL_CTX_set_default_verify_paths(ssl_context);
#endif
#endif
}


namespace sp {
namespace io {
namespace network {


TcpSocket::TcpSocket()
: ssl_handle(nullptr)
{
}

TcpSocket::~TcpSocket()
{
    close();
}

bool TcpSocket::connect(const Address& host, int port)
{
    if (handle != INVALID_SOCKET)
        close();
    
    for(const auto& addr_info : host.addr_info)
    {
        handle = ::socket(addr_info.family, SOCK_STREAM, 0);
        if (handle == INVALID_SOCKET)
            return false;
        setBlocking(blocking);
        if (addr_info.family == AF_INET && sizeof(struct sockaddr_in) == addr_info.addr.size())
        {
            struct sockaddr_in server_addr;
            memset(&server_addr, 0, sizeof(server_addr));
            memcpy(&server_addr, addr_info.addr.data(), addr_info.addr.size());
            server_addr.sin_port = htons(port);
            if (::connect(handle, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr)) == 0)
                return true;
            if (isLastErrorNonBlocking())
            {
                connecting = true;
                return true;
            }
        }
        if (addr_info.family == AF_INET6 && sizeof(struct sockaddr_in6) == addr_info.addr.size())
        {
            struct sockaddr_in6 server_addr;
            memset(&server_addr, 0, sizeof(server_addr));
            memcpy(&server_addr, addr_info.addr.data(), addr_info.addr.size());
            server_addr.sin6_port = htons(port);
            if (::connect(handle, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr)) == 0)
                return true;
            if (isLastErrorNonBlocking())
            {
                connecting = true;
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
    SSL_set_fd(static_cast<SSL*>(ssl_handle), static_cast<int>(handle));
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

void TcpSocket::setDelay(bool delay)
{
    if (handle == INVALID_SOCKET)
    {
        LOG(Warning, "Failed to setDelay due to being called on an incomplete socket");
        return;
    }
    int mode = delay ? 0 : 1;
    if (setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, (char*)&mode, sizeof(mode)) == -1)
    {
        LOG(Warning, "Failed to setDelay on a socket");
    }
}

void TcpSocket::close()
{
    if (handle != INVALID_SOCKET)
    {
#ifdef _WIN32
        closesocket(handle);
#else
        ::close(handle);
#endif
        handle = INVALID_SOCKET;
        connecting = false;
        clearQueue();
        if (ssl_handle)
            SSL_free(static_cast<SSL*>(ssl_handle));
        ssl_handle = nullptr;
    }
}

StreamSocket::State TcpSocket::getState()
{
    if (handle == INVALID_SOCKET)
        return StreamSocket::State::Closed;
    if (connecting) {
        struct pollfd fds;
        fds.fd = handle;
        fds.events = POLLOUT;
        fds.revents = 0;
#ifdef WIN32
        if (WSAPoll(&fds, 1, 0))
#else
        if (poll(&fds, 1, 0))
#endif
        {
            struct sockaddr_in6 server_addr;
            socklen_t server_addr_len = sizeof(server_addr);
            if (getpeername(handle, reinterpret_cast<sockaddr*>(&server_addr), &server_addr_len))
            {
                close();
                return StreamSocket::State::Closed;
            }
            connecting = false;
            return StreamSocket::State::Connected;
        }
        return StreamSocket::State::Connecting;
    }
    return StreamSocket::State::Connected;
}

size_t TcpSocket::_send(const void* data, size_t size)
{
    int result;
    if (ssl_handle)
        result = SSL_write(static_cast<SSL*>(ssl_handle), static_cast<const char*>(data), static_cast<int>(size));
    else
        result = ::send(handle, reinterpret_cast<const void *>(static_cast<const char*>(data)), size, flags);
    if (result < 0)
    {
        if (!isLastErrorNonBlocking())
            close();
        return 0;
    }
    return result;
}

size_t TcpSocket::_receive(void* data, size_t size)
{
    int result;
    if (ssl_handle)
        result = SSL_read(static_cast<SSL*>(ssl_handle), static_cast<char*>(data), static_cast<int>(size));
    else
        result = ::recv(handle, data, size, flags);
    if (result < 0)
    {
        result = 0;
        if (!isLastErrorNonBlocking())
            close();
    }
    return result;
}

}//namespace network
}//namespace io
}//namespace sp
