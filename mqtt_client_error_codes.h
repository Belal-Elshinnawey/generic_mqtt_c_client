

#ifndef _MQTT_CLIENT_ERROR_CODES_H
#define _MQTT_CLIENT_ERROR_CODES_H

#ifdef __cplusplus
extern "C"
{
#endif
#define SSL_ERROR_WANT_READ -0x6900
#define SSL_ERROR_WANT_WRITE -0x6880
#define SSL_ERROR_WANT_CONNECT 7
#define SSL_ERROR_WANT_ACCEPT 8
#define SSL_ERROR_SYSCALL 5
#define SSL_ERROR_WANT_X509_LOOKUP 83
#define SSL_ERROR_ZERO_RETURN 6
#define SSL_ERROR_SSL 85

#define EPERM 1
#define ENOENT 2
#define ESRCH 3
#define EINTR 4
#define EIO 5
#define ENXIO 6
#define E2BIG 7
#define ENOEXEC 8
#define EBADF 9
#define ECHILD 10
#define EAGAIN 11
#define ENOMEM 12
#define EACCES 13
#define EFAULT 14
#define ENOTBLK 15
#define EBUSY 16
#define EEXIST 17
#define EXDEV 18
#define ENODEV 19
#define ENOTDIR 20
#define EISDIR 21
#define EINVAL 22
#define ENFILE 23
#define EMFILE 24
#define ENOTTY 25
#define ETXTBSY 26
#define EFBIG 27
#define ENOSPC 28
#define ESPIPE 29
#define EROFS 30
#define EMLINK 31
#define EPIPE 32
#define EDOM 33
#define ERANGE 34
#define ENOMSG 35
#define EDEADLK 45
#define ENOLCK 46
#define ENOSTR 60
#define ENODATA 61
#define ETIME 62
#define ENOSR 63
#define EPROTO 71
#define EBADMSG 77
#define ENOSYS 88
#define ENOTEMPTY 90
#define ENAMETOOLONG 91
#define ELOOP 92
#define EOPNOTSUPP 95
#define EPFNOSUPPORT 96
#define ECONNRESET 104
#define ENOBUFS 105
#define EAFNOSUPPORT 106
#define EPROTOTYPE 107
#define ENOTSOCK 108
#define ENOPROTOOPT 109
#define ESHUTDOWN 110
#define ECONNREFUSED 111
#define EADDRINUSE 112
#define ECONNABORTED 113
#define ENETUNREACH 114
#define ENETDOWN 115
#define ETIMEDOUT 116
#define EHOSTDOWN 117
#define EHOSTUNREACH 118
#define EINPROGRESS 119
#define EALREADY 120
#define EDESTADDRREQ 121
#define EMSGSIZE 122
#define EPROTONOSUPPORT 123
#define ESOCKTNOSUPPORT 124
#define EADDRNOTAVAIL 125
#define ENETRESET 126
#define EISCONN 127
#define ENOTCONN 128
#define ETOOMANYREFS 129
#define ENOTSUP 134
#define EILSEQ 138
#define EOVERFLOW 139
#define ECANCELED 140
#define EWOULDBLOCK EAGAIN
#ifdef __cplusplus
}
#endif

#endif